#include "frames.h"

frameGPUi::frameGPUi (int w, int h, pFormat type, videoInfo* f) {
	GLenum internalFormat, format;
	switch (type) {
		case pFormat::INT10:
			internalFormat = GL_RGB10_A2;
			format = GL_UNSIGNED_INT_2_10_10_10_REV;
			break;
		case pFormat::HALF_FLOAT:
			internalFormat = GL_RGBA16F;
			format = GL_HALF_FLOAT;
			break;
		case pFormat::FLOAT:
			internalFormat = GL_RGBA32F;
			format = GL_FLOAT;
			break;
	}
	glGenTextures (1, &plane);
	glBindTexture (GL_TEXTURE_2D, plane);
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, f->hwScaleLinear ? GL_LINEAR : GL_NEAREST);
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, f->hwScaleLinear ? GL_LINEAR : GL_NEAREST);
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexImage2D (GL_TEXTURE_2D, 0, internalFormat, w, h, 0, GL_RGBA, format, NULL);

	timecode = 0;
}

frameGPUi::~frameGPUi () {
	glDeleteTextures (1, &plane);
}
