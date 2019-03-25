#pragma once
#ifndef SSPZCAM_H
#define SSPZCAM_H

#include <imf/net/loop.h>
#include <imf/net/threadloop.h>
#include <imf/ssp/sspclient.h>
//think of using pthread...
#include "queue/share_queue.h"
#include "queue/share_queue_write.h"

#define DEFAULT_DELAY (0)
#define DEFAULT_WIDTH (1280)
#define DEFAULT_HEIGHT (1280)
#define DEFAULT_FPS (30)
#define MAX_AV_PLANES 8

#ifdef SSPZCAMLIBRARY_EXPORTS
#define SSPZCAMAPI __declspec(dllexport)
#else
#define SSPZCAMAPI __declspec(dllimport)
#endif

typedef struct virtual_output {
	share_queue video_queue;
	int width = 0;
	int height = 0;
	int delay = DEFAULT_DELAY;
	int video_mode = 0;
	int64_t last_video_ts = 0;
	bool keep_ratio;
}virtual_output;

virtual_output *leftOutput, *rightOutput;

typedef struct video_data {
	uint8_t           *data[MAX_AV_PLANES];
	uint32_t          linesize[MAX_AV_PLANES];
	uint64_t          timestamp;
}video_data;

class SSPZCAMAPI SSPZCam 
{
public:
	SSPZCam(std::string ip1, std::string ip2);
	~SSPZCam();
	void threadLooperStart();
	void threadLooperStop();
	void setCameraIps(std::string left, std::string right);
private:
};
//libssp related functions
std::unique_ptr<imf::ThreadLoop> threadLooper;
imf::SspClient *leftCamClient = NULL;
imf::SspClient *rightCamClient = NULL;
std::string leftCamIp;
std::string rightCamIp;
bool output_running = false;
void on_264_1(struct imf::SspH264Data * h264);
void on_264_2(struct imf::SspH264Data * h264);
void on_audio_data_1(struct imf::SspAudioData * audio);
void on_audio_data_2(struct imf::SspAudioData * audio);
void on_meta_1(struct imf::SspVideoMeta *v, struct imf::SspAudioMeta *a, struct imf::SspMeta * m);
void on_meta_2(struct imf::SspVideoMeta *v, struct imf::SspAudioMeta *a, struct imf::SspMeta * m);
void on_disconnect();
void setup(imf::Loop *loop);

//virtual output related functions
void virtual_output_create(int videoMode);
bool virtual_output_start(virtual_output *data);
void virtual_video(virtual_output *data , video_data *frame);
#endif // SSPZCAM_H
