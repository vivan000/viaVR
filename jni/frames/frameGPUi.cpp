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

frameGPUi::frameGPUi (int w, int h, iFormat type, bool linear) : width (w), height (h) {
	GLenum internalFormat, format;
	GLenum repeat = GL_CLAMP_TO_EDGE;
	switch (type) {
		case iFormat::INT8:
			internalFormat = GL_RGBA8;
			format = GL_UNSIGNED_BYTE;
			break;
		case iFormat::INT10:
			internalFormat = GL_RGB10_A2;
			format = GL_UNSIGNED_INT_2_10_10_10_REV;
			break;
		case iFormat::FLOAT16:
			internalFormat = GL_RGBA16F;
			format = GL_HALF_FLOAT;
			break;
		case iFormat::FLOAT32:
			internalFormat = GL_RGBA32F;
			format = GL_FLOAT;
			break;
		case iFormat::DITHER:
			internalFormat = GL_RGB10_A2;
			format = GL_UNSIGNED_INT_2_10_10_10_REV;
			repeat = GL_REPEAT;
			break;
	}
	glGenTextures (1, &plane);
	glBindTexture (GL_TEXTURE_2D, plane);
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, linear ? GL_LINEAR : GL_NEAREST);
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, linear ? GL_LINEAR : GL_NEAREST);
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, repeat);
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, repeat);
	glTexStorage2D (GL_TEXTURE_2D, 1, internalFormat, w, h);

	timecode = 0;
}

frameGPUi::~frameGPUi () {
	glDeleteTextures (1, &plane);
}
