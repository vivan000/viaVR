#pragma once
#include "interface/IVideoDecoder.h"
#include "videoInfo.h"

class patternGenerator: public IVideoDecoder {
public:
	patternGenerator ();
	~patternGenerator ();

	int getNextVideoframe (char* buf, int size);
	void seek (int timestamp);

	int getWidth ();
	int getHeight ();

	int getSarWidth ();
	int getSarHeight ();

	int getFpsNumerator ();
	int getFpsDenominator ();

	int getRange ();
	int getMatrix ();
	int getFourCC ();

private:
	videoInfo* info;

	int width, height;
	int sarWidth, sarHeight;
	int fpsNumerator, fpsDenominator;
	int range, matrix, fourCC;
	int bpp;

	int mode, decoderCount;

	unsigned char* dataR;
	unsigned char* dataG;
	unsigned char* dataB;
};
