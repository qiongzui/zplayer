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

	// init dumpfile
	_writer = std::make_shared<WriteFile>();
	std::string path = get_current_path() + "\\dump\\output.aac";
	_writer->start(path);

	// init decoder
	_vdecoder = std::make_shared<FFDecoder>();
	_vdecoder->init(video_stream);
	_adecoder = std::make_shared<FFDecoder>();
	_adecoder->init(audio_stream);

	// init render
	_vrender = std::make_shared<VRender>(_surface);
	if (!_vrender) {
		return;
	}
	if (_vrender->init()) {
		return;
	}

	_packet_queue.init(10);
	_aframe = av_frame_alloc();
	_vframe = av_frame_alloc();

	// start thread
	_demuxer_thread = std::thread(&Client::demuxerThread, this);
	_adecoder_thread = std::thread(&Client::adecoderThread, this);
	_vdecoder_thread = std::thread(&Client::vdecoderThread, this);
	// _arender_thread = std::thread(&Client::arenderThread, this);
	// _vrender_thread = std::thread(&Client::vrenderThread, this);

	_state = PlayState::Prepared;
}

void Client::release() {
	if (_state != PlayState::Stopped) {
		return;
	}
	
	if (_demuxer) {
		_demuxer->release();
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
	
	// if (_arender_thread.joinable()) {
	// 	_arender_thread.join();
	// }
	
	// if (_vrender_thread.joinable()) {
	// 	_vrender_thread.join();
	// }

	_packet_queue.release();

	if (_aframe) {
		av_frame_free(&_aframe);
		_aframe = nullptr;
	}

	if (_vframe) {
		av_frame_free(&_vframe);
		_vframe = nullptr;
	}

	if (_writer) {
		_writer->stop();
		_writer = nullptr;
	}
	_state = PlayState::Idle;
}

int Client::play() {
	if (_state != PlayState::Prepared) {
		return -1;
	}
	
	_state = PlayState::Playing;
	return 0;
}

int Client::pause() {
	if (_state != PlayState::Playing) {
		return -1;
	}
	
	_state = PlayState::Paused;
	return 0;
}

int Client::seek(int timestamp) {
	if (_state == PlayState::Playing) {
		_state = PlayState::Paused;
	}

	if (_demuxer) {
		_demuxer->seek(timestamp);
	}

	_state = PlayState::Playing;
	return 0;
}

int Client::stop() {
	_state = PlayState::Stopped;
	return 0;
}

void Client::setState(PlayState state)
{
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
		return "Paused";
	default:
		return "Error State";
	}
}

void Client::demuxerThread() {
	if (!_demuxer) {
		return;
	}
	
	AVPacket* packet = nullptr;
	int keyId = -1;
	while (true) {
		if (_state == PlayState::Stopped) {
			break;
		}

		if (_state != PlayState::Playing) {
			continue;
		}
			
		packet = _packet_queue.empty_dequeue();

		if (!packet) {
			continue;
		}
		_demuxer->readPacket(packet);
		if (!packet->data) {
			_packet_queue.empty_enqueue(packet);
			continue;
		}

		if (_demuxer->getVideoStream() && packet->stream_index == _demuxer->getVideoStream()->index) {
			_packet_queue.video_full_enqueue(packet);
			_vpacket_cv.notify_all();
		} else if (_demuxer->getAudioStream() && packet->stream_index == _demuxer->getAudioStream()->index) {
			_packet_queue.audio_full_enqueue(packet);
			_apacket_cv.notify_all();
		}
	}
	
}

void Client::adecoderThread() {
	if (!_adecoder) {
		return;
	}

	AVPacket* packet = nullptr;
	int ret = 0;
	while (true) {
		if (_state != PlayState::Playing) {
			continue;
		}
		{		
			std::unique_lock<std::mutex> lock(_apacket_queue_mutex);
			_apacket_cv.wait(lock, [&]() {
				return _packet_queue.get_audio_full_queue_size() > 0 || _state == PlayState::Stopped;
			});

			if (_state == PlayState::Stopped) {
				break;
			}

			packet = _packet_queue.audio_full_dequeue();
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
			if (_arender && (_aframe->sample_rate != _sampleRate ||
				_aframe->ch_layout.nb_channels != _channels)) {
					_arender->release();
					_arender = nullptr;
			}
			
			if (!_arender) {
				_sampleRate = _aframe->sample_rate;
				_channels = _aframe->ch_layout.nb_channels;
				_arender = std::make_shared<ARender>(_sampleRate, _channels, 2);
				if (!_arender) {
					return;
				}

				if (_arender->init()) {
					return;
				}
			}

			_arender->render(_aframe);

			av_frame_unref(_aframe);
		}

		_packet_queue.empty_enqueue(packet);
	
	}
}

void Client::vdecoderThread() {
	if (!_vdecoder) {
		return;
	}
	
	AVPacket* packet = nullptr;
	int ret = 0;
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
				return _packet_queue.get_video_full_queue_size() > 0 || _state == PlayState::Stopped;
			});
			if (_state == PlayState::Stopped) {
				break;
			}
		}
		packet = _packet_queue.video_full_dequeue();

		if (!packet) {
			continue;
		}
		ret = _vdecoder->decode(packet, _vframe);
		if (ret != 0) {
			continue;
		}

		if (_vrender) {
			_vrender->render(_vframe);
		}

		av_frame_unref(_vframe);

		_packet_queue.empty_enqueue(packet);
	}
}

void Client::arenderThread() {
	if (!_adecoder) {
		return;
	}
	
	while (true) {
		if (_state == PlayState::Stopped) {
			break;
		}
		if (_state != PlayState::Playing) {
			continue;
		}

		if (_anumofdecoded == 0) {
			continue;
		}

		auto ret = _adecoder->receive_frame(_aframe);
		if (ret != 0) {
			continue;
		}
		
		_anumofdecoded--;
		// render
		//logi("aframe pts: %ld", _aframe->pts);
		auto aframeDurationMs = _aframe->duration * 1000 / _demuxer->getAudioStream()->time_base.den;
		// sleep(aframeDurationMs);
		_currentTimestampMs += aframeDurationMs;
		av_frame_unref(_aframe);
		break;
	}
	
}

void Client::vrenderThread() {
	if (!_vdecoder) {
		return;
	}
	
	while (true) {
		if (_state == PlayState::Stopped) {
			break;
		}
		if (_state != PlayState::Playing) {
			continue;
		}

		if (_vnumofdecoded == 0) {
			continue;
		}
		
		auto ret = _vdecoder->receive_frame(_vframe);
		if (ret != 0) {
			loge("ERROR: vdecoder receive_frame failed: %s", ff_error(ret));
			continue;
		}
		
		_vnumofdecoded--;
		// render
		if (_lastRenderTimestampMs != 0)  {
			int64_t currentTimestampMs = get_current_timestamp();
			int64_t delta = currentTimestampMs - _lastRenderTimestampMs;
			if (delta < _renderDelay) {
				sleep(_renderDelay - delta);
			}
		}

		_vrender->render(_vframe);
		av_frame_unref(_vframe);
		_lastRenderTimestampMs = get_current_timestamp();
		break;
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