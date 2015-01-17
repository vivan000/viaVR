#include "frames.h"

frameGPUu::frameGPUu (videoInfo* f) {
	bool hwChroma = true;
	numberOfPlanes = f->planes;
	plane = new GLuint[numberOfPlanes];
	glGenTextures (numberOfPlanes, plane);

	for (int i = 0; i < numberOfPlanes; i++) {
		glBindTexture (GL_TEXTURE_2D, plane[i]);
		glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, hwChroma && i ? GL_LINEAR : GL_NEAREST);
		glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, hwChroma && i ? GL_LINEAR : GL_NEAREST);
		glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		if (!i)
			glTexImage2D (GL_TEXTURE_2D, 0,	f->internalLumaFormat, f->width, f->height,
				0, f->lumaFormat, f->lumaType, NULL);
		else
			glTexImage2D (GL_TEXTURE_2D, 0,	f->internalChromaFormat, f->chromaWidth, f->chromaHeight,
				0, f->chromaFormat, f->chromaType, NULL);
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
