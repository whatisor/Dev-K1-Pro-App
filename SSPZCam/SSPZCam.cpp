// SSPZCam.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"

#include <functional>
#include <memory>
#include <thread>
#include <stdlib.h>

#include "sspzcam.h"
#include "VideoCodec.h"

#ifdef DEBUG_VIEW
#include "opencv2/highgui.hpp"
#include "opencv2/imgproc.hpp"
#endif

#ifdef PROFILING
_timerDataInit
static int on_1 = 0;
static int on_2 = 0;
#else
#define _timerS(p)
#define _timerE(p)
#endif

#define DEFAULT_FORMAT AV_PIX_FMT_YUV420P
using namespace std::placeholders;
//to avoid reference from application
CVideoCodec*        m_pLeftCodec;
CVideoCodec*        m_pRightCodec;

SSPZCam::SSPZCam(std::string ip1, std::string ip2)
{
	setCameraIps(ip1, ip2);
	//setCameraIps("10.0.0.1", "10.0.0.2");
}
SSPZCam::~SSPZCam()
{
	consummerLeft->join();
	consummerRight->join();
}

void on_264_1(struct imf::SspH264Data *h264)
{	

#ifdef PROFILING
	if ((on_1 % 2) == 0)
	{
		_timerS("on_264_1_hardware");
		on_1++;
	}
	else
	{
		_timerE("on_264_1_hardware");
		on_1++;
	}
#endif
	//return;
	_timerS("on_264_1");
	//push to queue for processing
	imf::SspH264Data h264New = *h264;
	h264New.data = new uint8_t[h264->len];
	memcpy(h264New.data, h264->data, h264->len);
	leftQueue.push_work(h264New);
	_timerE("on_264_1");
}

#ifdef DEBUG_VIEW
void displayFrame(std::string windowName, video_data *frame, int w , int h)
{
	//cv::Mat img = cv::Mat(leftOutput->height*3/2, leftOutput->width, CV_8UC1, frame->data[0]);
//
	if (w && h) {


		cv::Mat y(cv::Size(frame->linesize[0], h), CV_8UC1, frame->data[0]);
		cv::Mat u(cv::Size(frame->linesize[1], h / 2), CV_8UC1, frame->data[1]);
		cv::Mat v(cv::Size(frame->linesize[2], h / 2), CV_8UC1, frame->data[2]);

		cv::Mat u_resized, v_resized;
		cv::resize(u, u_resized, cv::Size(frame->linesize[0], h), 0, 0, cv::INTER_NEAREST); //repeat u values 4 times
		cv::resize(v, v_resized, cv::Size(frame->linesize[0], h), 0, 0, cv::INTER_NEAREST); //repeat v values 4 times

		cv::Mat yuv;

		std::vector<cv::Mat> yuv_channels = { y, u_resized, v_resized };
		cv::merge(yuv_channels, yuv);

		cv::Mat bgr;
		cv::cvtColor(yuv, bgr, cv::COLOR_YUV2BGR);
		cv::imshow(cv::String(windowName), bgr);
		cv::waitKey(1);
	}
}
#endif

/** Consumming work
*/
void consuming(CVideoCodec * decoder, imf::SspH264Data h264, virtual_output* output, std::string debugName)
{
	_timerS(debugName);
	video_data *frame = new video_data;
	if (decoder->Decode(h264.data, h264.len, frame->data, (int*)frame->linesize))
	{
		frame->timestamp = h264.ntp_timestamp;

		//_timerS(debugName+"output");
		virtual_video(output, frame);
		//_timerE(debugName + "output");
#ifdef DEBUG_VIEW
		displayFrame(debugName,frame, output->width, output->height);
#endif
		//printf("2 - decoded size %d x %d, %lld\n", rightOutput->width, rightOutput->height, (frame->linesize[0]+ frame->linesize[1]+ frame->linesize[2]));
	}
	delete frame;
	delete h264.data;

	_timerE(debugName);
}

void on_264_2(struct imf::SspH264Data *h264)
{

#ifdef PROFILING
	if ((on_2 % 2) == 0)
	{
		_timerS("on_264_2_hardware");
		on_2++;
	}
	else
	{
		_timerE("on_264_2_hardware");
		on_2++;
	}
#endif
	//return;
	_timerS("on_264_2");
	
	//push to queue for processing
	imf::SspH264Data h264New = *h264;
	h264New.data = new uint8_t[h264->len];
	memcpy(h264New.data, h264->data, h264->len);
	rightQueue.push_work(h264New);

	_timerE("on_264_2");
}

void on_audio_data_1(struct imf::SspAudioData *audio)
{

}

void on_audio_data_2(struct imf::SspAudioData *audio)
{

}

void on_meta_1(struct imf::SspVideoMeta *v, struct imf::SspAudioMeta *a, struct imf::SspMeta *m)
{
	printf("on meta 1 wall clock %d", m->pts_is_wall_clock);
	printf("              video %dx%d %d/%d\n", v->width, v->height, v->unit, v->timescale);
	printf("              audio %d\n", a->sample_rate);
	//update left right camera width , height
	if (leftOutput == NULL)
		return;

	m_pLeftCodec = new CVideoCodec();
	m_pLeftCodec->SetCodecID(AV_CODEC_ID_H264);
	m_pLeftCodec->SetCodecParam(a->bitrate, v->width, v->height, v->gop, 0, DEFAULT_FORMAT);
	if (!m_pLeftCodec->InitDecode())
	{
		printf("Codec initializing is failed!\n");
	}

	leftOutput->width = v->width;
	leftOutput->height = v->height;
	//start output
	virtual_output_start(leftOutput);
}

void on_meta_2(struct imf::SspVideoMeta *v, struct imf::SspAudioMeta *a, struct imf::SspMeta *m)
{
	printf("on meta 1 wall clock %d", m->pts_is_wall_clock);
	printf("              video %dx%d %d/%d\n", v->width, v->height, v->unit, v->timescale);
	printf("              audio %d\n", a->sample_rate);
	if (rightOutput == NULL)
		return;

	m_pRightCodec = new CVideoCodec();
	m_pRightCodec->SetCodecID(AV_CODEC_ID_H264);
	m_pRightCodec->SetCodecParam(a->bitrate, v->width, v->height, v->gop, 0, DEFAULT_FORMAT);
	if (!m_pRightCodec->InitDecode())
	{
		printf("Codec initializing is failed!\n");
	}

	rightOutput->width = v->width;
	rightOutput->height = v->height;
	//start output
	virtual_output_start(rightOutput);
}

void on_disconnect()
{
	printf("on disconnect\n");
	//threadLooper->stop();
}

void setup(imf::Loop *loop)
{
	if (leftCamClient != NULL)
	{
		delete leftCamClient;
		leftCamClient = NULL;
	}
	if (rightCamClient != NULL)
	{
		delete rightCamClient;
		rightCamClient = NULL;
	}

	//left camera client
	leftCamClient = new imf::SspClient(leftCamIp, loop, 0x400000);
	leftCamClient->init();

	leftCamClient->setOnH264DataCallback(std::bind(&on_264_1, _1));
	leftCamClient->setOnMetaCallback(std::bind(&on_meta_1, _1, _2, _3));
	leftCamClient->setOnDisconnectedCallback(std::bind(&on_disconnect));
	leftCamClient->setOnAudioDataCallback(std::bind(&on_audio_data_1, _1));

	leftCamClient->start();

	//right camera client
	rightCamClient = new imf::SspClient(rightCamIp, loop, 0x400000);
	rightCamClient->init();

	rightCamClient->setOnH264DataCallback(std::bind(&on_264_2, _1));
	rightCamClient->setOnMetaCallback(std::bind(&on_meta_2, _1, _2, _3));
	rightCamClient->setOnDisconnectedCallback(std::bind(&on_disconnect));
	rightCamClient->setOnAudioDataCallback(std::bind(&on_audio_data_2, _1));

	rightCamClient->start();

	//create multithreading consummer
	//left camera consummer
	consummerLeft =  new std::thread ([&]() {

		while (true)
		{
			imf::SspH264Data frame = leftQueue.wait_and_pop();
			consuming(m_pLeftCodec, frame,leftOutput,"left");
			//std::cout << "Got some left work" << std::endl;
		}
	});

	//right camera consummer
	consummerRight = new std::thread([&]() {
		while (true)
		{
			imf::SspH264Data frame = rightQueue.wait_and_pop();
			consuming(m_pRightCodec, frame,rightOutput,"right");
			//std::cout << "Got some right work" << std::endl;
		}
	});
}

void virtual_output_create(int videoMode)
{
	if (output_running)
		return;
	if (videoMode != ModeVideo && videoMode != ModeVideo2)
		videoMode = ModeVideo;

	switch (videoMode)
	{
	case ModeVideo:
		leftOutput = new virtual_output;
		leftOutput->video_mode = ModeVideo;
		leftOutput->keep_ratio = true;
		break;
	case ModeVideo2:
		rightOutput = new virtual_output;
		rightOutput->video_mode = ModeVideo2;
		rightOutput->keep_ratio = true;
		break;
	default:
		break;
	}
}

bool virtual_output_start(virtual_output *data)
{
	bool start = false;
	virtual_output *out_data = data;
	if (out_data == NULL)
		return start;
	AVPixelFormat fmt = DEFAULT_FORMAT;
	double fps = DEFAULT_FPS;
	uint64_t interval = static_cast<int64_t>(1000000000 / fps);
	printf("started video mode --------- %d", out_data->video_mode);
	start = shared_queue_create(&out_data->video_queue, out_data->video_mode, fmt, out_data->width, out_data->height, interval, out_data->delay + 10);
	if (start) {

		output_running = true;
		shared_queue_set_delay(&out_data->video_queue, out_data->delay);

	}
	else {
		output_running = false;

		shared_queue_write_close(&out_data->video_queue);
	}

	return start;
}

void virtual_video(virtual_output *param, video_data *frame)
{
	if (!output_running)
		return;

	virtual_output *out_data = param;
	out_data->last_video_ts = frame->timestamp;

	shared_queue_push_video(&out_data->video_queue, frame->linesize, out_data->width, out_data->height, frame->data, frame->timestamp);

}
void virtual_output_destroy()
{
	output_running = false;
	leftCamIp = std::string("");
	rightCamIp = std::string("");
}

void SSPZCam::setCameraIps(std::string left, std::string right)
{
	leftCamIp = left;
	rightCamIp = right;
}

void SSPZCam::threadLooperStop()
{

	threadLooper->stop();
	delete leftCamClient;
	delete rightCamClient;
	leftCamClient = NULL;
	rightCamClient = NULL;
}
void SSPZCam::threadLooperStart()
{
	//create output
	virtual_output_create(ModeVideo);
	virtual_output_create(ModeVideo2);

	//start streaming
	threadLooper = std::unique_ptr<imf::ThreadLoop>(new imf::ThreadLoop(std::bind(&setup, _1)));
	threadLooper->start();
}






