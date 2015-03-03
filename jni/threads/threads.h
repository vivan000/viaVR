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
#include <unistd.h>
#include <thread>
#include <vector>
#include "interface/IVideodecoder.h"
#include "frames/frames.h"
#include "queue.h"
#include "threads/helpers/shaderLoader.h"
#include "threads/helpers/renderingPass.h"
#include "log.h"

typedef std::vector<renderingPass*> rPass;

class decoder {
public:
	decoder (videoInfo* info, config* cfg, queue<frameCPU>* decodeQueue, IVideoDecoder* video);
	~decoder ();
	void stop ();
	void start ();

private:
	void decode ();

	int size;
	queue<frameCPU>* decodeQueue;
	IVideoDecoder* video;

	frameCPU* to;
	bool working, joined;
	std::thread thread;
};

class uploader {
public:
	uploader (videoInfo* info, config* cfg, queue<frameCPU>* decodeQueue, queue<frameGPUu>* uploadQueue,
		EGLDisplay display, EGLSurface uploadPbuffer, EGLContext uploadContext);
	~uploader ();
	void stop ();
	void start ();

private:
	void upload ();

	config* cfg;
	videoInfo* info;
	queue<frameCPU>* decodeQueue;
	queue<frameGPUu>* uploadQueue;

	EGLDisplay display;
	EGLSurface pbuffer;
	EGLContext context;

	frameCPU* from;
	frameGPUu* to;
	bool working, joined;
	std::thread thread;
};

class renderer {
public:
	renderer (videoInfo* info, config* cfg, queue<frameCPU>* decodeQueue, queue<frameGPUo>* renderQueue,
		EGLDisplay display, EGLSurface renderPbuffer, EGLContext renderContext, GLuint* vboIds);
	~renderer ();
	void stop ();
	void start ();

private:
	void render ();
	bool renderInit ();

	videoInfo* info;
	config* cfg;
	queue<frameCPU>* decodeQueue;
	queue<frameGPUo>* renderQueue;

	EGLDisplay display;
	EGLSurface pbuffer;
	EGLContext context;
	GLuint* vboIds;
	GLuint framebuffer, framebufferAR;

	rPass pass;
	std::vector<frameGPUi*> helperFrames;

	frameCPU* from;
	frameGPUu* up;
	frameGPUo* to;

	bool working, joined;
	std::thread thread;
};

class presenter {
public:
	presenter (videoInfo* info, config* cfg, queue<frameGPUo>* renderQueue,
		EGLDisplay display, EGLSurface mainSurface, EGLContext mainContext);
	~presenter ();
	void pause ();
	void play (int timecode);

private:
	void present ();
	void getNextFrame ();
	void presentFrame ();
	int64_t nanotime ();
	int tcNow ();

	videoInfo* info;
	config* cfg;
	queue<frameGPUo>* renderQueue;

	EGLDisplay display;
	EGLSurface surface;
	EGLContext context;

	GLuint displaySP;

	frameGPUo* from;
	bool working, playing;
	std::thread thread;

	int64_t start;
	int64_t prev, prev2;

	int repeat, repeatLim;
	bool newFrame;
	int frameNumber, presentedFrames;
	int hardLate, softLate, softEarly, hardEarly;

	int videoFps;
};
