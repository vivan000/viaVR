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

#include "threads/threads.h"

uploader::uploader (videoInfo* info, config* cfg, queue<frameCPU>* decodeQueue, queue<frameGPUu>* uploadQueue,
		EGLDisplay display, EGLSurface uploadPbuffer, EGLContext uploadContext) {
	uploader::info = info;
	uploader::cfg = cfg;
	uploader::decodeQueue = decodeQueue;
	uploader::uploadQueue = uploadQueue;

	uploader::display = display;
	uploader::pbuffer = uploadPbuffer;
	uploader::context = uploadContext;

	eglMakeCurrent (display, pbuffer, pbuffer, context);
	from = new frameCPU (info, cfg);
	to = new frameGPUu (info, cfg);
	eglMakeCurrent (display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);

	start ();
}

uploader::~uploader () {
	if (!joined)
		stop ();

	eglMakeCurrent (display, pbuffer, pbuffer, context);
	delete from;
	delete to;
	eglMakeCurrent (display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
}

void uploader::start () {
	thread = std::thread (&uploader::upload, this);
}

void uploader::stop () {
	working = false;
	thread.join ();
	joined = true;
}

void uploader::upload () {
	eglMakeCurrent (display, pbuffer, pbuffer, context);

	joined = false;
	working = true;
	while (working) {
		if (!decodeQueue->isEmpty () && !uploadQueue->isFull ()) {
			decodeQueue->pop (from);

			to->timecode = from->timecode;

			glBindTexture (GL_TEXTURE_2D, to->plane[0]);
			glTexSubImage2D (GL_TEXTURE_2D, 0, 0, 0, info->width, info->height,
				info->lumaFormat, info->lumaType, (GLvoid*) from->plane);

			if (info->planes > 1) {
				glBindTexture (GL_TEXTURE_2D, to->plane[1]);
				glTexSubImage2D (GL_TEXTURE_2D, 0, 0, 0, info->chromaWidth, info->chromaHeight,
					info->chromaFormat, info->chromaType, (GLvoid*) (from->plane + info->offset1));
			}

			if (info->planes > 2) {
				glBindTexture (GL_TEXTURE_2D, to->plane[2]);
				glTexSubImage2D (GL_TEXTURE_2D, 0, 0, 0, info->chromaWidth, info->chromaHeight,
					info->chromaFormat, info->chromaType, (GLvoid*) (from->plane + info->offset2));
			}

			glFlush ();

			uploadQueue->push (to);
		} else {
			usleep (10000);
		}
	}

	eglMakeCurrent (display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
}
