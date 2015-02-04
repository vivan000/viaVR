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
#include <thread>
#include <vector>
#include "IVideoRenderer.h"
#include "IVideoDecoder.h"
#include "frames.h"
#include "queue.h"
#include "renderingPass.h"

typedef std::vector<renderingPass*> rPass;

class videoRenderer: public IVideoRenderer {
public:
	videoRenderer ();
	~videoRenderer ();

	bool addVideoDecoder (IVideoDecoder* video);
	bool addWindow (ANativeWindow* window);
	void setRefreshRate (int fps);
	bool init ();

	void seek (int timecode);
	void play (int timecode);
	void pause ();

	const char* getName ();
	int getMajorVersion ();
	int getMinorVersion ();

private:
	int64_t nanotime ();
	int tcNow ();

	bool checkExtensions ();

	void setAspect ();
	void genContexts ();
	void delContexts ();

	void drawFrame ();
	void presentFrame (frameGPUo* f);
	void getNextFrame (frameGPUo* f);

	void upload ();
	void decode ();
	void render ();

	bool renderInit ();
	void getGlError ();
	void getFbStatus ();
	const char* getEglErrorStr ();

	// video config
	IVideoDecoder* video;
	videoInfo* info;
	int videoSarWidth, videoSarHeight;
	int videoFps;

	bool decoding, uploading, rendering, playing;

	queue<frameCPU>* decodeQueue;
	queue<frameGPUu>* uploadQueue;
	queue<frameGPUo>* renderQueue;

	std::thread decodeThread;
	std::thread uploadThread;
	std::thread renderThread;
	std::thread drawThread;

	EGLDisplay display;

	EGLSurface mainSurface;
	EGLSurface uploadPbuffer;
	EGLSurface renderPbuffer;

	EGLContext mainContext;
	EGLContext uploadContext;
	EGLContext renderContext;

	EGLint surfaceWidth, surfaceHeight;

	int precisionTex = 0;
	bool precisionHighp = true;

	bool initialized;
	int64_t start;

	int displayRefreshRate = 60;

	frameGPUo* displayCurr;
	int repeat, repeatLim;
	bool newFrame;
	int frameNumber, presentedFrames;
	int hardLate, softLate, softEarly, hardEarly;

	int64_t prev, prev2;

	GLuint vboIds[3];

	rPass pass;
	pFormat internalType;
	const char* precision = "highp";
	int bitdepth;

	bool decodeSeeking;
	bool uploadSeeking;
	bool renderSeeking;

	frameCPU*  decodeTo;
	frameCPU*  uploadFrom;
	frameGPUu* uploadTo;
	frameGPUu* renderFrom;
	frameGPUo* renderTo;

	GLuint framebuffer;
};
