#include "frames.h"

frameGPUi::frameGPUi (int w, int h, pFormat f) {
	glGenTextures (1, &plane);
	glBindTexture (GL_TEXTURE_2D, plane);
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexImage2D (GL_TEXTURE_2D, 0, chooseType (f), w, h, 0, GL_RGBA, chooseType (f), NULL);

	timecode = 0;
}

frameGPUi::~frameGPUi () {
	glDeleteTextures (1, &plane);
}

void frameGPUi::swap (frameGPUi& that) {
	std::swap (plane, that.plane);
	std::swap (timecode, that.timecode);
}

GLenum frameGPUi::chooseType (pFormat format) {
	switch (format) {
		case pFormat::HALF:
			return GL_HALF_FLOAT_OES;
		case pFormat::FULL:
			return GL_FLOAT;
	}
	return 0;
}
