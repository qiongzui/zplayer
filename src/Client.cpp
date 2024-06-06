#include "Client.h"
#include "zlog.h"
#include "ztools.h"

using namespace ZPlayer;

void Client::setDataSource(std::string file)
{
	// TODO: 转UTF-8
	_dataSource = file;
	logi("play file: %s", _dataSource.c_str());
}

void Client::setSurface(void* surface) {
	_surface = surface;
}

void Client::prepare() {
	if (_state != PlayState::Idle) {
		return;
	}

	// init module
	_demuxer = std::make_shared<ZDemuxer>();
	_demuxer->setDataSource(_dataSource);
	_demuxer->init();
	_renderDelay = 1.0 / _demuxer->getFrameRate() * 1000;
	auto video_stream = _demuxer->getVideoStream();
	auto audio_stream = _demuxer->getAudioStream();

	_sampleRate = _demuxer->getSampleRate();
	_channels = _demuxer->getChannels();

	// init dumpfile
	_writer = std::make_shared<WriteFile>();
	std::string path = get_current_path() + "\\dump\\output.h264";
	_writer->start(path);

	// init decoder
	_vdecoder = std::make_shared<FFDecoder>();
	_vdecoder->init(video_stream);
	_adecoder = std::make_shared<FFDecoder>();
	_adecoder->init(audio_stream);

	// init avsync
	_avSync = std::make_shared<AVSync>();

	// init speech recognizer
	_speechRecognizer = std::make_unique<SpeechRecognizer>();
	_speechRecognizer->start();

	// init render
	_vrender = std::make_shared<VRender>(_surface);
	if (!_vrender) {
		return;
	}
	_vrender->setSyncHandler(_avSync);

	if (_vrender->init()) {
		return;
	}

	_arender = std::make_shared<ARender>(_sampleRate, _channels, 2);
	if (!_arender) {
		return;
	}

	if (_arender->init()) {
		return;
	}

	_arender->setSyncHandler(_avSync);

	_aframe = av_frame_alloc();
	_vframe = av_frame_alloc();

	// start thread
	_demuxer_thread = std::thread(&Client::demuxerThread, this);
	_adecoder_thread = std::thread(&Client::adecoderThread, this);
	_vdecoder_thread = std::thread(&Client::vdecoderThread, this);

	setState(PlayState::Prepared);
}

void Client::release() {
	if (_state != PlayState::Stopped) {
		return;
	}

	if (_demuxer_thread.joinable()) {
		_demuxer_thread.join();
	}

	if (_adecoder_thread.joinable()) {
		_adecoder_thread.join();
	}

	if (_vdecoder_thread.joinable()) {
		_vdecoder_thread.join();
	}

	if (_demuxer) {
		_demuxer->release();
	}

	if (_speechRecognizer) {
		_speechRecognizer->stop();
		_speechRecognizer = nullptr;
	}

	if (_aframe) {
		av_frame_free(&_aframe);
		_aframe = nullptr;
	}

	if (_vframe) {
		av_frame_free(&_vframe);
		_vframe = nullptr;
	}

	while (!_audio_q.empty()) {
		av_packet_free(&_audio_q.front());
		_audio_q.pop();
	}

	while (!_video_q.empty()) {
		av_packet_free(&_video_q.front());
		_video_q.pop();
	}

	if (_writer) {
		_writer->stop();
		_writer = nullptr;
	}
	setState(PlayState::Idle);
}

int Client::play() {
	_avSync->start();
	if (_state != PlayState::Prepared) {
		return -1;
	}
	
	setState(PlayState::Playing);
	return 0;
}

int Client::pause() {
	if (_state != PlayState::Playing) {
		return -1;
	}
	
	setState(PlayState::Paused);
	return 0;
}

int Client::seek(int timestamp) {
	logi("seek start, pos: %lld", timestamp);
	std::lock_guard<std::mutex> lock(_read_mutex);
	if (_state == PlayState::Playing) {
		setState(PlayState::Paused);
	}

	flush();
	
	_seekTimestampMs = timestamp;
	if (_demuxer) {
		_demuxer->seek(timestamp);
	}

	if (_avSync) {
		_avSync->updateMasterClock(timestamp);
	}

	setState(PlayState::Playing);
	_read_cv.notify_all();
	return 0;
}

int Client::flush() {
	if (_state != PlayState::Paused) {
		setState(PlayState::Paused);
	}

	while (!_audio_q.empty()) {
		av_packet_free(&_audio_q.front());
		_audio_q.pop();
	}

	while (!_video_q.empty()) {
		av_packet_free(&_video_q.front());
		_video_q.pop();
	}

	if (_vdecoder) {
		_vdecoder->flush();
	}

	if (_adecoder) {
		_adecoder->flush();
	}

	if (_speechRecognizer) {
		_speechRecognizer->flush();
	}

	_avSync->updateMasterClock(0);

	return 0;
}

int Client::stop() {
	setState(PlayState::Stopped);
	_read_cv.notify_all();
	_apacket_cv.notify_all();
	_vpacket_cv.notify_all();
	if (_avSync) {
		_avSync->stop();
	}

	if (_speechRecognizer) {
		_speechRecognizer->stop();
	}
	return 0;
}

void Client::setState(PlayState state)
{
	if (state > Paused) {
		loge("ERROR: state: {%s}\n", printState(state));
		return;
	}
	if (_state == state) {
		loge("ERROR: state: {%s}\n", printState(state));
		return;
	}

	_state = state;
	logi("State: {%s}\n", printState(state));
}

std::string Client::printState(PlayState state)
{
	switch (state)
	{
	case PlayState::Idle:
		return "Idle";
	case PlayState::Prepared:
		return "Prepared";
	case PlayState::Playing:
		return "Playing";
	case PlayState::Stopped:
		return "Stopped";
	case PlayState::Paused:
		return "Paused";
	default:
		return "Error State";
	}
}

void Client::demuxerThread() {
	if (!_demuxer) {
		return;
	}
	
	AVPacket* packet = av_packet_alloc();
	int keyId = -1;
	while (true) {
		if (!packet) {
			continue;
		}
		
		{
			std::unique_lock<std::mutex> lock(_read_mutex);
			_read_cv.wait(lock, [&]() {
				return _state != PlayState::Playing || _video_q.size() < 50 || _audio_q.size() < 50;
				});

			if (_state == PlayState::Stopped) {
				break;
			}
			if (_state != PlayState::Playing) {
				continue;
			}
			_demuxer->readPacket(packet);
		}

		if (_demuxer->getVideoStream() && packet->stream_index == _demuxer->getVideoStream()->index) {
			_videoFrameCount++;
			static int64_t timestamp = get_current_timestamp();
			int64_t curTime = get_current_timestamp();
			if (curTime - timestamp > 200) {
				// logw("read video packet timeout, delta: %lld", curTime - timestamp);
				timestamp = curTime;

			}

			std::lock_guard<std::mutex> lock(_vpacket_queue_mutex);
			_video_q.emplace(av_packet_clone(packet));
			_vpacket_cv.notify_all();
		} else if (_demuxer->getAudioStream() && packet->stream_index == _demuxer->getAudioStream()->index) {
			if (packet->pts < _seekTimestampMs) {
				continue;
			}
			_audioFrameCount++;
			_audio_q.emplace(av_packet_clone(packet));
			_apacket_cv.notify_all();
		} else {
			logi("read finished, sum of video: %d, audio: %d\n", _videoFrameCount, _audioFrameCount);
			break;
		}

		av_packet_unref(packet);
	}
	
}

void Client::adecoderThread() {
	if (!_adecoder) {
		return;
	}

	AVPacket* packet = nullptr;
	int ret = 0;
	while (_state == PlayState::Playing) {
		if (_state != PlayState::Playing) {
			continue;
		}
		{		
			std::unique_lock<std::mutex> lock(_apacket_queue_mutex);
			_apacket_cv.wait(lock, [&]() {
				return _audio_q.size() > 0 || _state == PlayState::Stopped;
			});

			if (_state == PlayState::Stopped) {
				break;
			}

			packet = _audio_q.front();
			_audio_q.pop();
			_read_cv.notify_all();
		}
		if (!packet) {
			logi("adecoderThread: no packet\n");
			continue;
		}

		//_writer->write(packet->buf->data, packet->buf->size);
		ret = _adecoder->send_packet(packet);
		if (ret != 0) {
			loge("adecoderThread: send_packet failed, ret: %d", ret);
			continue;
		}

		while (ret >= 0) {
			ret = _adecoder->receive_frame(_aframe);
			if (ret < 0) {
				// loge("adecoderThread: receive_frame failed, ret: %d", ret);
				break;
			}

			_aframe->pts = static_cast<int64_t>(_aframe->pts * 1000.f / _aframe->sample_rate);

			auto pcm = std::make_shared<Pcm>();
			pcm->data = _aframe->data[0];
			pcm->size = _aframe->linesize[0];
			pcm->channels = _aframe->ch_layout.nb_channels;
			pcm->sample_rate = _aframe->sample_rate;
			pcm->sample_count = _aframe->nb_samples;

			if (_speechRecognizer) {
				_speechRecognizer->recognize(pcm);
			}
			_arender->render(_aframe);

			av_frame_unref(_aframe);
		}

		av_packet_free(&packet);
	}
}

void Client::vdecoderThread() {
	if (!_vdecoder) {
		return;
	}
	
	AVPacket* packet = nullptr;
	int ret = -1;
	while (true) {
		if (_state == PlayState::Stopped) {
			break;
		}
		if (_state != PlayState::Playing) {
			continue;
		}

		{
			std::unique_lock<std::mutex> lock(_vpacket_queue_mutex);
			_vpacket_cv.wait(lock, [&]() {
				return _video_q.size() > 0 || _state == PlayState::Stopped;
			});
			if (_state == PlayState::Stopped) {
				break;
			}

			packet = _video_q.front();
			_video_q.pop();
			_read_cv.notify_all();
		}

		if (packet->pts != 0) {
			packet->pts = _lastFrameTimestampMs + _renderDelay;
		}

		_lastFrameTimestampMs = packet->pts;
		ret = _vdecoder->send_packet(packet);
		if (ret != 0) {
			loge("vdecoderThread: send_packet failed, ret: %s", ff_error(ret));
		}

		while (ret >= 0) {
			ret = _vdecoder->receive_frame(_vframe);
			if (ret < 0) {
				//loge("vdecoderThread: receive_frame failed, ret: %s", ff_error(ret));
				continue;
			}

			if (_vframe->pts < _seekTimestampMs) {
				continue;
			}

			// _vframe->pts = packet->pts;
			if (_vrender) {
				_vrender->render(_vframe);
			}

			av_frame_unref(_vframe);
		}

		av_packet_free(&packet);
	}
}

int Client::getDurationMs() {
	if (_state == PlayState::Idle) {
		return 0;
	}
	if (_demuxer) {
		return _demuxer->getDurationMs();
	}
	return 0;
}

int Client::render() {
	_vrender->render(_vframe);
	return 0;
}