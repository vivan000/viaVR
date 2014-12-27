#include "videoDecoder.h"

videoDecoder::videoDecoder () {
	width = 720;
	height = 1038;
	sarWidth = 1;
	sarHeight = 1;
	fpsNumerator = 30;
	fpsDenominator = 1;
	range = 0;
	matrix = 0;
	fourCC = 0x41424752;

	decoderCount = 0;
	int dheight = height + 510;

	data = new unsigned int[width * dheight];
	for (int y = 0; y < dheight; ++y)
		for (int x = 0; x < width; ++x)
			if (y % 255 > 40)
				data[width * y + x] = (abs (x % 510 - 255) << 0) | (abs (y % 510 - 255) << 8) | (0xFF << 16) | (0x00 << 24);
			else data[width * y + x] = 0;
}

videoDecoder::~videoDecoder () {
	delete[] data;
}

int videoDecoder::getNextVideoframe (char* buf, int size) {
	int shift = 510 - (decoderCount * 10) % 510;
	memcpy (buf, data + width * shift, 4 * width * height);
	return decoderCount++ * 1000 * fpsDenominator / fpsNumerator;
}

int videoDecoder::getWidth () {
	return width;
}

int videoDecoder::getHeight () {
	return height;
}

int videoDecoder::getSarWidth () {
	return sarWidth;
}

int videoDecoder::getSarHeight () {
	return sarHeight;
}

int videoDecoder::getFpsNumerator () {
	return fpsNumerator;
}

int videoDecoder::getFpsDenominator () {
	return fpsDenominator;
}

int videoDecoder::getRange () {
	return range;
}

int videoDecoder::getMatrix () {
	return matrix;
}

int videoDecoder::getFourCC () {
	return fourCC;
}

int videoDecoder::getCurrentTimecode () {
	return decoderCount++ * 1000 * fpsDenominator / fpsNumerator;
}
