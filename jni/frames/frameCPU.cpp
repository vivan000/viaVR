#include "frames.h"

frameCPU::frameCPU (int w, int h, pFormat f) {
	plane = new char[w * h * chooseFormat (f) / 8];

	timecode = 0;
}

frameCPU::~frameCPU () {
	delete[] plane;
}

void frameCPU::swap (frameCPU& that) {
	std::swap (plane, that.plane);
	std::swap (timecode, that.timecode);
}

int frameCPU::chooseFormat (pFormat format) {
	switch (format) {
		case pFormat::P008:
		case pFormat::NV12:
		case pFormat::NV21:
			return 12;

		case pFormat::P208:
			return 16;

		case pFormat::P408:
		case pFormat::P010:
		case pFormat::P016:
			return 24;

		case pFormat::P210:
		case pFormat::P216:
		case pFormat::RGBA:
			return 32;

		case pFormat::P410:
		case pFormat::P416:
			return 48;
	}
	return 0;
}
