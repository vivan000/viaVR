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

#include <GLES3/gl3.h>
#include <EGL/egl.h>
#include "log.h"

bool checkGlStatus () {
	bool error = false;
	GLenum err;
	while ((err = glGetError()) != GL_NO_ERROR) {
		error = true;
		switch (err) {
			case GL_INVALID_ENUM:
				LOGE ("OpenGL error: GL_INVALID_ENUM");
				break;
			case GL_INVALID_VALUE:
				LOGE ("OpenGL error: GL_INVALID_VALUE");
				break;
			case GL_INVALID_OPERATION:
				LOGE ("OpenGL error: GL_INVALID_OPERATION");
				break;
			case GL_INVALID_FRAMEBUFFER_OPERATION:
				LOGE ("OpenGL error: GL_INVALID_FRAMEBUFFER_OPERATION");
				break;
			case GL_OUT_OF_MEMORY:
				LOGE ("OpenGL error: GL_OUT_OF_MEMORY");
				break;
			default:
				LOGE ("OpenGL error: unknown error %d", err);
		}
	}

	return !error;
}

bool checkFbStatus () {
	GLenum err = glCheckFramebufferStatus (GL_FRAMEBUFFER);
	switch (err) {
		case GL_FRAMEBUFFER_COMPLETE:
			LOGE ("Framebuffer: GL_FRAMEBUFFER_COMPLETE");
			return true;
		case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
			LOGE ("Framebuffer: GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT");
			return false;
		case GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS:
			LOGE ("Framebuffer: GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS");
			return false;
		case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
			LOGE ("Framebuffer: GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT");
			return false;
		case GL_FRAMEBUFFER_UNSUPPORTED:
			LOGE ("Framebuffer: GL_FRAMEBUFFER_UNSUPPORTED");
			return false;
		default:
			LOGE ("Framebuffer: unknown error %d", err);
			return false;
	}
}

const char* getEglErrorStr () {
	switch (eglGetError ()) {
		case EGL_SUCCESS:
			return "EGL_SUCCESS";
		case EGL_NOT_INITIALIZED:
			return "EGL_NOT_INITIALIZED";
		case EGL_BAD_ACCESS:
			return "EGL_BAD_ACCESS";
		case EGL_BAD_ALLOC:
			return "EGL_BAD_ALLOC";
		case EGL_BAD_ATTRIBUTE:
			return "EGL_BAD_ATTRIBUTE";
		case EGL_BAD_CONTEXT:
			return "EGL_BAD_CONTEXT";
		case EGL_BAD_CONFIG:
			return "EGL_BAD_CONFIG";
		case EGL_BAD_CURRENT_SURFACE:
			return "EGL_BAD_CURRENT_SURFACE";
		case EGL_BAD_DISPLAY:
			return "EGL_BAD_DISPLAY";
		case EGL_BAD_SURFACE:
			return "EGL_BAD_SURFACE";
		case EGL_BAD_MATCH:
			return "EGL_BAD_MATCH";
		case EGL_BAD_PARAMETER:
			return "EGL_BAD_PARAMETER";
		case EGL_BAD_NATIVE_PIXMAP:
			return "EGL_BAD_NATIVE_PIXMAP";
		case EGL_BAD_NATIVE_WINDOW:
			return "EGL_BAD_NATIVE_WINDOW";
		case EGL_CONTEXT_LOST:
			return "EGL_CONTEXT_LOST";
		default:
			return "unknown EGL error";
	}
}
