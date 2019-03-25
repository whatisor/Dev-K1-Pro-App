#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "VideoCodec.h"

#define VBUF_SIZE 4915200

CVideoCodec::CVideoCodec(void)
{
	m_pOutBuffEnc = new unsigned char[VBUF_SIZE];
	m_nOutBuffEnc = 0;

	m_pOutBuffDec = new unsigned char[VBUF_SIZE];
	m_nOutBuffEnc = 0;

	m_width = 0;
	m_height = 0;

	m_encoder = 0;
	m_decoder = 0;
}


CVideoCodec::~CVideoCodec(void)
{
	delete m_pOutBuffDec;
}

bool CVideoCodec::init(int nWidth, int nHeight, int fps)
{
	m_width = nWidth;
	m_height = nHeight;

	WelsCreateSVCEncoder(&m_encoder);
	if (!m_encoder) return 0;
	printf("created video codec h264 encode successfully.");

	WelsCreateDecoder(&m_decoder);
	if (!m_decoder) return 0;
	printf("created video codec h264 decode successfully.");

	long lRet;
	memset(&m_encParam, 0, sizeof(SEncParamBase));
	m_encParam.iUsageType = CAMERA_VIDEO_REAL_TIME;
	m_encParam.fMaxFrameRate = fps;
	m_encParam.iPicWidth = nWidth;
	m_encParam.iPicHeight = nHeight;
	m_encParam.iTargetBitrate = 320000;
	lRet = m_encoder->Initialize(&m_encParam);
	if (lRet != 0)return false;

	memset(&m_decParam, 0, sizeof(SDecodingParam));
	m_decParam.uiTargetDqLayer = UCHAR_MAX;
	m_decParam.eEcActiveIdc = ERROR_CON_SLICE_COPY;
	m_decParam.sVideoProperty.eVideoBsType = VIDEO_BITSTREAM_DEFAULT;
	lRet=m_decoder->Initialize(&m_decParam);
	if(lRet!=0)return false;

	return true;
}

bool CVideoCodec::encodeFrame(unsigned char* pYUVData)
{
	memset(&m_frameInfo, 0, sizeof(SFrameBSInfo));

	SSourcePicture pic;
	memset(&pic, 0, sizeof(SSourcePicture));
	pic.iPicWidth = m_encParam.iPicWidth;
	pic.iPicHeight = m_encParam.iPicHeight;
	pic.iColorFormat = videoFormatI420;
	pic.iStride[0] = m_encParam.iPicWidth;
	pic.iStride[1] = pic.iStride[2] = m_encParam.iPicWidth >> 1;
	pic.pData[0] = pYUVData;
	pic.pData[1] = pic.pData[0] + m_encParam.iPicWidth * m_encParam.iPicHeight;
	pic.pData[2] = pic.pData[1] + (m_encParam.iPicWidth * m_encParam.iPicHeight >> 2);

	int nRet = m_encoder->EncodeFrame(&pic, &m_frameInfo);
	if (nRet != cmResultSuccess)return false;
	if (m_frameInfo.eFrameType == videoFrameTypeSkip) return false;

	memcpy(m_pOutBuffEnc, m_frameInfo.sLayerInfo[0].pBsBuf, m_frameInfo.iFrameSizeInBytes);
	m_nOutBuffEnc = m_frameInfo.iFrameSizeInBytes;
	return true;
}

bool CVideoCodec::decodeFrame(unsigned char* pData, int len)
{
	unsigned char* data[3] = {0};
	SBufferInfo bufInfo={0};

	DECODING_STATE rv = m_decoder->DecodeFrame2 (pData, len, data, &bufInfo);
	if(rv != dsErrorFree)
		return false;
	if(bufInfo.iBufferStatus != 1)return false;

	int planeSize = m_width * m_height;
	int halfWidth = m_width >> 1;

	for( int y = 0; y < m_height; y++ ){
		memcpy( m_pOutBuffDec + y*m_width, data[0] + y*bufInfo.UsrData.sSystemBuffer.iStride[0], m_width );
	}
	for( int y = 0; y < m_height/2; y++ ){
		memcpy( m_pOutBuffDec + planeSize + y*halfWidth, data[1] + y*bufInfo.UsrData.sSystemBuffer.iStride[1], halfWidth );
	}
	for( int y = 0; y < m_height/2; y++ ){
		memcpy( m_pOutBuffDec + planeSize + (planeSize >> 2) + y*halfWidth, data[2] + y*bufInfo.UsrData.sSystemBuffer.iStride[2], halfWidth );
	}
	
	m_nOutBuffDec = m_height * m_width * 3 / 2;

	return true;
}
