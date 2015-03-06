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

frameGPUo::frameGPUo (videoInfo* f, config* cfg) :
	width (cfg->hwScale ? f->width : cfg->targetWidth),
	height (cfg->hwScale ? f->height : cfg->targetHeight) {
	GLenum internalFormat;
	switch (cfg->internalType) {
		case iFormat::INT8:
			internalFormat = GL_RGBA8;
			break;
		case iFormat::INT10:
			internalFormat = GL_RGB10_A2;
			break;
		case iFormat::FLOAT16:
			internalFormat = GL_RGBA16F;
			break;
		case iFormat::FLOAT32:
			internalFormat = GL_RGBA32F;
			break;
	}
	GLint filter = (width != cfg->targetWidth || height != cfg->targetHeight) && cfg->hwScaleLinear ? GL_LINEAR : GL_NEAREST;

	glGenTextures (1, &plane);
	glBindTexture (GL_TEXTURE_2D, plane);
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter);
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter);
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexStorage2D (GL_TEXTURE_2D, 1, internalFormat, width, height);

	timecode = 0;
}

frameGPUo::~frameGPUo () {
	glDeleteTextures (1, &plane);
}

void frameGPUo::swap (frameGPUo& that) {
	std::swap (plane, that.plane);
	std::swap (timecode, that.timecode);
}
