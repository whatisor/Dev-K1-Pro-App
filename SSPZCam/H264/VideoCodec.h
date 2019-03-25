#pragma once

#include <stdio.h>
#include "codec_def.h"
#include "codec_app_def.h"
#include "codec_api.h"
#include "typedefs.h"
#include "d3d9_utils.h"
#include "measure_time.h"

#define SAFE_RELEASE(p)      { if(p) { (p)->Release(); (p)=NULL; } }

//////////////////////////////////////////////////////////////////////

#define SCALEBITS 12
#define ONE_HALF  (1UL << (SCALEBITS - 1))
#define FIX(x)    ((int) ((x) * (1UL<<SCALEBITS) + 0.5))
#define LIMIT(x) (unsigned char) ((x > 255) ? 255 : ((x < 0) ? 0 : x ))
#define PARRAYSIZE(array) ((int)(sizeof(array)/sizeof(array[0])))

#define RGB2Y(r, g, b, y) \
    y=(unsigned char)(((int)257*(r)  +(int)504*(g) +(int)98*(b))/1000)

#define RGB2YUV(r, g, b, y, cb, cr) \
    RGB2Y(r, g, b, y); \
    cb=(unsigned char)((-148*(r)  -291*(g) +439*(b))/1000 + 128); \
    cr=(unsigned char)(( 439*(r)  -368*(g) - 71*(b))/1000 + 128)
//////////////////////////////////////////////////////////////////////////

#include "welsDecoderExt.h"

using namespace WelsDec;

 inline void YUY2toYUV420P(const unsigned char *yuy2, unsigned char *yuv420p, int width, int height)
{
    const unsigned char *s;
    unsigned char *y, *u, *v;
    unsigned int x, h;
    int npixels = width * height;

    s = yuy2;
    y = yuv420p;
    u = yuv420p + npixels;
    v = u + npixels/4;

    for (h=0; h<(unsigned int)height; h+=2) {
        /* Copy the first line keeping all information */
        for (x=0; x<(unsigned int)width; x+=2) {
            *y++ = *s++;
            *u++ = *s++;
            *y++ = *s++;
            *v++ = *s++;
        }
        /* Copy the second line discarding u and v information */
        for (x=0; x<(unsigned int)width; x+=2) {
            *y++ = *s++;
            s++;
            *y++ = *s++;
            s++;
        }
    }
}

 inline void YUV420PtoRGB(unsigned char* dst, unsigned char* src, int width, int height, bool bVFlip, bool bSwapRGB )
{
    int rgbIncrement=3;
    int redOffset=0;
    int blueOffset=2;

    unsigned char *dstImageFrame;
    unsigned int   nbytes    = width*height;
    const unsigned char *yplane    = src;               // 1 byte Y (luminance) for each pixel
    const unsigned char *uplane    = yplane+nbytes;                // 1 byte U for a block of 4 pixels
    const unsigned char *vplane    = uplane+(nbytes/4);            // 1 byte V for a block of 4 pixels

    unsigned int   pixpos[4] = {0, 1, width, width + 1};
    unsigned int   originalPixpos[4] = {0, 1, width, width + 1};

    unsigned int   x, y, p;

    long     int   yvalue;
    long     int   l, r, g, b;

    if (bVFlip) {
        dstImageFrame = dst + ((height - 2) * width * rgbIncrement);
        pixpos[0] = width;
        pixpos[1] = width +1;
        pixpos[2] = 0;
        pixpos[3] = 1;
    }
    else
        dstImageFrame = dst;

    for (y = 0; y < (unsigned int)height; y += 2)
    {
        for (x = 0; x < (unsigned int)width; x += 2)
        {
            // The RGB value without luminance
            long cb = *uplane-128;
            long cr = *vplane-128;
            long rd = FIX(1.40200) * cr + ONE_HALF;
            long gd = -FIX(0.34414) * cb -FIX(0.71414) * cr + ONE_HALF;
            long bd = FIX(1.77200) * cb + ONE_HALF;

            // Add luminance to each of the 4 pixels

            for (p = 0; p < 4; p++)
            {
                yvalue = *(yplane + originalPixpos[p]);

                l = yvalue << SCALEBITS;

                if( bSwapRGB )
                {
                    b = (l+rd)>>SCALEBITS;
                    g = (l+gd)>>SCALEBITS;
                    r = (l+bd)>>SCALEBITS;
                }
                else
                {
                    r = (l+rd)>>SCALEBITS;
                    g = (l+gd)>>SCALEBITS;
                    b = (l+bd)>>SCALEBITS;
                }

                unsigned char *rgpPtr = dstImageFrame + rgbIncrement*pixpos[p];
                rgpPtr[redOffset] = LIMIT(r);
                rgpPtr[1] = LIMIT(g);
                rgpPtr[blueOffset] = LIMIT(b);
                if (rgbIncrement == 4)
                    rgpPtr[3] = 0;
            }

            yplane += 2;
            dstImageFrame += rgbIncrement*2;

            uplane++;
            vplane++;
        }

        yplane += width;
        if (bVFlip)
            dstImageFrame -= 3*rgbIncrement*width;
        else
            dstImageFrame += rgbIncrement*width;
    }
}

inline void YUY2toRGB(unsigned char *dst, unsigned char *src, int width, int height, bool bVFlip, bool bSwapRGB )
{
   unsigned char *yuv = new unsigned char[width*height*3/2];
   YUY2toYUV420P(src, yuv, width, height);
   YUV420PtoRGB(dst, yuv, width, height, bVFlip, bSwapRGB);
   delete[] yuv;
   return;
}

inline void RGB2YUV420P(unsigned char* yuv,
                       unsigned char* rgb,
                       int srcFrameWidth,
                       int srcFrameHeight,
                       bool verticalFlip)
{
   int rgbIncrement = 3;
   int redOffset	= 0;
   int blueOffset	= 2;

   int planeSize = srcFrameWidth*srcFrameHeight;
   int halfWidth = srcFrameWidth >> 1;

   // get pointers to the data
   unsigned char* yplane  = yuv;
   unsigned char* uplane  = yuv + planeSize;
   unsigned char* vplane  = yuv + planeSize + (planeSize >> 2);
   const unsigned char* rgbIndex = rgb;

   for (int y = 0; y < srcFrameHeight; y++) {
       unsigned char* yline  = yplane + (y * srcFrameWidth);
       unsigned char* uline  = uplane + ((y >> 1) * halfWidth);
       unsigned char* vline  = vplane + ((y >> 1) * halfWidth);

       if (verticalFlip)
           rgbIndex = rgb + (srcFrameWidth*(srcFrameHeight-1-y)*rgbIncrement);

       for (int x = 0; x < srcFrameWidth; x+=2) {
           RGB2Y(rgbIndex[redOffset], rgbIndex[1], rgbIndex[blueOffset], *yline);
           rgbIndex += rgbIncrement;
           yline++;
           RGB2YUV(rgbIndex[redOffset], rgbIndex[1], rgbIndex[blueOffset], *yline, *uline, *vline);
           rgbIndex += rgbIncrement;
           yline++;
           uline++;
           vline++;
       }
   }
}

class CVideoCodec
{
public:
	CVideoCodec(void);
	~CVideoCodec(void);
	bool init(int nWidth, int nHeight, int fps);
private:
	int m_width,m_height;

protected:
	ISVCEncoder* m_encoder;
	SEncParamBase		m_encParam;
	SFrameBSInfo		m_frameInfo;
public:
	bool encodeFrame(unsigned char* pYUVData);
	unsigned char* m_pOutBuffEnc;
	int			   m_nOutBuffEnc;

protected:
	ISVCDecoder* m_decoder;
	SDecodingParam m_decParam;
public:
	bool decodeFrame(unsigned char* pData, int len);
	unsigned char* m_pOutBuffDec;
	int			   m_nOutBuffDec;
};

