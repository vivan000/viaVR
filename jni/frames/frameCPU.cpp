#include "frames.h"

frameCPU::frameCPU (videoInfo* f) {
	plane = new char[f->width * f->height * f->Bpp / 8];

	timecode = 0;
}

frameCPU::~frameCPU () {
	delete[] plane;
}

void frameCPU::swap (frameCPU& that) {
	std::swap (plane, that.plane);
	std::swap (timecode, that.timecode);
}
