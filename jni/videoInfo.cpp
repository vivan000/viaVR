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
			lumaFormat = GL_LUMINANCE;
			chromaFormat = GL_LUMINANCE;
			break;

		case pFormat::P010:
		case pFormat::P210:
		case pFormat::P410:
		case pFormat::P016:
		case pFormat::P216:
		case pFormat::P416:
			lumaFormat = GL_LUMINANCE_ALPHA;
			chromaFormat = GL_LUMINANCE_ALPHA;
			break;

		case pFormat::NV12:
		case pFormat::NV21:
			lumaFormat = GL_LUMINANCE;
			chromaFormat = GL_LUMINANCE_ALPHA;
			break;

		case pFormat::RGBA:
			lumaFormat = GL_RGBA;
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
}

videoInfo::~videoInfo () {
}
