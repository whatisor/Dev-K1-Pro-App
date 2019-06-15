#pragma once
#ifndef SSPZCAM_H
#define SSPZCAM_H
#include <thread>
#include <iostream>
#include <chrono>
#include <queue>
#include <mutex>
#include <vector>
#include <condition_variable>
#include <imf/net/loop.h>
#include <imf/net/threadloop.h>
#include <imf/ssp/sspclient.h>
//think of using pthread...
#include "queue/share_queue.h"
#include "queue/share_queue_write.h"
#include "CommonUtils.hpp"

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
using namespace std::chrono_literals;
using std::vector;
using std::thread;
using std::unique_lock;
using std::mutex;
using std::condition_variable;
using std::queue;
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

//In-out shared queue
class WorkQueue
{
	condition_variable work_available;
	mutex work_mutex;
	queue<struct imf::SspH264Data> work;

public:
	void push_work(imf::SspH264Data item)
	{
		unique_lock<mutex> lock(work_mutex);

		bool was_empty = work.empty();
		work.push(item);

		lock.unlock();

		if (was_empty)
		{
			work_available.notify_one();
		}
	}

	struct imf::SspH264Data wait_and_pop()
	{
		unique_lock<mutex> lock(work_mutex);
		if (work.empty())
		{
			work_available.wait(lock);
		}

		imf::SspH264Data tmp = work.front();
		work.pop();
		while (work.size()> 1)//delay max 1 frame
		{
			tmp = work.front();
			work.pop();
		}
		return tmp;
	}
};
//libssp related functions
std::unique_ptr<imf::ThreadLoop> threadLooper;
imf::SspClient *leftCamClient = NULL;
imf::SspClient *rightCamClient = NULL;
std::string leftCamIp;
std::string rightCamIp;

WorkQueue leftQueue, rightQueue;
std::thread* consummerLeft, *consummerRight;

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
