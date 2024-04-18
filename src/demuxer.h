#pragma once

#include <string>

extern "C" {
#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"
}


namespace ZPlayer {
	class ZDemuxer
	{
	public:
		void setDataSource(std::string file) { _url = file; };
		void init();
		void release();
		void readPacket(AVPacket* packdet);
		void seek(int64_t posMs);

		AVStream* getVideoStream();
		AVStream* getAudioStream();

		int getDurationMs() { return _durationMs; };
		int getFrameRate() { return _frameRate; }
	private:
		std::string _url;
		int _durationMs = -1;
		AVFormatContext* _formatContext = nullptr;
		AVPacket* _packet = nullptr;
		int _videoStreamIndex = -1;
		int _audioStreamIndex = -1;
		int _frameRate = 30;
		int _seekTimestampMs = -1;
	};
}
