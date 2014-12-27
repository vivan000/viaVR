#include "frames.h"

frameCPU::frameCPU (int w, int h, pFormat f) {
	plane = new char[w * h * chooseBpp (f) / 8];

	timecode = 0;
}

frameCPU::~frameCPU () {
	delete[] plane;
}

void frameCPU::swap (frameCPU& that) {
	std::swap (plane, that.plane);
	std::swap (timecode, that.timecode);
}
