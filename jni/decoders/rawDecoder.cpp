#include "decoders/rawDecoder.h"
#include <fstream>

rawDecoder::rawDecoder () {
	std::ifstream f ("/storage/emulated/0/video/test.y4m", std::ios::binary);

	f.seekg (0, std::ios::end);
	std::streampos fsize = f.tellg ();
	f.seekg (0, std::ios::beg);

	char str[100];
	f.getline (str, 100);

	int subsampling;
	int bitdepth = 8;

	sscanf (str, "YUV4MPEG2 W%i H%i F%i:%i Ip A%i:%i C%ip%i",
		&width, &height, &fpsNumerator, &fpsDenominator, &sarWidth, &sarHeight, &subsampling, &bitdepth);

	switch (subsampling) {
		case 420:
			switch (bitdepth) {
				case 16:
					fourCC = (int) pFormat::P016;
					break;
				case 10:
					fourCC = (int) pFormat::P010;
					break;
				case 8:
					fourCC = (int) pFormat::P008;
					break;
			}
			break;

		case 422:
			switch (bitdepth) {
				case 16:
					fourCC = (int) pFormat::P216;
					break;
				case 10:
					fourCC = (int) pFormat::P210;
					break;
				case 8:
					fourCC = (int) pFormat::P208;
					break;
			}
			break;

		case 444:
			switch (bitdepth) {
				case 16:
					fourCC = (int) pFormat::P416;
					break;
				case 10:
					fourCC = (int) pFormat::P410;
					break;
				case 8:
					fourCC = (int) pFormat::P408;
					break;
			}
			break;
	}

	range = 0;
	matrix = 0;
	info = new videoInfo (width, height, fourCC, range, matrix);

	if (!fpsNumerator)
		fpsNumerator = 30;
	if (!fpsDenominator)
		fpsDenominator = 1;
	if (!sarWidth)
		sarWidth = 1;
	if (!sarHeight)
		sarHeight = 1;

	bufsize = width * height * info->Bpp / 8;

	std::streampos fpos = f.tellg ();
	frames = (int) ((fsize - fpos) / (bufsize + 6));

	framesBuf = new char*[frames];
	for (int i = 0; i < frames; i++) {
		framesBuf[i] = new char[bufsize];

		f.getline (str, 100);
		f.read (framesBuf[i], bufsize);
	}

	f.close ();

	decoderCount = 0;
}


int rawDecoder::getNextVideoframe (char* buf, int size) {

	if (bufsize <= size)
		memcpy (buf, framesBuf[decoderCount % frames], bufsize);

	return decoderCount++ * 1000 * fpsDenominator / fpsNumerator;
}

rawDecoder::~rawDecoder () {
	for (int i = 0; i < frames; i++)
		delete framesBuf[i];
	delete framesBuf;
	delete info;
}

void rawDecoder::seek (int timestamp) {
	decoderCount = timestamp;
}

int rawDecoder::getWidth () {
	return width;
}

int rawDecoder::getHeight () {
	return height;
}

int rawDecoder::getSarWidth () {
	return sarWidth;
}

int rawDecoder::getSarHeight () {
	return sarHeight;
}

int rawDecoder::getFpsNumerator () {
	return fpsNumerator;
}

int rawDecoder::getFpsDenominator () {
	return fpsDenominator;
}

int rawDecoder::getRange () {
	return range;
}

int rawDecoder::getMatrix () {
	return matrix;
}

int rawDecoder::getFourCC () {
	return fourCC;
}
