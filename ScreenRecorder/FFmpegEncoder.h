#pragma once

extern "C"
{
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
#include "libavdevice/avdevice.h"
#include <libswscale/swscale.h>

#pragma comment(lib, "avcodec.lib")
#pragma comment(lib, "avdevice.lib")
#pragma comment(lib, "avfilter.lib")
#pragma comment(lib, "avformat.lib")
#pragma comment(lib, "avutil.lib")
#pragma comment(lib, "postproc.lib")
#pragma comment(lib, "swresample.lib")
#pragma comment(lib, "swscale.lib")
};

#include <boost/thread/mutex.hpp>

class FFmpegEncoder
{
public:
	FFmpegEncoder();
	~FFmpegEncoder();

	int init();

	int start(char* outFilePath, int frameRate = 25);

	void stop();

	int flush_encoder();

	void encode(AVFrame* pYUVFrame);

private:

	boost::mutex decodeMutex;

	int frameRate = 25;

	int i = 0;
	char *outFilePath = nullptr;
	AVFormatContext* pFormatCtx = nullptr;
	AVOutputFormat* fmt = nullptr;
	AVStream* video_st = nullptr;
	AVCodecContext* pCodecCtx = nullptr;
	AVCodec* pCodec = nullptr;
	AVPacket pkt = {};
	int width = 0, height = 0;

	//uint8_t* picture_buf = nullptr;
	//AVFrame* pFrame = nullptr;
	int picture_size = 0;
	int y_size = 0;
	int framecnt = 0;
};

