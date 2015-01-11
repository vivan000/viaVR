#include "videoDecoder.h"

videoDecoder::videoDecoder () {
	width = 720;
	height = 1038;
	sarWidth = 1;
	sarHeight = 1;
	fpsNumerator = 30;
	fpsDenominator = 1;
	range = 2;
	matrix = 1;
	//fourCC = 0x41424752; // RGBA
	fourCC = 0x50343434; // P408
	//fourCC = 0x10003359; // P416

	decoderCount = 0;
	int dheight = height + 510;
/*
	// planar RGB (P416)
	dataR = new unsigned short[width * dheight];
	dataG = new unsigned short[width * dheight];
	dataB = new unsigned short[width * dheight];
	for (int y = 0; y < dheight; ++y)
		for (int x = 0; x < width; ++x) {
			dataR[width * y + x] = 32768;
			dataG[width * y + x] = x * 65535 / (width - 1);
			dataB[width * y + x] = 65535 - (y * 65535 / (height - 1));
		}
*/
	// planar RGB (P408)
	dataR = new unsigned char[width * dheight];
	dataG = new unsigned char[width * dheight];
	dataB = new unsigned char[width * dheight];
	for (int y = 0; y < dheight; ++y)
		for (int x = 0; x < width; ++x) {
			dataR[width * y + x] = 128;
			dataG[width * y + x] = x * 255 / (width - 1);
			dataB[width * y + x] = 255 - (y * 255 / (height - 1));
		}
/*
	for (int y = 0; y < dheight; ++y)
		for (int x = 0; x < width; ++x)
			if (y % 255 > 40) {
				dataR[width * y + x] = abs (x % 510 - 255);
				dataG[width * y + x] = abs (y % 510 - 255);
				dataB[width * y + x] = 0xFF;
			} else {
				dataR[width * y + x] = 0;
				dataG[width * y + x] = 0;
				dataB[width * y + x] = 0;
			}
*/
/*
 	// RGBA
	data = new unsigned int[width * dheight];
	for (int y = 0; y < dheight; ++y)
		for (int x = 0; x < width; ++x)
			if (y % 255 > 40)
				data[width * y + x] = (abs (x % 510 - 255) << 0) | (abs (y % 510 - 255) << 8) | (0xFF << 16) | (0x00 << 24);
			else data[width * y + x] = 0;
*/
}

videoDecoder::~videoDecoder () {
	delete[] dataR;
	delete[] dataG;
	delete[] dataB;
}

int videoDecoder::getNextVideoframe (char* buf, int size) {
	int shift = 0;//510 - (decoderCount * 10) % 510;
	int b = sizeof (*dataR);
	memcpy (buf + width * height * b * 0, dataR + width * b * shift, width * height * b);
	memcpy (buf + width * height * b * 1, dataG + width * b * shift, width * height * b);
	memcpy (buf + width * height * b * 2, dataB + width * b * shift, width * height * b);
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
