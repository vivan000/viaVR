#include "frames.h"

frameGPU::frameGPU (int w, int h, pFormat f) {
	GLenum pixelFormat = chooseFormat (f);
	GLenum pixelType = chooseType (f);

	glGenTextures (1, &plane);
	glBindTexture (GL_TEXTURE_2D, plane);
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexImage2D (GL_TEXTURE_2D, 0, pixelFormat, w, h, 0, pixelFormat, pixelType, NULL);

	timecode = 0;
}

frameGPU::~frameGPU () {
	glDeleteTextures (1, &plane);
}

void frameGPU::swap (frameGPU& that) {
	std::swap (plane, that.plane);
	std::swap (timecode, that.timecode);
}

GLenum frameGPU::chooseFormat (pFormat format) {
	switch (format) {
		case pFormat::RGBA:
			return GL_RGBA;
	}
	return 0;
}
GLenum frameGPU::chooseType (pFormat format) {
	switch (format) {
		case pFormat::RGBA:
			return GL_UNSIGNED_BYTE;
	}
	return 0;
}
