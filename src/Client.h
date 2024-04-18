#pragma once
#include <string>
#include <iostream>
#include <thread>
#include <atomic>
#include <mutex>
#include "demuxer.h"
#include "ff_decoder.h"
#include "zqueue.h"
#include "render.h"
#include "write_file.h"


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

		int getDurationMs();
		int getCurrentTimestampMs() { return _currentTimestampMs; }
	private:
		void setState(PlayState state);
		std::string printState(PlayState state);

		void demuxerThread();
		void adecoderThread();
		void vdecoderThread();
		void arenderThread();
		void vrenderThread();
	private:
		std::string _dataSource;
		void* _surface = nullptr;
		std::atomic<PlayState> _state;
		int _renderDelay = 16;
		std::shared_ptr<ZDemuxer> _demuxer = nullptr;
		std::shared_ptr<FFDecoder> _vdecoder = nullptr;
		std::shared_ptr<FFDecoder> _adecoder = nullptr;
		std::shared_ptr<ZRender> _arender = nullptr;
		std::shared_ptr<ZRender> _vrender = nullptr;

		std::atomic_int _anumofdecoding = 0;
		std::atomic_int _vnumofdecoding = 0;

		Packet_Queue _packet_queue;
		std::mutex _packet_queue_mutex;
		AVFrame* _aframe = nullptr;
		AVFrame* _vframe = nullptr;

		std::thread _demuxer_thread;
		std::thread _adecoder_thread;
		std::thread _vdecoder_thread;
		std::thread _arender_thread;
		std::thread _vrender_thread;

		int _currentTimestampMs = 0;
		int _lastRenderTimestampMs = 0;

		std::shared_ptr<WriteFile> _writer = nullptr;
	};
}
