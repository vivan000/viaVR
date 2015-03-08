/*
 * Copyright (c) 2015 Ivan Valiulin
 *
 * This file is part of viaVR.
 *
 * viaVR is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation, either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * viaVR is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with viaVR. If not, see http://www.gnu.org/licenses
 */

#include "frames/frames.h"

frameGPUu::frameGPUu (videoInfo* f, config* cfg) {
	numberOfPlanes = f->planes;
	plane = new GLuint[numberOfPlanes];
	glGenTextures (numberOfPlanes, plane);

	for (int i = 0; i < numberOfPlanes; i++) {
		glBindTexture (GL_TEXTURE_2D, plane[i]);
		glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
			f->halfWidth && cfg->hwChromaLinear && i ? GL_LINEAR : GL_NEAREST);
		glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,
			f->halfWidth && cfg->hwChromaLinear && i ? GL_LINEAR : GL_NEAREST);
		glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		if (!i)
			glTexStorage2D (GL_TEXTURE_2D, 1, f->internalLumaFormat, f->width, f->height);
		else
			glTexStorage2D (GL_TEXTURE_2D, 1, f->internalChromaFormat, f->chromaWidth, f->chromaHeight);
	}

	timecode = 0;
}

frameGPUu::~frameGPUu () {
	glDeleteTextures (numberOfPlanes, plane);
	delete[] plane;
}

void frameGPUu::swap (frameGPUu* that) {
	std::swap (plane, that->plane);
	std::swap (timecode, that->timecode);
}
