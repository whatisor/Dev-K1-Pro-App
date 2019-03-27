#ifndef VIDEOCODEC_H
#define VIDEOCODEC_H
extern "C"
{
#include <libavutil/pixfmt.h>
#include <libavcodec/avcodec.h>
};

#include "AVCodec.h"

class CVideoCodec : public CAVCodec
{
public:
	CVideoCodec();
	~CVideoCodec();

	void	SetCodecParam(int bitRate, int width, int height, int gopSize, int bframePeriod, AVPixelFormat fmt);

	bool	InitDecode();
	bool	Decode(const uint8_t* pEncBuf, int encBufLen, uint8_t** ppDecBuf, int* pDecBufLen);

	bool	FreeVideoCodec();

	AVFrame*	m_pPicture;

private:
	AVPacket	m_packet;
	int			m_outBufSize;
	uint8_t*	m_outBuf;
	int			m_bitRate;
	int			m_width;
	int			m_height;
	int			m_gopSize;
	int			m_bframePeriod;
	AVPixelFormat	m_pixFormat;
};

#endif