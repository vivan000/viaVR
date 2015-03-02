#include <math.h>
#include <stdlib.h>
#include "decoders/patternGenerator.h"

patternGenerator::patternGenerator () {
	width = 1280;
	height = 720;
	sarWidth = 1;
	sarHeight = 1;
	fpsNumerator = 30;
	fpsDenominator = 1;
	range = 1;
	matrix = 1;
	decoderCount = 0;

	mode = 5;

	switch (mode) {
		case 1:
			fourCC = (int) pFormat::RGBA;
			bpp = 4;
			break;
		case 2:
			fourCC = (int) pFormat::P408;
			bpp = 1;
			break;
		case 3:
			fourCC = (int) pFormat::P416;
			bpp = 2;
			break;
		case 4:
			fourCC = (int) pFormat::P008;
			bpp = 1;
			break;
		case 5:
			fourCC = (int) pFormat::P010;
			bpp = 2;
			break;
		case 6:
			fourCC = (int) pFormat::P416;
			bpp = 2;
			break;
	}

	info = new videoInfo (width, height, fourCC, range, matrix);

	int dheight = height + 720;
	switch (mode) {
		case 1:
			// RGBA
			dataR = new unsigned char[width * height * 4];
			for (int y = 0; y < height; ++y)
				for (int x = 0; x < width; ++x)
					((unsigned int*) dataR)[width * y + x] = ((x % 256) << 0) | ((y % 256) << 8) | (0xFF << 16) | (0x00 << 24);
			break;
		case 2:
			// planar 8-bit 4:4:4 (P408)
			dataR = new unsigned char[info->width * info->height * bpp];
			dataG = new unsigned char[info->chromaWidth * info->chromaHeight * bpp];
			dataB = new unsigned char[info->chromaWidth * info->chromaHeight * bpp];
			for (int y = 0; y < info->height; ++y)
				for (int x = 0; x < info->width; ++x) {
					dataR[width * y + x] = x * 255 / (info->width + y - 1) / 8 * 8;
					dataG[width * y + x] = 128; // x * 255 / (info->width - 1);
					dataB[width * y + x] = 128; // 255 - (y * 255 / (info->height - 1);
				}
			break;
		case 3:
			// planar 16-bit 4:4:4 (P416)
			dataR = new unsigned char[info->width * info->height * bpp];
			dataG = new unsigned char[info->chromaWidth * info->chromaHeight * bpp];
			dataB = new unsigned char[info->chromaWidth * info->chromaHeight * bpp];
			for (int y = 0; y < height; ++y)
				for (int x = 0; x < width; ++x) {
					((unsigned short*) dataR)[width * y + x] = 32768;
					((unsigned short*) dataG)[width * y + x] = x * 65535 / (width - 1);
					((unsigned short*) dataB)[width * y + x] = 65535 - (y * 65535 / (height - 1));
				}
			break;
		case 4:
			// planar 8-bit 4:2:0 (P008)
			dataR = new unsigned char[info->width * info->height * bpp];
			dataG = new unsigned char[info->chromaWidth * info->chromaHeight * bpp];
			dataB = new unsigned char[info->chromaWidth * info->chromaHeight * bpp];
			for (int y = 0; y < info->height; ++y)
				for (int x = 0; x < info->width; ++x) {
					int luma = 16;
					if (((x + y) % 20 == 1) || (abs (x - y) % 20 == 1) || ((x + y) % 20 == 19) || (abs (x - y) % 20 == 19))
						luma = 49;
					if (((x + y) % 20 == 0) || (abs (x - y) % 20 == 0))
						luma = 82;
					dataR[info->width * y + x] = luma;
				}
			for (int y = 0; y < info->chromaHeight; ++y)
				for (int x = 0; x < info->chromaWidth; ++x) {
					bool line = (((x * 2 + y * 2) % 20 == 0) || (abs (x * 2 - y * 2) % 20 == 0));
					dataG[info->chromaWidth * y + x] = line ? 90 : 128;
					dataB[info->chromaWidth * y + x] = line ? 240 : 128;
				}
			break;
		case 5:
			// planar 10-bit 4:2:0 (P010)
			dataR = new unsigned char[info->width * info->height * bpp];
			dataG = new unsigned char[info->chromaWidth * info->chromaHeight * bpp];
			dataB = new unsigned char[info->chromaWidth * info->chromaHeight * bpp];
			for (int y = 0; y < info->height; ++y)
				for (int x = 0; x < info->width; ++x) {
					int luma = 16*4;
					if (((x + y) % 20 == 1) || (abs (x - y) % 20 == 1) || ((x + y) % 20 == 19) || (abs (x - y) % 20 == 19))
						luma = 49*4;
					if (((x + y) % 20 == 0) || (abs (x - y) % 20 == 0))
						luma = 82*4;
					((unsigned short*) dataR)[info->width * y + x] = luma;
				}
			for (int y = 0; y < info->chromaHeight; ++y)
				for (int x = 0; x < info->chromaWidth; ++x) {
					bool line = (((x * 2 + y * 2) % 20 == 0) || (abs (x * 2 - y * 2) % 20 == 0));
					((unsigned short*) dataG)[info->chromaWidth * y + x] = line ? 90*4 : 128*4;
					((unsigned short*) dataB)[info->chromaWidth * y + x] = line ? 240*4 : 128*4;
				}
			break;
		case 6:
			// planar 16-bit 4:4:4 (P416), animated
			dataR = new unsigned char[info->width * dheight * bpp];
			dataG = new unsigned char[info->width * dheight * bpp];
			dataB = new unsigned char[info->width * dheight * bpp];
			for (int y = 0; y < dheight; ++y)
				for (int x = 0; x < info->width; ++x) {
					((unsigned short*) dataR)[info->width * y + x] = 32768;
					((unsigned short*) dataG)[info->width * y + x] = x * 65535 / (width - 1);
					((unsigned short*) dataB)[info->width * y + x] = 65535 - (y * 65535 / (720 - 1));
				}
			break;
	}
}

patternGenerator::~patternGenerator () {
	delete[] dataR;
	delete[] dataG;
	delete[] dataB;
	delete info;
}

int patternGenerator::getNextVideoframe (char* buf, int size) {
	int shift = (mode == 6 ? 720 - (decoderCount * 10) % 720 : 0);

	memcpy (buf, dataR, info->width * info->height * bpp);
	if (info->planes == 3) {
		memcpy (buf + info->offset1, dataG + info->chromaWidth * shift * bpp, info->chromaWidth * info->chromaHeight * bpp);
		memcpy (buf + info->offset2, dataB + info->chromaWidth * shift * bpp, info->chromaWidth * info->chromaHeight * bpp);
	}

	return decoderCount++ * 1000 * fpsDenominator / fpsNumerator;
}

void patternGenerator::seek (int timestamp) {
	decoderCount = timestamp;
}

int patternGenerator::getWidth () {
	return width;
}

int patternGenerator::getHeight () {
	return height;
}

int patternGenerator::getSarWidth () {
	return sarWidth;
}

int patternGenerator::getSarHeight () {
	return sarHeight;
}

int patternGenerator::getFpsNumerator () {
	return fpsNumerator;
}

int patternGenerator::getFpsDenominator () {
	return fpsDenominator;
}

int patternGenerator::getRange () {
	return range;
}

int patternGenerator::getMatrix () {
	return matrix;
}

int patternGenerator::getFourCC () {
	return fourCC;
}
