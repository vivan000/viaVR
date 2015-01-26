#include "videoDecoder.h"
#include "videoInfo.h"
#include <math.h>

videoDecoder::videoDecoder () {
	width = 720;
	height = 720; // 1038;
	sarWidth = 1;
	sarHeight = 1;
	fpsNumerator = 30;
	fpsDenominator = 1;
	range = 1;
	matrix = 1;
	decoderCount = 0;

	int mode = 2;

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
	}

	videoInfo* info = new videoInfo (width, height, fourCC, range, matrix);

	int dheight = height + 510;
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
					dataR[width * y + x] = x * 255 / (info->width + y - 1) / 4 * 4;
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
					int luma = 16*256;
					if (((x + y) % 20 == 1) || (abs (x - y) % 20 == 1) || ((x + y) % 20 == 19) || (abs (x - y) % 20 == 19))
						luma = 49*256;
					if (((x + y) % 20 == 0) || (abs (x - y) % 20 == 0))
						luma = 82*256;
					((unsigned short*) dataR)[info->width * y + x] = luma;
				}
			for (int y = 0; y < info->chromaHeight; ++y)
				for (int x = 0; x < info->chromaWidth; ++x) {
					bool line = (((x * 2 + y * 2) % 20 == 0) || (abs (x * 2 - y * 2) % 20 == 0));
					((unsigned short*) dataG)[info->chromaWidth * y + x] = line ? 90*256 : 128*256;
					((unsigned short*) dataB)[info->chromaWidth * y + x] = line ? 240*256 : 128*256;
				}
			break;
	}
	infoptr = (void*) info;
}

videoDecoder::~videoDecoder () {
	delete[] dataR;
	delete[] dataG;
	delete[] dataB;
	delete (videoInfo*)infoptr;
}

int videoDecoder::getNextVideoframe (char* buf, int size) {
	int shift = 0;//510 - (decoderCount * 10) % 510;
	videoInfo* info = (videoInfo*) infoptr;

	memcpy (buf, dataR, info->width * info->height * bpp);
	if (info->planes == 3) {
		memcpy (buf + info->offset1, dataG, info->chromaWidth * info->chromaHeight * bpp);
		memcpy (buf + info->offset2, dataB, info->chromaWidth * info->chromaHeight * bpp);
	}

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
	return decoderCount * 1000 * fpsDenominator / fpsNumerator;
}
