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
#include "frames.h"
#include "queue.h"
#include "shader.h"
#include "renderingPass.h"
#include "log.h"

typedef std::vector<renderingPass*> rPass;

class decoder {
public:
	decoder (videoInfo* info, queue<frameCPU>* decodeQueue, IVideoDecoder* video);
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
	uploader (videoInfo* info, queue<frameCPU>* decodeQueue, queue<frameGPUu>* uploadQueue,
		EGLDisplay display, EGLSurface uploadPbuffer, EGLContext uploadContext);
	~uploader ();
	void stop ();
	void start ();

private:
	void upload ();

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
	renderer (videoInfo* info, queue<frameGPUu>* uploadQueue, queue<frameGPUo>* renderQueue,
		EGLDisplay display, EGLSurface renderPbuffer, EGLContext renderContext, GLuint* vboIds);
	~renderer ();
	void stop ();
	void start ();

private:
	void render ();
	bool renderInit ();

	videoInfo* info;
	queue<frameGPUu>* uploadQueue;
	queue<frameGPUo>* renderQueue;

	EGLDisplay display;
	EGLSurface pbuffer;
	EGLContext context;
	GLuint* vboIds;
	GLuint framebuffer;

	rPass pass;

	frameGPUu* from;
	frameGPUo* to;
	bool working, joined;
	std::thread thread;

	pFormat internalType = pFormat::INT10;
	const char* precision = "highp";
	int bitdepth = 8;
};
