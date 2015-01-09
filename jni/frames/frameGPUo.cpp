#include "frames.h"

frameGPUo::frameGPUo (videoInfo* f) {
	glGenTextures (1, &plane);
	glBindTexture (GL_TEXTURE_2D, plane);
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexImage2D (GL_TEXTURE_2D, 0, GL_RGBA, f->width, f->height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);

	timecode = 0;
}

frameGPUo::~frameGPUo () {
	glDeleteTextures (1, &plane);
}

void frameGPUo::swap (frameGPUo& that) {
	std::swap (plane, that.plane);
	std::swap (timecode, that.timecode);
}
