#ifndef AVCODEC_H
#define AVCODEC_H
#pragma once 
extern "C"
{
#include <libavcodec/avcodec.h>
};

class CAVCodec
{
public:
	CAVCodec();
	static void CoInitialize();
	bool		InitCodec(bool bEncode = true);
	bool		FreeCodec();
	void		SetCodecID(AVCodecID id) { m_codecID = id; }
	AVCodecID		GetCodecID() { return m_codecID; }

public:
	AVCodec*	m_pCodec;
	AVCodecContext*	m_pCodecContext;

private:
	AVCodecID		m_codecID;
};

#endif // AVCODEC_H
