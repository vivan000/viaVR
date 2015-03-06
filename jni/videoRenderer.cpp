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

#include <time.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <string>
#include "log.h"
#include "videoRenderer.h"

videoRenderer::videoRenderer () {
	cfg = new config ();
	LOGD ("%s v%i.%i %s %s", getName (), getMajorVersion (), getMinorVersion (), __DATE__, __TIME__);
}

videoRenderer::~videoRenderer () {
	LOGD ("Destroing videoRenderer");
	if (initialized) {
		delete presentO;
		delete decodeO;
		delete renderO;

		eglMakeCurrent (display, mainSurface, mainSurface, mainContext);

		delete decodeQueue;
		delete renderQueue;

		glDeleteBuffers (3, vboIds);

		eglMakeCurrent (display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);

		eglDestroySurface (display, renderPbuffer);
		eglDestroyContext (display, renderContext);
		eglDestroySurface (display, mainSurface);
		eglDestroyContext (display, mainContext);

		eglTerminate (display);
	}

	delete info;
	delete cfg;
}

const char* videoRenderer::getName () {
	return "viaVR";
}

int videoRenderer::getMajorVersion () {
	return 0;
}

int videoRenderer::getMinorVersion () {
	return 0;
}

bool videoRenderer::addVideoDecoder (IVideoDecoder* video) {
	videoRenderer::video = video;
	info = new videoInfo (video->getWidth (), video->getHeight (),
		video->getFourCC (), video->getRange (), video->getMatrix ());

	if (!info->init)
		return false;

	info->sarWidth = video->getSarWidth ();
	info->sarHeight = video->getSarHeight ();
	info->fpsNumerator = video->getFpsNumerator ();
	info->fpsDenominator = video->getFpsDenominator ();

	LOGD ("Video info:");
	LOGD ("Video %ix%i (%i:%i)", info->width, info->height, info->sarWidth, info->sarHeight);
	LOGD (reinterpret_cast <char*> (&info->fourCC)[2] > 16 ? "FourCC: %c%c%c%c\n" : "FourCC: %c%c[%i][%i]\n",
		reinterpret_cast <char*> (&info->fourCC)[0], reinterpret_cast <char*> (&info->fourCC)[1],
		reinterpret_cast <char*> (&info->fourCC)[2], reinterpret_cast <char*> (&info->fourCC)[3]);
	LOGD ("Matrix: %s (%s)", info->matrix == pMatrix::BT709 ? "BT.709" : "BT.601", video->getMatrix () != 0 ? "upstream" : "guess");
	LOGD ("Range: %s (%s)",	info->range == pRange::TV ? "TV" : "PC", video->getRange () != 0 ? "upstream" : "guess");
	LOGD ("Framerate: %i/%i", info->fpsNumerator, info->fpsDenominator);

	LOGD ("Video decoder connected");
	return true;
}

bool videoRenderer::addWindow (ANativeWindow* window) {
	const EGLint attribListWindowConfig[] = {
		EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
		EGL_BLUE_SIZE, 8,
		EGL_GREEN_SIZE, 8,
		EGL_RED_SIZE, 8,
		EGL_RENDERABLE_TYPE, 0x00000040, // EGL_OPENGL_ES3_BIT is not defined in egl.h yet
		EGL_NONE};

	const EGLint attribListPbufferConfig[] = {
		EGL_SURFACE_TYPE, EGL_PBUFFER_BIT,
		EGL_NONE};

	const EGLint attribListMainContext[] = {
		EGL_CONTEXT_CLIENT_VERSION, 3,
		EGL_CONTEXT_PRIORITY_LEVEL_IMG, EGL_CONTEXT_PRIORITY_HIGH_IMG,
		EGL_NONE};

	const EGLint attribListBackgroundContext[] = {
		EGL_CONTEXT_CLIENT_VERSION, 3,
		EGL_CONTEXT_PRIORITY_LEVEL_IMG, EGL_CONTEXT_PRIORITY_LOW_IMG,
		EGL_NONE};

	const EGLint attribListNoPriorityContext[] = {
		EGL_CONTEXT_CLIENT_VERSION, 3,
		EGL_NONE};

	const EGLint attribListPbufferSurface[] = {
		EGL_WIDTH, 1,
		EGL_HEIGHT, 1,
		EGL_NONE};

	EGLConfig choosedConfig;
	EGLint numConfigs;
	EGLint format;
	bool priority = true;

	display = eglGetDisplay (EGL_DEFAULT_DISPLAY);
	if (display == EGL_NO_DISPLAY) {
		LOGE ("GetDisplay error: %s", getEglErrorStr ());
		return false;
	}

	if (!eglInitialize (display, 0, 0)) {
		LOGE ("Initialize error: %s", getEglErrorStr ());
		return false;
	}

	if (!eglChooseConfig (display, attribListWindowConfig, &choosedConfig, 1, &numConfigs)) {
		LOGE ("ChooseConfig error: %s", getEglErrorStr ());
		return false;
	}

	if (!eglGetConfigAttrib (display, choosedConfig, EGL_NATIVE_VISUAL_ID, &format)) {
		LOGE ("GetConfigAttrib error: %s", getEglErrorStr ());
		return false;
	}

	ANativeWindow_setBuffersGeometry (window, 0, 0, format);

	mainSurface = eglCreateWindowSurface (display, choosedConfig, window, 0);
	if (!mainSurface) {
		LOGE ("CreateWindowSurface error: %s", getEglErrorStr ());
		return false;
	}

	mainContext = eglCreateContext (display, choosedConfig, 0, attribListMainContext);
	if (!mainContext) {
		mainContext = eglCreateContext (display, choosedConfig, 0, attribListNoPriorityContext);
		if (!mainContext) {
			LOGE ("CreateContext error: %s", getEglErrorStr ());
			return false;
		} else {
			LOGD ("Context priority is not supported");
			priority = false;
		}
	}

	if (!eglMakeCurrent (display, mainSurface, mainSurface, mainContext)) {
		LOGE ("MakeCurrent error: %s", getEglErrorStr ());
		return false;
	}

	if (!eglQuerySurface (display, mainSurface, EGL_WIDTH, &surfaceWidth) ||
		!eglQuerySurface (display, mainSurface, EGL_HEIGHT, &surfaceHeight)) {
		LOGE ("QuerySurface error: %s", getEglErrorStr ());
		return false;
	}

	if (!eglChooseConfig (display, attribListPbufferConfig, &choosedConfig, 1, &numConfigs)) {
		LOGE ("Pbuffer ChooseConfig error: %s", getEglErrorStr ());
		return false;
	}

	renderPbuffer = eglCreatePbufferSurface (display, choosedConfig, attribListPbufferSurface);
	if (!renderPbuffer) {
		LOGE ("Render pbuffer CreatePbufferSurface error: %s", getEglErrorStr ());
		return false;
	}

	renderContext = eglCreateContext (display, choosedConfig, mainContext,
		priority ? attribListBackgroundContext : attribListNoPriorityContext);
	if (!renderContext) {
		LOGE ("Render pbuffer CreateContext error: %s", getEglErrorStr ());
		return false;
	}

	eglMakeCurrent (display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);

	LOGD ("Window added");
	return true;
}

bool videoRenderer::init () {
	eglMakeCurrent (display, mainSurface, mainSurface, mainContext);

	// check version
	GLint GLversion = 0;
	glGetIntegerv (GL_MAJOR_VERSION, &GLversion);
	if (GLversion < 3)
		return false;
	LOGD ("Version: OK");

	// check video decoder
	if (!info->init)
		return false;
	LOGD ("Video decoder: OK");

	// set viewport
	setAspect ();

	// check extenions
	if (!checkExtensions ())
		return false;
	LOGD ("Extensions: OK");

	// load coordinates
	loadVbos ();

	// create queues
	decodeQueue = new queue<frameCPU> (cfg->decodeQueueSize, info, cfg);
	renderQueue = new queue<frameGPUo> (cfg->renderQueueSize, info, cfg);

	// check GL errors
	if (!checkGlStatus ())
		return false;
	LOGD ("No GL errors so far");

	// start threads
	eglMakeCurrent (display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
	presentO = new presenter (info, cfg, renderQueue, display, mainSurface, mainContext);
	decodeO = new decoder (info, cfg, decodeQueue, video);
	renderO = new renderer (info, cfg, decodeQueue, renderQueue, display, renderPbuffer, renderContext, vboIds);

	// wait till there's at least 1 frame to show
#if !BENCHMARK
	while (renderQueue->isEmpty ())
		usleep (10000);
#endif
	initialized = true;
	LOGD ("Initialization: OK");
	return true;
}

bool videoRenderer::checkExtensions () {
	GLint numExtensions;
	std::vector<std::string> extList;

	glGetIntegerv (GL_NUM_EXTENSIONS, &numExtensions);
	for (int i = 0; i < numExtensions; i++)
		extList.push_back ((const char*) glGetStringi (GL_EXTENSIONS, i));

	bool extColorHalfFloat = false;
	bool extColorFloat = false;
	bool extHalfFloatLinear = false;
	bool extFloatLinear = false;
	bool extBinningControl = false;
	bool extWriteOnly = false;
	bool extTimerQuery = false;

	for (unsigned int i = 0; i < extList.size (); i++) {
		if (!extList.at (i).compare ("GL_EXT_color_buffer_half_float") && !extColorHalfFloat)
			extColorHalfFloat = true;
		if (!extList.at (i).compare ("GL_EXT_color_buffer_float") && !extColorFloat)
			extColorFloat = true;
		if (!extList.at (i).compare ("GL_OES_texture_half_float_linear") && !extHalfFloatLinear)
			extHalfFloatLinear = true;
		if (!extList.at (i).compare ("GL_OES_texture_float_linear") && !extFloatLinear)
			extFloatLinear = true;
		if (!extList.at (i).compare ("GL_QCOM_binning_control") && !extBinningControl)
			extBinningControl = true;
		if (!extList.at (i).compare ("GL_QCOM_writeonly_rendering") && !extWriteOnly)
			extWriteOnly = true;
		if (!extList.at (i).compare ("GL_EXT_disjoint_timer_query") && !extTimerQuery)
			extTimerQuery = true;
	}

	if (cfg->internalType == iFormat::FLOAT16) {
		if (!extColorHalfFloat)
			return false;
		if (!extHalfFloatLinear && cfg->hwScaleLinear)
			return false;
	}

	if (cfg->internalType == iFormat::FLOAT32) {
		if (!extColorFloat)
			return false;
		if (!extFloatLinear && cfg->hwScaleLinear)
			return false;
	}

/*
	if (extBinningControl)
		glHint (0x8FB0, 0x8FB3); // BINNING_CONTROL_HINT_QCOM, RENDER_DIRECT_TO_FRAMEBUFFER_QCOM

	if (extWriteOnly)
		glEnable (0x8823); // WRITEONLY_RENDERING_QCOM
*/

	LOGD ("Extensions:");
	LOGD ("Half-float texture: %s, bilinear: %s",
		extColorHalfFloat ? "supported" : "not supported",
		extHalfFloatLinear  ? "supported" : "not supported");
	LOGD ("Float texture: %s, bilinear: %s",
		extColorFloat ? "supported" : "not supported",
		extFloatLinear  ? "supported" : "not supported");
	LOGD ("Binning control: %s", extBinningControl ? "supported" : "not supported");
	LOGD ("Write-only rendering: %s", extWriteOnly ? "supported" : "not supported");
	LOGD ("Timer queries: %s", extTimerQuery ? "supported" : "not supported");
	return true;
}

void videoRenderer::setAspect () {
	switch (cfg->displayMode) {
		// stretch
		case 0:
			cfg->targetX = 0;
			cfg->targetY = 0;
			cfg->targetWidth = surfaceWidth;
			cfg->targetHeight = surfaceHeight;
			break;

		// touch from inside
		case 1:
			if ((long long) surfaceWidth * info->height * info->sarHeight < (long long) info->width * info->sarWidth * surfaceHeight) {
				cfg->targetX = 0;
				cfg->targetWidth = surfaceWidth;
				cfg->targetHeight = (long long) surfaceWidth * info->height * info->sarHeight / info->width / info->sarWidth;
				cfg->targetY = (surfaceHeight - cfg->targetHeight) / 2;
			} else {
				cfg->targetY = 0;
				cfg->targetHeight = surfaceHeight;
				cfg->targetWidth = (long long) surfaceHeight * info->width * info->sarWidth / info->height / info->sarHeight;
				cfg->targetX = (surfaceWidth - cfg->targetWidth) / 2;
			}
			break;

		// touch from outside
		case 2:
			if ((long long) surfaceWidth * info->height * info->sarHeight > (long long) info->width * info->sarWidth * surfaceHeight) {
				cfg->targetX = 0;
				cfg->targetWidth = surfaceWidth;
				cfg->targetHeight = (long long) surfaceWidth * info->height * info->sarHeight / info->width / info->sarWidth;
				cfg->targetY = (surfaceHeight - cfg->targetHeight) / 2;
			} else {
				cfg->targetY = 0;
				cfg->targetHeight = surfaceHeight;
				cfg->targetWidth = (long long) surfaceHeight * info->width * info->sarWidth / info->height / info->sarHeight;
				cfg->targetX = (surfaceWidth - cfg->targetWidth) / 2;
			}
			break;

		// 100%
		case 3:
			cfg->targetWidth = info->width;
			cfg->targetHeight = info->height;
			cfg->targetX = (surfaceWidth - cfg->targetWidth) / 2;
			cfg->targetY = (surfaceHeight - cfg->targetHeight) / 2;
			break;
	}

	glViewport (cfg->targetX, cfg->targetY, cfg->targetWidth, cfg->targetHeight);
	LOGD ("Target rectangle: %i %i %i %i", cfg->targetX, cfg->targetY, cfg->targetWidth, cfg->targetHeight);
}

void videoRenderer::loadVbos () {
	const float VertexPositions[] = {
		-1.0f, -1.0f, 0.0f, 1.0f,
		-1.0f,  1.0f, 0.0f, 1.0f,
		 1.0f, -1.0f, 0.0f, 1.0f,
		 1.0f,  1.0f, 0.0f, 1.0f};

	const float VertexTexcoord[] = {
		0.0f, 0.0f,
		0.0f, 1.0f,
		1.0f, 0.0f,
		1.0f, 1.0f};

	const float VertexFlippedTexcoord[] = {
		0.0f, 1.0f,
		0.0f, 0.0f,
		1.0f, 1.0f,
		1.0f, 0.0f};

	glGenBuffers (3, vboIds);

	const GLuint vertexCoordLoc = 0;
	glBindBuffer (GL_ARRAY_BUFFER, vboIds[0]);
	glBufferData (GL_ARRAY_BUFFER, sizeof (VertexPositions), VertexPositions, GL_STATIC_DRAW);
	glVertexAttribPointer (vertexCoordLoc, 4, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray (vertexCoordLoc);

	const GLuint textureFlippedCoordLoc = 1;
	glBindBuffer (GL_ARRAY_BUFFER, vboIds[1]);
	glBufferData (GL_ARRAY_BUFFER, sizeof (VertexFlippedTexcoord), VertexFlippedTexcoord, GL_STATIC_DRAW);
	glVertexAttribPointer (textureFlippedCoordLoc, 2, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray (textureFlippedCoordLoc);

	glBindBuffer (GL_ARRAY_BUFFER, vboIds[2]);
	glBufferData (GL_ARRAY_BUFFER, sizeof (VertexTexcoord), VertexTexcoord, GL_STATIC_DRAW);
}

void videoRenderer::seek (int timecode) {
	if (!initialized)
		return;

	pause ();

	// stop everything
	decodeO->stop ();
	renderO->stop ();

	// flush queues
	decodeQueue->flush();
	renderQueue->flush();

	// seek video
	video->seek (timecode);

	// restart threads
	decodeO->start ();
	renderO->start ();
}

void videoRenderer::play (int timecode) {
	if (!initialized)
		return;

	presentO->play (timecode);
}

void videoRenderer::pause () {
	if (!initialized)
		return;

	presentO->pause ();
}
