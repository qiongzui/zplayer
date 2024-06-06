#pragma once
#include <string>
#include <iostream>
#include <thread>
#include <atomic>
#include <mutex>
#include "demuxer.h"
#include "ff_decoder.h"
#include "zqueue.h"
#include "vrender.h"
#include "arender.h"
#include "write_file.h"
#include "speech_recognizer.h"

namespace ZPlayer {
	enum PlayState
	{
		Idle,
		Prepared,
		Playing,
		Stopped,
		Paused
	};

	class Client
	{
	public:
		void setDataSource(std::string file);
		void setSurface(void* surface);
		void prepare();
		void release();

		int play();
		int pause();
		int seek(int timestamp);
		int stop();
		int render();

		int getDurationMs();
		int getCurrentTimestampMs() { return _avSync->getMasterClock(); }
	private:
		void setState(PlayState state);
		std::string printState(PlayState state);
		int flush();

		void demuxerThread();
		void adecoderThread();
		void vdecoderThread();
		void arenderThread();
		void vrenderThread();
	private:
		std::string _dataSource;
		void* _surface = nullptr;
		PlayState _state = Idle;
		int _renderDelay = 16;
		std::shared_ptr<ZDemuxer> _demuxer = nullptr;
		std::shared_ptr<FFDecoder> _vdecoder = nullptr;
		std::shared_ptr<FFDecoder> _adecoder = nullptr;
		std::shared_ptr<ARender> _arender = nullptr;
		std::shared_ptr<VRender> _vrender = nullptr;

		std::queue<AVPacket*> _audio_q;
		std::queue<AVPacket*> _video_q;
		std::mutex _apacket_queue_mutex;
		std::mutex _vpacket_queue_mutex;
		std::mutex _read_mutex;
		std::condition_variable _apacket_cv;
		std::condition_variable _vpacket_cv;
		std::condition_variable _read_cv;
		AVFrame* _aframe = nullptr;
		AVFrame* _vframe = nullptr;

		int _sampleRate = 0;
		int _channels = 0;

		int _lastFrameTimestampMs = 0;

		std::thread _demuxer_thread;
		std::thread _adecoder_thread;
		std::thread _vdecoder_thread;

		int _seekTimestampMs = 0;
		int _lastRenderTimestampMs = 0;

		std::shared_ptr<WriteFile> _writer = nullptr;

		std::shared_ptr<AVSync> _avSync = nullptr;

		int _videoFrameCount = 0;
		int _audioFrameCount = 0;

		std::unique_ptr<SpeechRecognizer> _speechRecognizer = nullptr;
	};
}
