#include <string>
#include <iostream>
#include "VideoCodec.h"

using namespace std;

CVideoCodec::CVideoCodec()
{
	m_pPicture = 0;
	m_outBuf = 0;
}

CVideoCodec::~CVideoCodec()
{
	if (m_pPicture)
		delete m_pPicture;
	if (m_outBuf)
		delete m_outBuf;
}

void CVideoCodec::SetCodecParam(int bitRate, int width, int height, int gopSize, int bframePeriod, AVPixelFormat fmt)
{
	m_bitRate = bitRate;
	m_width = width;
	m_height = height;
	m_gopSize = gopSize;
	m_bframePeriod = bframePeriod;
	m_pixFormat = fmt;
}

bool CVideoCodec::InitDecode()
{
	if (!InitCodec(false))
		return false;

	m_pPicture = av_frame_alloc();
	
	/* open it */
	if (avcodec_open2(m_pCodecContext, m_pCodec, 0) < 0) {
		return false;
	}

	return true;
}

bool CVideoCodec::Decode(const uint8_t* pEncBuf, int encBufLen, uint8_t** ppDecBuf, int* pDecBufLen)
{
	m_packet.data = (uint8_t*)malloc(encBufLen);
	memset(m_packet.data, '\0', encBufLen);
	memcpy(m_packet.data, pEncBuf, encBufLen);
	m_packet.size = encBufLen;

	int got_picture = 0;
	int len = avcodec_decode_video2(m_pCodecContext, m_pPicture, &got_picture, &m_packet);

	if (len < 0 || !got_picture)
	{
		free(m_packet.data);
		return false;
	}

	ppDecBuf[0] = m_pPicture->data[0];
	ppDecBuf[1] = m_pPicture->data[1];
	ppDecBuf[2] = m_pPicture->data[2];
	/*
	pDecBufLen[0] = m_pPicture->width;
	pDecBufLen[1] = m_pPicture->width / 2;
	pDecBufLen[2] = m_pPicture->width / 2;
	*/
	pDecBufLen[0] = m_pPicture->linesize[0];
	pDecBufLen[1] = m_pPicture->linesize[1];
	pDecBufLen[2] = m_pPicture->linesize[2];
	//printf("decoded framesize - %d, %d, %d, %d, %d, %d \n",m_pPicture->width , m_pPicture->height, m_pPicture->linesize[0], m_pPicture->linesize[1],m_pPicture->linesize[2], m_pPicture->linesize[0] * m_pPicture->height + m_pPicture->linesize[1] * m_pPicture->height / 2 + m_pPicture->linesize[2] * m_pPicture->height / 2);
	free(m_packet.data);
	

	return true;
}

bool CVideoCodec::FreeVideoCodec()
{
	if (!FreeCodec())
		return false;

	av_frame_free(&m_pPicture);

	return true;
}
