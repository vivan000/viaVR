#include "frames.h"

frameGPUu::frameGPUu (videoInfo* f) {
	numberOfPlanes = f->planes;
	plane = new GLuint[numberOfPlanes];
	glGenTextures (numberOfPlanes, plane);

	for (int i = 0; i < numberOfPlanes; i++) {
		glBindTexture (GL_TEXTURE_2D, plane[i]);
		glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		if (!i)
			glTexImage2D (GL_TEXTURE_2D, 0,	f->lumaFormat, f->width, f->height,
				0, f->lumaFormat, GL_UNSIGNED_BYTE, NULL);
		else
			glTexImage2D (GL_TEXTURE_2D, 0,	f->chromaFormat, f->chromaWidth, f->chromaHeight,
				0, f->chromaFormat, GL_UNSIGNED_BYTE, NULL);
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
