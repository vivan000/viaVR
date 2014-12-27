#include "frames.h"

frameGPUu::frameGPUu (int w, int h, pFormat f) {
	numberOfPlanes = chooseUPlanes (f);
	plane = new GLuint[numberOfPlanes];
	glGenTextures (numberOfPlanes, plane);

	for (int i = 0; i < numberOfPlanes; i++) {
		glBindTexture (GL_TEXTURE_2D, plane[i]);
		glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexImage2D (GL_TEXTURE_2D, 0,	chooseUFormat (f, !i),
			chooseUHalf (f, !i, true) ? w/2 : w, chooseUHalf (f, !i, false) ? h/2 : h,
			0, chooseUFormat (f, !i), GL_UNSIGNED_BYTE, NULL);
	}

	timecode = 0;
}

frameGPUu::~frameGPUu () {
	glDeleteTextures (numberOfPlanes, plane);
	delete[] plane;
}

void frameGPUu::swap (frameGPUu& that) {
	std::swap (plane, that.plane);
	std::swap (timecode, that.timecode);
}
