
#include "AVCodec.h"

CAVCodec::CAVCodec()
{
	m_pCodec = NULL;
	m_pCodecContext = NULL;

	CoInitialize();
}

void CAVCodec::CoInitialize()
{
	/* must be called before using avcodec lib */
//	avcodec_init();
	/* register all the codecs */
	avcodec_register_all();
}

bool CAVCodec::InitCodec(bool bEncode)
{
	m_pCodec = bEncode ? avcodec_find_encoder(m_codecID) : avcodec_find_decoder(m_codecID);
	if (!m_pCodec) {
		printf("InitiCodec Function Fail\n");
		return false;
	}

	m_pCodecContext = avcodec_alloc_context3(m_pCodec);
	if (m_pCodec->capabilities&CODEC_CAP_TRUNCATED)
		m_pCodecContext->flags |= CODEC_FLAG_TRUNCATED;

	return true;
}

bool CAVCodec::FreeCodec()
{
	if (!m_pCodecContext)
		return false;
	avcodec_close(m_pCodecContext);
	av_free(m_pCodecContext);
	return true;
}
