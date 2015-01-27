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

#include "frames.h"

frameGPUi::frameGPUi (int w, int h, pFormat type, videoInfo* f) : width (w), height (h) {
	GLenum internalFormat, format;
	bool repeat = false;
	switch (type) {
		case pFormat::INT8:
			internalFormat = GL_RGBA8;
			format = GL_UNSIGNED_BYTE;
			break;
		case pFormat::INT10:
			internalFormat = GL_RGB10_A2;
			format = GL_UNSIGNED_INT_2_10_10_10_REV;
			break;
		case pFormat::FLOAT16:
			internalFormat = GL_RGBA16F;
			format = GL_HALF_FLOAT;
			break;
		case pFormat::FLOAT32:
			internalFormat = GL_RGBA32F;
			format = GL_FLOAT;
			break;
		case pFormat::DITHER:
			internalFormat = GL_RGB10_A2;
			format = GL_UNSIGNED_INT_2_10_10_10_REV;
			repeat = true;
			break;
	}
	glGenTextures (1, &plane);
	glBindTexture (GL_TEXTURE_2D, plane);
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, f->hwScaleLinear ? GL_LINEAR : GL_NEAREST);
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, f->hwScaleLinear ? GL_LINEAR : GL_NEAREST);
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, repeat ? GL_REPEAT : GL_CLAMP_TO_EDGE);
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, repeat ? GL_REPEAT : GL_CLAMP_TO_EDGE);
	glTexImage2D (GL_TEXTURE_2D, 0, internalFormat, w, h, 0, GL_RGBA, format, NULL);

	timecode = 0;
}

frameGPUi::~frameGPUi () {
	glDeleteTextures (1, &plane);
}
