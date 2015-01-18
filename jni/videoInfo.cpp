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

#include "videoInfo.h"

videoInfo::videoInfo (int videoWidth, int videoHeight, int videoFourCC, int videoRange, int videoMatrix) {
	width = videoWidth;
	height = videoHeight;
	init = true;

	// range
	switch (videoRange) {
		case (int) pRange::UNKNOWN:
		case (int) pRange::TV:
			range = pRange::TV;
			break;

		case (int) pRange::PC:
			range = pRange::PC;
			break;

		default:
			init = false;
			return;
	}

	// matrix
	switch (videoMatrix) {
		case (int) pMatrix::UNKNOWN:
			matrix = width > 1024 ? pMatrix::BT709 : pMatrix::BT601;
			break;

		case (int) pMatrix::BT601:
			matrix = pMatrix::BT601;
			break;

		case (int) pMatrix::BT709:
			matrix = pMatrix::BT709;
			break;

		default:
			init = false;
			return;
	}

	// fourCC
	switch (videoFourCC) {
		case (int) pFormat::P008:
			fourCC = pFormat::P008;
			break;

		case (int) pFormat::P208:
			fourCC = pFormat::P208;
			break;

		case (int) pFormat::P408:
			fourCC = pFormat::P408;
			break;

		case (int) pFormat::P010:
			fourCC = pFormat::P010;
			break;

		case (int) pFormat::P210:
			fourCC = pFormat::P210;
			break;

		case (int) pFormat::P410:
			fourCC = pFormat::P410;
			break;

		case (int) pFormat::P016:
			fourCC = pFormat::P016;
			break;

		case (int) pFormat::P216:
			fourCC = pFormat::P216;
			break;

		case (int) pFormat::P416:
			fourCC = pFormat::P416;
			break;

		case (int) pFormat::NV12:
			fourCC = pFormat::NV12;
			break;

		case (int) pFormat::NV21:
			fourCC = pFormat::NV21;
			break;

		case (int) pFormat::RGBA:
			fourCC = pFormat::RGBA;
			break;

		default:
			init = false;
			return;
	}

	// Bpp
	switch (fourCC) {
		case pFormat::P008:
			Bpp = 12;
			break;

		case pFormat::NV12:
		case pFormat::NV21:
			Bpp = 12;
			break;

		case pFormat::P208:
			Bpp = 16;
			break;

		case pFormat::P408:
		case pFormat::P010:
		case pFormat::P016:
			Bpp = 24;
			break;

		case pFormat::P210:
		case pFormat::P216:
		case pFormat::RGBA:
			Bpp = 32;
			break;

		case pFormat::P410:
		case pFormat::P416:
			Bpp = 48;
			break;

		default:
			init = false;
			return;
	}

	// subsampling
	switch (fourCC) {
		// 4:2:0 - both are subsampled
		case pFormat::P008:
		case pFormat::P010:
		case pFormat::P016:
		case pFormat::NV12:
		case pFormat::NV21:
			halfWidth = true;
			halfHeight = true;
			break;

		// 4:2:2 - only width is subsampled
		case pFormat::P208:
		case pFormat::P210:
		case pFormat::P216:
			halfWidth = true;
			halfHeight = false;
			break;

		// 4:4:4 - not subsampled
		case pFormat::P408:
		case pFormat::P410:
		case pFormat::P416:
		case pFormat::RGBA:
			halfWidth = false;
			halfHeight = false;
			break;

		default:
			init = false;
			return;
	}

	// number of planes
	switch (fourCC) {
		case pFormat::P008:
		case pFormat::P208:
		case pFormat::P408:
		case pFormat::P010:
		case pFormat::P210:
		case pFormat::P410:
		case pFormat::P016:
		case pFormat::P216:
		case pFormat::P416:
			planes = 3;
			break;

		case pFormat::NV12:
		case pFormat::NV21:
			planes = 2;
			break;

		case pFormat::RGBA:
			planes = 1;
			break;

		default:
			init = false;
			return;
	}

	// plane format
	switch (fourCC) {
		case pFormat::P008:
		case pFormat::P208:
		case pFormat::P408:
			internalLumaFormat = GL_R8;
			internalChromaFormat = GL_R8;
			lumaFormat = GL_RED;
			chromaFormat = GL_RED;
			lumaType = GL_UNSIGNED_BYTE;
			chromaType = GL_UNSIGNED_BYTE;
			break;

		case pFormat::P010:
		case pFormat::P210:
		case pFormat::P410:
		case pFormat::P016:
		case pFormat::P216:
		case pFormat::P416:
			internalLumaFormat = GL_RG8;
			internalChromaFormat = GL_RG8;
			lumaFormat = GL_RG;
			chromaFormat = GL_RG;
			lumaType = GL_UNSIGNED_BYTE;
			chromaType = GL_UNSIGNED_BYTE;
			break;

		case pFormat::NV12:
		case pFormat::NV21:
			internalLumaFormat = GL_R8;
			internalChromaFormat = GL_RG8;
			lumaFormat = GL_RED;
			chromaFormat = GL_RG;
			lumaType = GL_UNSIGNED_BYTE;
			chromaType = GL_UNSIGNED_BYTE;
			break;

		case pFormat::RGBA:
			internalLumaFormat = GL_RGBA8;
			lumaFormat = GL_RGBA;
			lumaType = GL_UNSIGNED_BYTE;
			break;

		default:
			init = false;
			return;
	}

	// chroma size
	chromaWidth = halfWidth ? width / 2 : width;
	chromaHeight = halfHeight ? height / 2 : height;
	if (fourCC == pFormat::NV12 || fourCC == pFormat::NV12)
		chromaWidth *= 2;

	// offset
	switch (fourCC) {
		case pFormat::P008:
		case pFormat::P208:
		case pFormat::P408:
		case pFormat::NV12:
		case pFormat::NV21:
		case pFormat::RGBA:
			offset1 = width * height;
			offset2 = offset1 + chromaWidth * chromaHeight;
			break;

		case pFormat::P010:
		case pFormat::P210:
		case pFormat::P410:
		case pFormat::P016:
		case pFormat::P216:
		case pFormat::P416:
			offset1 = width * height * 2;
			offset2 = offset1 + chromaWidth * chromaHeight * 2;
			break;

		default:
			init = false;
			return;
	}

	// calculate yub -> rgb matrix
	if (fourCC != pFormat::RGBA) {
		switch (range) {
			case pRange::TV:
				rangeY = 255.0 / 219.0;
				rangeC = 255.0 / 112.0;
				luma0 = 16.0;
				break;
			case pRange::PC:
				rangeY = 1.0;
				rangeC = 2.0;
				luma0 = 0.0;
				break;
			default:
				init = false;
				return;
		}

		switch (matrix) {
			case pMatrix::BT601:
				Kb = 0.114;
				Kr = 0.299;
				break;
			case pMatrix::BT709:
				Kb = 0.0722;
				Kr = 0.2126;
				break;
			default:
				init = false;
				return;
		}

		colorConversion[0] = (float) (rangeY);
		colorConversion[1] = (float) (0.0);
		colorConversion[2] = (float) (rangeC * (1.0 - Kr));
		colorConversion[3] = (float) (rangeY);
		colorConversion[4] = (float) (-rangeC * (1.0 - Kb) * Kb / (1.0 - Kb - Kr));
		colorConversion[5] = (float) (-rangeC * (1.0 - Kr) * Kr / (1.0 - Kb - Kr));
		colorConversion[6] = (float) (rangeY);
		colorConversion[7] = (float) (rangeC * (1.0 - Kb));
		colorConversion[8] = (float) (0.0);

		colorOffset[0] = (float) ((-luma0 * rangeY - 128.0 * rangeC * (1.0 - Kr)) / 255.0);
		colorOffset[1] = (float) ((-luma0 * rangeY + rangeC * (1.0 - Kb) * Kb / (1.0 - Kb - Kr) * 128.0 + rangeC * (1.0 - Kr) * Kr / (1.0 - Kb - Kr) * 128.0) / 255.0);
		colorOffset[2] = (float) ((-luma0 * rangeY - 128.0 * rangeC * (1.0 - Kb)) / 255.0);
	}
}

videoInfo::~videoInfo () {
}
