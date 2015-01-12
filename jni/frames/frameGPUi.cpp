#include "frames.h"

frameGPUi::frameGPUi (int w, int h, bool full) {
	glGenTextures (1, &plane);
	glBindTexture (GL_TEXTURE_2D, plane);
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexImage2D (GL_TEXTURE_2D, 0, full ? GL_RGBA32F : GL_RGBA16F, w, h, 0, GL_RGBA, full ? GL_FLOAT : GL_HALF_FLOAT, NULL);

	timecode = 0;
}

frameGPUi::~frameGPUi () {
	glDeleteTextures (1, &plane);
}
