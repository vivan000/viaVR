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
#include "shader.h"

videoRenderer::videoRenderer () {
	glClearColor (0.0f, 0.0f, 0.0f, 0.0f);
	glClear (GL_COLOR_BUFFER_BIT);
	initialized = false;
}

videoRenderer::~videoRenderer () {
	LOGD ("Destroing videoRenderer");
	if (initialized) {
		playing = false;

		delete decodeO;
		delete uploadO;
		delete renderO;

		drawThread.join ();

		eglMakeCurrent (display, mainSurface, mainSurface, mainContext);

		delete decodeQueue;
		delete uploadQueue;
		delete renderQueue;

		delete displayCurr;
		glDeleteBuffers (3, vboIds);

		eglMakeCurrent (display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);

		delContexts ();
	}

	delete info;
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

int64_t videoRenderer::nanotime () {
	struct timespec now;
	clock_gettime (CLOCK_MONOTONIC, &now);
	return (int64_t) now.tv_sec * 1000000000LL + now.tv_nsec;
}

int videoRenderer::tcNow () {
	return (int) ((nanotime () - start) / 1000000);
}

bool videoRenderer::addVideoDecoder (IVideoDecoder* video) {
	videoRenderer::video = video;
	info = new videoInfo (video->getWidth (), video->getHeight (),
		video->getFourCC (), video->getRange (), video->getMatrix ());

	if (!info->init)
		return false;

	videoSarWidth =		video->getSarWidth ();
	videoSarHeight =	video->getSarHeight ();
	videoFps = 			round ((double) video->getFpsNumerator () / video->getFpsDenominator ());

	LOGD ("Video info:");
	LOGD ("Video %ix%i (%i:%i)", info->width, info->height, videoSarWidth, videoSarHeight);
	LOGD (reinterpret_cast <char*> (&info->fourCC)[2] > 16 ? "FourCC: %c%c%c%c\n" : "FourCC: %c%c[%i][%i]\n",
		reinterpret_cast <char*> (&info->fourCC)[0], reinterpret_cast <char*> (&info->fourCC)[1],
		reinterpret_cast <char*> (&info->fourCC)[2], reinterpret_cast <char*> (&info->fourCC)[3]);
	LOGD ("Matrix: %s (%s)", info->matrix == pMatrix::BT709 ? "BT.709" : "BT.601", video->getMatrix () != 0 ? "upstream" : "guess");
	LOGD ("Range: %s (%s)",	info->range == pRange::TV ? "TV" : "PC", video->getRange () != 0 ? "upstream" : "guess");
	LOGD ("Framerate: %i/%i", video->getFpsNumerator (), video->getFpsDenominator ());

	info->hwChroma = true;
	info->hwChromaLinear = true;
	info->hwScale = true;
	info->hwScaleLinear = true;

	LOGD ("Video decoder connected");

	return true;
}

bool videoRenderer::addWindow (ANativeWindow* window) {
	const EGLint attribListWindowCfg[] = {
		EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
		EGL_BLUE_SIZE, 8,
		EGL_GREEN_SIZE, 8,
		EGL_RED_SIZE, 8,
		EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
		EGL_CONFORMANT, EGL_OPENGL_ES2_BIT,
		EGL_NONE};

	const EGLint attribListPbufCfg[] = {
		EGL_SURFACE_TYPE, EGL_PBUFFER_BIT,
		EGL_NONE};

	const EGLint attribListContext[] = {
		EGL_CONTEXT_CLIENT_VERSION, 3,
		EGL_NONE};

	const EGLint attribListPbufSrf[] = {
		EGL_WIDTH, 1,
		EGL_HEIGHT, 1,
		EGL_NONE};

	EGLConfig config;
	EGLint numConfigs;
	EGLint format;

	display = eglGetDisplay (EGL_DEFAULT_DISPLAY);
	if (display == EGL_NO_DISPLAY) {
		LOGE ("GetDisplay error: %s", getEglErrorStr ());
		return false;
	}

	if (!eglInitialize (display, 0, 0)) {
		LOGE ("Initialize error: %s", getEglErrorStr ());
		return false;
	}

	if (!eglChooseConfig (display, attribListWindowCfg, &config, 1, &numConfigs)) {
		LOGE ("ChooseConfig error: %s", getEglErrorStr ());
		return false;
	}

	if (!eglGetConfigAttrib (display, config, EGL_NATIVE_VISUAL_ID, &format)) {
		LOGE ("GetConfigAttrib error: %s", getEglErrorStr ());
		return false;
	}

	ANativeWindow_setBuffersGeometry (window, 0, 0, format);

	mainSurface = eglCreateWindowSurface (display, config, window, 0);
	if (!mainSurface) {
		LOGE ("CreateWindowSurface error: %s", getEglErrorStr ());
		return false;
	}

	mainContext = eglCreateContext (display, config, 0, attribListContext);
	if (!mainContext) {
		LOGE ("CreateContext error: %s", getEglErrorStr ());
		return false;
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

	if (!eglChooseConfig (display, attribListPbufCfg, &config, 1, &numConfigs)) {
		LOGE ("Pbuffer ChooseConfig error: %s", getEglErrorStr ());
		return false;
	}

	uploadPbuffer = eglCreatePbufferSurface (display, config, attribListPbufSrf);
	if (!uploadPbuffer) {
		LOGE ("Upload pbuffer CreatePbufferSurface error: %s", getEglErrorStr ());
		return false;
	}

	uploadContext = eglCreateContext (display, config, mainContext, attribListContext);
	if (!uploadContext) {
		LOGE ("Upload pbuffer CreateContext error: %s", getEglErrorStr ());
		return false;
	}

	renderPbuffer = eglCreatePbufferSurface (display, config, attribListPbufSrf);
	if (!renderPbuffer) {
		LOGE ("Render pbuffer CreatePbufferSurface error: %s", getEglErrorStr ());
		return false;
	}

	renderContext = eglCreateContext (display, config, mainContext, attribListContext);
	if (!renderContext) {
		LOGE ("Render pbuffer CreateContext error: %s", getEglErrorStr ());
		return false;
	}

	glClearColor (0.0f, 0.0f, 0.0f, 0.0f);
	glDisable (GL_DITHER);

	eglMakeCurrent (display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
	return true;
}

void videoRenderer::setRefreshRate (int fps) {
	displayRefreshRate = fps;
}

bool videoRenderer::init () {
	eglMakeCurrent (display, mainSurface, mainSurface, mainContext);

	srand (time (NULL));

	// check version
	GLint GLversion = 0;
	glGetIntegerv (GL_MAJOR_VERSION, &GLversion);
	if (GLversion < 3)
		return false;
	LOGD ("Version: OK");

	if (!info->init)
		return false;
	LOGD ("Video decoder: OK");

	setAspect ();

	// check extenions
	if (!checkExtensions ())
		return false;
	LOGD ("Extensions: OK");

	// load coordinates
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

	// load shaders
	const char* displayVP =
		#include "shaders/displayVert.h"
	const char* displayFP =
		#include "shaders/displayFrag.h"

	shader displayShader (displayVP, displayFP);
	GLuint displaySP = displayShader.loadProgram ();
	if (!displaySP)
		return false;

	GLint displayTLoc = glGetUniformLocation (displaySP, "video");
	glUseProgram (displaySP);
	glUniform1i (displayTLoc, 0);
	LOGD ("Display shader: OK");

	// start threads
	decodeQueue = new queue<frameCPU> (8, info);
	uploadQueue = new queue<frameGPUu> (8, info);
	renderQueue = new queue<frameGPUo> (8, info);

	eglMakeCurrent (display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
	decodeO = new decoder (info, decodeQueue, video);
	uploadO = new uploader (info, decodeQueue, uploadQueue, display, uploadPbuffer, uploadContext);
	renderO = new renderer (info, uploadQueue, renderQueue, display, renderPbuffer, renderContext, vboIds);
	eglMakeCurrent (display, mainSurface, mainSurface, mainContext);

	// wait till there's at least 1 frame to show
	while (renderQueue->isEmpty ()) {
		usleep (10000);
	}

	// create display texture
	displayCurr = new frameGPUo (info);

	hardLate = -2000 / videoFps;
	softLate = 0;
	softEarly = 2000 / videoFps;
	hardEarly = 4000 / videoFps;
	repeatLim = displayRefreshRate / videoFps * videoFps;

	initialized = true;
	LOGD ("Initialization: OK");

	eglMakeCurrent (display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
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
	}

	// half-float texture is target and not supported
	if ((precisionTex == 1) && !extColorHalfFloat)
		return false;

	// float texture is target and not supported
	if ((precisionTex == 2) && !extColorFloat)
		return false;
/*
	if (extBinningControl)
		glHint (0x8FB0, 0x8FB3); // BINNING_CONTROL_HINT_QCOM, RENDER_DIRECT_TO_FRAMEBUFFER_QCOM

	if (extWriteOnly)
		glEnable (0x8823); // WRITEONLY_RENDERING_QCOM
*/
	glDisable (GL_DITHER);

	LOGD ("Extensions:");
	LOGD ("Target texture: %i, target processing: %s", precisionTex, precisionHighp ? "highp" : "mediump");
	LOGD ("Half-float texture: %s, bilinear: %s",
		extColorHalfFloat ? "supported" : "not supported",
		extHalfFloatLinear  ? "supported" : "not supported");
	LOGD ("Float texture: %s, bilinear: %s",
		extColorFloat ? "supported" : "not supported",
		extFloatLinear  ? "supported" : "not supported");
	LOGD ("Binning control: %s", extBinningControl ? "supported" : "not supported");
	LOGD ("Write-only rendering: %s", extWriteOnly ? "supported" : "not supported");
	return true;
}

void videoRenderer::setAspect () {
	int mode = 1;
	switch (mode) {
		// stretch
		case 0:
			info->targetX = 0;
			info->targetY = 0;
			info->targetWidth = surfaceWidth;
			info->targetHeight = surfaceHeight;
			break;

		// touch from inside
		case 1:
			if ((long long) surfaceWidth * info->height * videoSarHeight < (long long) info->width * videoSarWidth * surfaceHeight) {
				info->targetX = 0;
				info->targetWidth = surfaceWidth;
				info->targetHeight = (long long) surfaceWidth * info->height * videoSarHeight / info->width / videoSarWidth;
				info->targetY = (surfaceHeight - info->targetHeight) / 2;
			} else {
				info->targetY = 0;
				info->targetHeight = surfaceHeight;
				info->targetWidth = (long long) surfaceHeight * info->width * videoSarWidth / info->height / videoSarHeight;
				info->targetX = (surfaceWidth - info->targetWidth) / 2;
			}
			break;

		// touch from outside
		case 2:
			if ((long long) surfaceWidth * info->height * videoSarHeight > (long long) info->width * videoSarWidth * surfaceHeight) {
				info->targetX = 0;
				info->targetWidth = surfaceWidth;
				info->targetHeight = (long long) surfaceWidth * info->height * videoSarHeight / info->width / videoSarWidth;
				info->targetY = (surfaceHeight - info->targetHeight) / 2;
			} else {
				info->targetY = 0;
				info->targetHeight = surfaceHeight;
				info->targetWidth = (long long) surfaceHeight * info->width * videoSarWidth / info->height / videoSarHeight;
				info->targetX = (surfaceWidth - info->targetWidth) / 2;
			}
			break;
	}

	glViewport (info->targetX, info->targetY, info->targetWidth, info->targetHeight);
}

void videoRenderer::delContexts () {
	eglDestroySurface (display, uploadPbuffer);
	eglDestroyContext (display, uploadContext);

	eglDestroySurface (display, renderPbuffer);
	eglDestroyContext (display, renderContext);

	eglDestroySurface (display, mainSurface);
	eglDestroyContext (display, mainContext);
}

void videoRenderer::drawFrame () {
	eglMakeCurrent (display, mainSurface, mainSurface, mainContext);

	while (playing) {
		glClear (GL_COLOR_BUFFER_BIT);

		int tc = tcNow ();
		while (repeat <= 0) {
			getNextFrame (displayCurr);
		}

		if (displayCurr->timecode < tc + softLate) {
			if (displayCurr->timecode < tc + hardLate) {
				// if very late - drop current frame
				LOGI ("hard drop (repeat: %i)", repeat / videoFps);
				repeat -= videoFps;
			} else if (newFrame && (repeat >= displayRefreshRate)) {
				// if just a bit late - try to find best frame to drop (that is repeated more times than others)
				LOGD ("soft drop (repeat: %i)", repeat / videoFps);
				repeat -= videoFps;
			}
		} else if (displayCurr->timecode > tc + softEarly) {
			if (displayCurr->timecode > tc + hardEarly) {
				// if very early - repeat current frame
				LOGI ("hard repeat (repeat: %i)", repeat / videoFps);
				repeat += videoFps;
			} else if (newFrame && (repeat <= repeatLim)) {
				// if just a bit early - try to find best frame to repeat (that is repeated less times than others)
				LOGD ("soft repeat (repeat: %i)", repeat / videoFps);
				repeat += videoFps;
			}
		}

		presentFrame (displayCurr);

		eglSwapBuffers (display, mainSurface);
	}

	eglMakeCurrent (display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
}

void videoRenderer::presentFrame (frameGPUo* f) {
	int64_t now = nanotime ();
	double deltaPrev = (now - prev) / 1000000.0;
	double deltaPrev2 = (now - prev2) / 1000000.0;
	prev2 = prev;
	prev = now;

	if (presentedFrames > 1) {
		if (deltaPrev2 >= 50.0)
			LOGE ("display drop %5.2f ms", deltaPrev2);
		//LOGD ("frame %3i timecode %5i now %5i (+%5.2f +%5.2f) repeat %2i",
		//	frameNumber, f->timecode, tcNow (), deltaPrev, deltaPrev2, repeat);
	}

	glBindTexture (GL_TEXTURE_2D, f->plane);
	glDrawArrays (GL_TRIANGLE_STRIP, 0, 4);

	presentedFrames++;
	newFrame = false;

	repeat -= videoFps;
}

void videoRenderer::getNextFrame (frameGPUo* f) {
	if (!renderQueue->isEmpty ()) {
		renderQueue->pop (f);

		frameNumber++;
		newFrame = true;
	}

	repeat += displayRefreshRate;
}

void videoRenderer::seek (int timestamp) {
	if (initialized) {
		// stop everything
		decodeO->stop ();
		uploadO->stop ();
		renderO->stop ();

		// flush queues
		decodeQueue->flush();
		uploadQueue->flush();
		renderQueue->flush();

		// seek video
		video->seek (timestamp);

		// restart threads
		decodeO->start ();
		uploadO->start ();
		renderO->start ();
	}
}

void videoRenderer::play (int timecode) {
	start = nanotime () - (int64_t) timecode * 1000000LL;
	presentedFrames = 0;
	repeat = 0;
	frameNumber = 0;
	playing = true;

	drawThread = std::thread (&videoRenderer::drawFrame, this);
}

void videoRenderer::pause () {
	playing = false;

	drawThread.join ();
}

void videoRenderer::getGlError () {
	GLenum err;
	while ((err = glGetError()) != GL_NO_ERROR)
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
		};
}

void videoRenderer::getFbStatus () {
	GLenum err = glCheckFramebufferStatus (GL_FRAMEBUFFER);
	switch (err) {
		case GL_FRAMEBUFFER_COMPLETE:
			LOGE ("Framebuffer: GL_FRAMEBUFFER_COMPLETE");
			break;
		case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
			LOGE ("Framebuffer: GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT");
			break;
		case GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS:
			LOGE ("Framebuffer: GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS");
			break;
		case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
			LOGE ("Framebuffer: GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT");
			break;
		case GL_FRAMEBUFFER_UNSUPPORTED:
			LOGE ("Framebuffer: GL_FRAMEBUFFER_UNSUPPORTED");
			break;
	};
}

const char* videoRenderer::getEglErrorStr () {
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
	}

	return "Unknown EGL error";
}
