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

#pragma once
#include <GLES3/gl3.h>
#include <EGL/egl.h>
#include "interface/IVideoRenderer.h"
#include "threads.h"

class videoRenderer: public IVideoRenderer {
public:
	videoRenderer ();
	~videoRenderer ();

	bool addVideoDecoder (IVideoDecoder* video);
	bool addWindow (ANativeWindow* window);
	bool init ();

	void seek (int timecode);
	void play (int timecode);
	void pause ();

	const char* getName ();
	int getMajorVersion ();
	int getMinorVersion ();

private:
	bool renderInit ();
	void loadVbos ();
	bool checkExtensions ();
	void setAspect ();

	void delContexts ();
	void getGlError ();
	void getFbStatus ();
	const char* getEglErrorStr ();

	IVideoDecoder* video;
	videoInfo* info;
	GLuint vboIds[3];
	bool initialized;

	queue<frameCPU>* decodeQueue;
	queue<frameGPUu>* uploadQueue;
	queue<frameGPUo>* renderQueue;

	EGLDisplay display;

	EGLSurface mainSurface;
	EGLSurface uploadPbuffer;
	EGLSurface renderPbuffer;

	EGLContext mainContext;
	EGLContext uploadContext;
	EGLContext renderContext;

	EGLint surfaceWidth, surfaceHeight;

	decoder* decodeO;
	uploader* uploadO;
	renderer* renderO;
	presenter* presentO;
};
