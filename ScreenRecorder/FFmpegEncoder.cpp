#include "stdafx.h"
#include "FFmpegEncoder.h"


FFmpegEncoder::FFmpegEncoder()
{
	init();
}


FFmpegEncoder::~FFmpegEncoder()
{
	stop();
}

void my_logoutput(void* ptr, int level, const char* fmt, va_list vl) {
	FILE *fp = fopen("ffmpeg_log.txt", "a+");
	if (fp) {
		vfprintf(fp, fmt, vl);
		fflush(fp);
		fclose(fp);
	}
}

int FFmpegEncoder::init()
{
	av_register_all();
	avformat_network_init();
	avdevice_register_all();

	av_log_set_level(AV_LOG_WARNING);
	av_log_set_callback(my_logoutput);

	return 0;
}

int FFmpegEncoder::start(char* outFilePath, int frameRate)
{
	this->outFilePath = outFilePath;
	this->frameRate = frameRate;

	//Method1.
	pFormatCtx = avformat_alloc_context();
	//Guess Format
	fmt = av_guess_format(NULL, outFilePath, NULL);
	pFormatCtx->oformat = fmt;

	//Method 2.
	//avformat_alloc_output_context2(&pFormatCtx, NULL, NULL, out_file);
	//fmt = pFormatCtx->oformat;


	//Open output URL
	if (avio_open(&pFormatCtx->pb, outFilePath, AVIO_FLAG_READ_WRITE) < 0)
	{
		printf("Failed to open output file! \n");
		return -1;
	}

	video_st = avformat_new_stream(pFormatCtx, 0);
	//video_st->time_base.num = 1; 
	//video_st->time_base.den = 25;  

	if (video_st == NULL)
	{
		return -1;
	}


	return 1;
}

void FFmpegEncoder::stop()
{
	//Flush Encoder
	int ret = flush_encoder();
	if (ret < 0)
	{
		printf("Flushing encoder failed\n");
		return;
	}

	//Write file trailer
	av_write_trailer(pFormatCtx);

	//Clean
	if (video_st)
	{
		avcodec_close(video_st->codec);
		//av_free(pFrame);
		//av_free(picture_buf);
	}
	avio_close(pFormatCtx->pb);
	avformat_free_context(pFormatCtx);
}

int FFmpegEncoder::flush_encoder()
{
	int ret;
	int got_frame;
	AVPacket enc_pkt;
	if (!(pFormatCtx->streams[0]->codec->codec->capabilities &
		CODEC_CAP_DELAY))
		return 0;
	while (1)
	{
		enc_pkt.data = NULL;
		enc_pkt.size = 0;
		av_init_packet(&enc_pkt);
		ret = avcodec_encode_video2(pFormatCtx->streams[0]->codec, &enc_pkt,
		                            NULL, &got_frame);
		av_frame_free(NULL);
		if (ret < 0)
			break;
		if (!got_frame)
		{
			ret = 0;
			break;
		}
		printf("Flush Encoder: Succeed to encode 1 frame!\tsize:%5d\n", enc_pkt.size);
		/* mux encoded frame */
		ret = av_write_frame(pFormatCtx, &enc_pkt);
		if (ret < 0)
			break;
	}
	return ret;
}

void FFmpegEncoder::encode(AVFrame* pYUVFrame)
{
	boost::mutex::scoped_lock lock(decodeMutex);

	if (width == 0 || height == 0)
	{
		width = pYUVFrame->width;
		height = pYUVFrame->height;

		//Param that must set
		pCodecCtx = video_st->codec;
		pCodecCtx->codec_id = fmt->video_codec;
		pCodecCtx->codec_type = AVMEDIA_TYPE_VIDEO;
		pCodecCtx->pix_fmt = AV_PIX_FMT_YUV420P;
		pCodecCtx->width = pYUVFrame->width;
		pCodecCtx->height = pYUVFrame->height;
		pCodecCtx->bit_rate = 4000000;
		pCodecCtx->gop_size = frameRate * 2;

		pCodecCtx->time_base.num = 1;
		pCodecCtx->time_base.den = frameRate;

		//H264
		//pCodecCtx->me_range = 16;
		//pCodecCtx->max_qdiff = 4;
		//pCodecCtx->qcompress = 0.6;
		pCodecCtx->qmin = 10;
		pCodecCtx->qmax = 51;

		//Optional Param
		pCodecCtx->max_b_frames = 3;

		// Set Option
		AVDictionary* param = 0;
		//H.264
		if (pCodecCtx->codec_id == AV_CODEC_ID_H264)
		{
			av_dict_set(&param, "preset", "slow", 0);
			//av_dict_set(&param, "preset", "ultrafast", 0);
			av_dict_set(&param, "tune", "zerolatency", 0);
			av_dict_set(&param, "profile", "high", 0);
		}
		//H.265
		if (pCodecCtx->codec_id == AV_CODEC_ID_H265)
		{
			av_dict_set(&param, "preset", "ultrafast", 0);
			av_dict_set(&param, "tune", "zero-latency", 0);
		}

		//Show some Information
		av_dump_format(pFormatCtx, 0, outFilePath, 1);

		pCodec = avcodec_find_encoder(pCodecCtx->codec_id);
		if (!pCodec)
		{
			printf("Can not find encoder! \n");
			return;
		}
		int ret = 0;
		if ((ret = avcodec_open2(pCodecCtx, pCodec, &param)) < 0)
		{
			printf("Failed to open encoder! \n");
			return;
		}

		//pFrame = av_frame_alloc();
		picture_size = avpicture_get_size(pCodecCtx->pix_fmt, pCodecCtx->width, pCodecCtx->height);
		//picture_buf = (uint8_t *)av_malloc(picture_size);
		//avpicture_fill((AVPicture *)pFrame, picture_buf, pCodecCtx->pix_fmt, pCodecCtx->width, pCodecCtx->height);

		//Write File Header
		avformat_write_header(pFormatCtx, NULL);

		av_new_packet(&pkt, picture_size);

		y_size = pCodecCtx->width * pCodecCtx->height;
	}

	pkgCount++;

	////PTS
	if(video_st->time_base.num == 0)
	{
		pYUVFrame->pts = 1;
	}
	else
	{
		pYUVFrame->pts = pkgCount * (video_st->time_base.den) / ((video_st->time_base.num) * frameRate);
	}
		
	int got_picture = 0;
	//Encode
	int ret = avcodec_encode_video2(pCodecCtx, &pkt, pYUVFrame, &got_picture);
	if (ret < 0)
	{
		printf("Failed to encode! \n");
		return;
	}
	if (got_picture == 1)
	{
		printf("Succeed to encode frame: %5d\tsize:%5d\n", framecnt, pkt.size);
		
		framecnt++;

		pkt.stream_index = video_st->index;
		ret = av_write_frame(pFormatCtx, &pkt);
		av_free_packet(&pkt);
	}
}
