#include "frames.h"

int chooseBpp (pFormat format) {
	switch (format) {
		case pFormat::P008:
		case pFormat::NV12:
		case pFormat::NV21:
			return 12;

		case pFormat::P208:
			return 16;

		case pFormat::P408:
		case pFormat::P010:
		case pFormat::P016:
			return 24;

		case pFormat::P210:
		case pFormat::P216:
		case pFormat::RGBA:
			return 32;

		case pFormat::P410:
		case pFormat::P416:
			return 48;
	}
	return 0;
}

int chooseUPlanes (pFormat format) {
	switch (format) {
		case pFormat::P008:
		case pFormat::P208:
		case pFormat::P408:
		case pFormat::P010:
		case pFormat::P210:
		case pFormat::P410:
		case pFormat::P016:
		case pFormat::P216:
		case pFormat::P416:
			return 3;

		case pFormat::NV12:
		case pFormat::NV21:
			return 2;

		case pFormat::RGBA:
			return 1;
	}
	return 0;
}

GLenum chooseUFormat (pFormat format, bool first) {
	switch (format) {
		case pFormat::P008:
		case pFormat::P208:
		case pFormat::P408:
			return GL_LUMINANCE;

		case pFormat::P010:
		case pFormat::P210:
		case pFormat::P410:
		case pFormat::P016:
		case pFormat::P216:
		case pFormat::P416:
			return GL_LUMINANCE_ALPHA;

		case pFormat::NV12:
		case pFormat::NV21:
			if (first)
				return GL_LUMINANCE;
			else
				return GL_LUMINANCE_ALPHA;

		case pFormat::RGBA:
			return GL_RGBA;
	}
	return 0;
}

bool chooseUHalf (pFormat format, bool first, bool width) {
	// luma is never subsampled
	if (first)
		return false;

	switch (format) {
		// 4:2:0 - both are subsampled
		case pFormat::P008:
		case pFormat::P010:
		case pFormat::P016:
		case pFormat::NV12:
		case pFormat::NV21:
			return true;

		// 4:2:2 - only width is subsampled
		case pFormat::P208:
		case pFormat::P210:
		case pFormat::P216:
			return width;

		// 4:4:4 - not subsampled
		case pFormat::P408:
		case pFormat::P410:
		case pFormat::P416:
		case pFormat::RGBA:
			return false;
	}
	return false;
}

GLenum chooseType (pFormat format) {
	switch (format) {
		case pFormat::HALF:
			return GL_HALF_FLOAT_OES;
		case pFormat::FULL:
			return GL_FLOAT;
	}
	return 0;
}
