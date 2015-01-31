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

	internalType = pFormat::INT10;
	bitdepth = 8;
}

videoRenderer::~videoRenderer () {
	if (initialized) {
		decoding = false;
		uploading = false;
		rendering = false;
		playing = false;

		decodeThread.join ();
		uploadThread.join ();
		renderThread.join ();

		delete decodeQueue;
		delete uploadQueue;
		delete renderQueue;

		delete displayCurr;

		delContexts ();

		glDeleteBuffers (3, vboIds);
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

void videoRenderer::setRefreshRate (int fps) {
	displayRefreshRate = fps;
}

bool videoRenderer::init () {
	// check version
	GLint GLversion = 0;
	glGetIntegerv (GL_MAJOR_VERSION, &GLversion);
	if (GLversion < 3)
		return false;
	LOGD ("Version: OK");

	if (!info->init)
		return false;
	LOGD ("Video decoder: OK");

	genContexts ();
	setAspect ();

	// check extenions
	if (!checkExtensions ())
		return false;
	LOGD ("Extensions: OK");

	// load shaders
	const char* displayVP =
		#include "shaders/displayVert.h"
	const char* displayFP =
		#include "shaders/displayFrag.h"

	shader displayShader (displayVP, displayFP);
	GLuint displaySP = displayShader.loadProgram ();
	if (!displaySP)
		return false;
	LOGD ("Shaders: OK");

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

	// set texture unit
	GLint displayTLoc = glGetUniformLocation (displaySP, "video");
	glUseProgram (displaySP);
	glUniform1i (displayTLoc, 0);

	// start threads
	decodeQueue = new queue<frameCPU> (8, info);
	decodeThread = std::thread (&videoRenderer::decode, this);

	uploadQueue = new queue<frameGPUu> (8, info);
	uploadThread = std::thread (&videoRenderer::upload, this);

	renderQueue = new queue<frameGPUo> (8, info);
	renderThread = std::thread (&videoRenderer::render, this);

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
	int rectangle[4];
	glGetIntegerv (GL_VIEWPORT, rectangle);
	surfaceWidth = rectangle[2] - rectangle[0];
	surfaceHeight = rectangle[3] - rectangle[1];

	int mode = 1;
	switch (mode) {
		// stretch
		case 0:
			info->targetX = 0;
			info->targetY = 0;
			info->targetWidth = info->width;
			info->targetHeight = info->height;
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

void videoRenderer::genContexts () {
	mainContext = eglGetCurrentContext ();
	display = eglGetDisplay (EGL_DEFAULT_DISPLAY);

	EGLConfig config;
	EGLint numConfigs;
	EGLint attribListCfg[] = {EGL_SURFACE_TYPE, EGL_PBUFFER_BIT, EGL_NONE};
	EGLint attribListCtx[] = {EGL_CONTEXT_CLIENT_VERSION, 3, EGL_NONE};
	EGLint attribListSrf[] = {EGL_WIDTH, 1, EGL_HEIGHT, 1, EGL_NONE};
	eglChooseConfig (display, attribListCfg, &config, 1, &numConfigs);
	uploadContext = eglCreateContext (display, config, mainContext, attribListCtx);
	uploadPBuffer = eglCreatePbufferSurface (display, config, attribListSrf);
	uploadPBuffer2 = eglCreatePbufferSurface (display, config, attribListSrf);

	renderContext = eglCreateContext (display, config, mainContext, attribListCtx);
	renderPBuffer = eglCreatePbufferSurface (display, config, attribListSrf);
	renderPBuffer2 = eglCreatePbufferSurface (display, config, attribListSrf);
}

void videoRenderer::delContexts () {
	eglDestroySurface (display, uploadPBuffer);
	eglDestroySurface (display, uploadPBuffer2);
	eglDestroyContext (display, uploadContext);

	eglDestroySurface (display, renderPBuffer);
	eglDestroySurface (display, renderPBuffer2);
	eglDestroyContext (display, renderContext);
}

void videoRenderer::drawFrame () {
	glFinish ();
	glClear (GL_COLOR_BUFFER_BIT);

	if (initialized) {
		if (playing) {
			int tc = tcNow ();
			while (repeat <= 0) {
				getNextFrame (displayCurr);
			}

			if (displayCurr->timecode < tc + softLate) {
				if (displayCurr->timecode < tc + hardLate) {
					// if very late - drop current frame
					LOGD ("hard drop (repeat: %i)", repeat);
					repeat -= videoFps;
				} else if (newFrame && (repeat >= repeatLim)) {
					// if just a bit late - try to find best frame to drop (that is repeated more times than others)
					LOGD ("soft drop (repeat: %i)", repeat);
					repeat -= videoFps;
				}
			} else if (displayCurr->timecode > tc + softEarly) {
				if (displayCurr->timecode > tc + hardEarly) {
					// if very early - repeat current frame
					LOGD ("hard repeat (repeat: %i)", repeat);
					repeat += videoFps;
				} else if (newFrame && (repeat <= repeatLim)) {
					// if just a bit early - try to find best frame to repeat (that is repeated less times than others)
					LOGD ("soft repeat (repeat: %i)", repeat);
					repeat += videoFps;
				}
			}

			presentFrame (displayCurr);
			if (uploadQueue->isEmpty () && !uploading)
				playing = false;
		} else {
			glBindTexture (GL_TEXTURE_2D, displayCurr->plane);
			glDrawArrays (GL_TRIANGLE_STRIP, 0, 4);
		}
	}
}

void videoRenderer::presentFrame (frameGPUo* f) {
	// LOGD ("frame %i timecode %i now %i repeat %i", frameNumber, f->timecode, tcNow (), repeat);
	glBindTexture (GL_TEXTURE_2D, f->plane);
	glDrawArrays (GL_TRIANGLE_STRIP, 0, 4);

	presentedFrames++;
	newFrame = false;

	repeat -= videoFps;
}

void videoRenderer::getNextFrame (frameGPUo* f) {
	if (!renderQueue->isEmpty ()) {
		renderQueue->pop (*f);

		frameNumber++;
		newFrame = true;
	}

	repeat += displayRefreshRate;
}

void videoRenderer::seek (int timestamp) {
	if (initialized) {
		// stop everything
		decoding = false;
		uploading = false;
		rendering = false;
		playing = false;

		decodeThread.join ();
		uploadThread.join ();
		renderThread.join ();

		// flush queues
		decodeQueue->flush();
		uploadQueue->flush();
		renderQueue->flush();

		// seek video
		video->seek (timestamp);

		// restart threads
		decodeThread = std::thread (&videoRenderer::decode, this);
		uploadThread = std::thread (&videoRenderer::upload, this);
		renderThread = std::thread (&videoRenderer::render, this);
	}
}

void videoRenderer::play (int timecode) {
	start = nanotime () - (int64_t) timecode * 1000000LL;
	presentedFrames = 0;
	repeat = 0;
	frameNumber = 0;
	playing = true;
}

void videoRenderer::pause () {
	playing = false;
}

void videoRenderer::decode () {
	frameCPU t (info);
	int size = info->width * info->height * info->Bpp / 8;

	decoding = true;
	while (decoding) {
		if (!decodeQueue->isFull ()) {
			t.timecode = video->getNextVideoframe (t.plane, size);

			switch (t.timecode) {
				case -1:
					// stop playback
					decoding = false;
					break;
				case -2:
					// try later
					usleep (10000);
					break;
				default:
					decodeQueue->push (t);
			}
		} else {
			usleep (10000);
		}
	}
}

void videoRenderer::upload () {
	eglMakeCurrent (display, uploadPBuffer, uploadPBuffer2, uploadContext);

	frameCPU from (info);
	frameGPUu to (info);

	uploading = true;
	while (uploading) {
		if (!decodeQueue->isEmpty () && !uploadQueue->isFull ()) {
			decodeQueue->pop (from);

			to.timecode = from.timecode;

			glBindTexture (GL_TEXTURE_2D, to.plane[0]);
			glTexSubImage2D (GL_TEXTURE_2D, 0, 0, 0, info->width, info->height,
				info->lumaFormat, info->lumaType, (GLvoid*) from.plane);

			if (info->planes > 1) {
				glBindTexture (GL_TEXTURE_2D, to.plane[1]);
				glTexSubImage2D (GL_TEXTURE_2D, 0, 0, 0, info->chromaWidth, info->chromaHeight,
					info->chromaFormat, info->chromaType, (GLvoid*) (from.plane + info->offset1));
			}

			if (info->planes > 2) {
				glBindTexture (GL_TEXTURE_2D, to.plane[2]);
				glTexSubImage2D (GL_TEXTURE_2D, 0, 0, 0, info->chromaWidth, info->chromaHeight,
					info->chromaFormat, info->chromaType, (GLvoid*) (from.plane + info->offset2));
			}

			glFlush ();

			uploadQueue->push (to);
		} else if (decodeQueue->isEmpty () && !decoding) {
			uploading = false;
		} else {
			usleep (10000);
		}
	}

	eglMakeCurrent (display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
}

void videoRenderer::render () {
	eglMakeCurrent (display, renderPBuffer, renderPBuffer2, renderContext);
	srand (time (NULL));

	// create FBO
	GLuint framebuffer;
	glGenFramebuffers (1, &framebuffer);
	glBindFramebuffer (GL_FRAMEBUFFER, framebuffer);

	// load coordinates
	const GLuint vertexCoordLoc = 0;
	glBindBuffer (GL_ARRAY_BUFFER, vboIds[0]);
	glVertexAttribPointer (vertexCoordLoc, 4, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray (vertexCoordLoc);

	const GLuint textureCoordLoc = 1;
	glBindBuffer (GL_ARRAY_BUFFER, vboIds[2]);
	glVertexAttribPointer (textureCoordLoc, 2, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray (textureCoordLoc);

	renderInit ();

	// dither
	frameGPUi dither (32, 32, pFormat::DITHER, info);
	GLint ditherOffset;
	GLuint renderDitherSP; {
		#include "ditherMatrix.h"

		glActiveTexture (GL_TEXTURE0 + 3);
		glBindTexture (GL_TEXTURE_2D, dither.plane);
		glTexSubImage2D (GL_TEXTURE_2D, 0, 0, 0, 32, 32, GL_RGBA, GL_UNSIGNED_INT_2_10_10_10_REV, (GLvoid*) ditherMatrix);
		glActiveTexture (GL_TEXTURE0);

		const char* renderVP =
			#include "shaders/displayVert.h"
		const char* renderDitherFP =
			#include "shaders/dither.h"

		shader renderDitherShader (renderVP, renderDitherFP, precision);
		renderDitherSP = renderDitherShader.loadProgram ();

		glUseProgram (renderDitherSP);
		glUniform1i (glGetUniformLocation (renderDitherSP, "video"), 0);
		glUniform1i (glGetUniformLocation (renderDitherSP, "dither"), 3);

		glUniform2f (glGetUniformLocation (renderDitherSP, "depth"),
			(float) (pow (2.0, bitdepth) - 1.0),
			(float) (1.0 / (pow (2.0, bitdepth) - 1.0)));
		glUniform2f (glGetUniformLocation (renderDitherSP, "resize"),
			(float) ((double) info->targetWidth / 32.0),
			(float) ((double) info->targetHeight / 32.0));
		ditherOffset = glGetUniformLocation (renderDitherSP, "offset");
	}

	frameGPUu from (info);
	frameGPUo to (info);

	rendering = true;
	while (rendering) {
		if (!uploadQueue->isEmpty () && !renderQueue->isFull ()) {
			uploadQueue->pop (from);

			to.timecode = from.timecode;

			for (int i = 0; i < info->planes; i++) {
				glActiveTexture (GL_TEXTURE0 + i);
				glBindTexture (GL_TEXTURE_2D, from.plane[i]);
			}

			for (rPass::iterator passIt = pass.begin (); passIt != pass.end (); ++passIt)
				(*passIt)->execute ();

			glViewport (0, 0, info->targetWidth, info->targetHeight);
			glFramebufferTexture2D (GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, to.plane, 0);
			glUseProgram (renderDitherSP);
			glUniform3f (ditherOffset,
				(float) ((rand() % 32) / 32.0),
				(float) ((rand() % 32) / 32.0),
				(float) (rand() % 3));
			glDrawArrays (GL_TRIANGLE_STRIP, 0, 4);

			glFlush ();

			renderQueue->push (to);
		} else if (uploadQueue->isEmpty () && !uploading) {
			rendering = false;
		} else {
			usleep (10000);
		}
	}

	for (rPass::iterator passIt = pass.begin (); passIt != pass.end (); ++passIt)
		delete *passIt;

	glDeleteFramebuffers (1, &framebuffer);
	eglMakeCurrent (display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
}

void videoRenderer::renderInit () {
	const char* renderVP =
		#include "shaders/displayVert.h"

	// convert to internal format
	switch (info->planes) {
		case 3: {
			const char* render08ToInternalFP =
				#include "shaders/planar08ToInternal.h"
			const char* render16ToInternalFP =
				#include "shaders/planar16ToInternal.h"

			shader renderToInternalShader (renderVP,
				info->lumaFormat == GL_RED ? render08ToInternalFP : render16ToInternalFP,
				"highp", info->halfWidth && info->hwChroma ? "#define HWCHROMA" : "");
			GLuint renderToInternalSP = renderToInternalShader.loadProgram ();

			glUseProgram (renderToInternalSP);
			glUniform1i (glGetUniformLocation (renderToInternalSP, "Y"),  0);
			glUniform1i (glGetUniformLocation (renderToInternalSP, "Cb"), 1);
			glUniform1i (glGetUniformLocation (renderToInternalSP, "Cr"), 2);
			if (info->halfWidth && info->hwChroma)
				glUniform1f (glGetUniformLocation (renderToInternalSP, "pitch"), (float) (0.5 / info->width));

			pass.push_back (new renderingPass (
				new frameGPUi (info->width, info->height, internalType, info),
				renderToInternalSP, 0));

			break;
		}

		case 1: {
			const char* renderRgbToInteralFP =
				#include "shaders/displayFrag.h"

			shader renderToInternalShader (renderVP, renderRgbToInteralFP, "highp");
			GLuint renderToInternalSP = renderToInternalShader.loadProgram ();

			glUseProgram (renderToInternalSP);
			glUniform1i (glGetUniformLocation (renderToInternalSP, "video"), 0);

			pass.push_back (new renderingPass (
				new frameGPUi (info->width, info->height, internalType, info),
				renderToInternalSP, 0));

			break;
		}
	}

	// convert 4:2:0 -> 4:2:2
	if (info->halfHeight && !info->hwChroma) {
		const char* render420to422FP =
			#include "shaders/up420to422.h"

		shader render420to422Shader (renderVP, render420to422FP, precision);
		GLuint render420to422SP = render420to422Shader.loadProgram ();

		glUseProgram (render420to422SP);
		glUniform1i (glGetUniformLocation (render420to422SP, "YCbCr"), 0);
		glUniform4f (glGetUniformLocation (render420to422SP, "pitch"),
			(float) ( 2.0 / info->height), (float) (1.0 / info->height),
			(float) (-0.5 / info->height), (float) (0.5 * info->height));

		pass.push_back (new renderingPass (
			new frameGPUi (info->chromaWidth, info->height, internalType, info),
			render420to422SP, 1));
	}

	// convert 4:2:2 -> 4:4:4
	if (info->halfWidth && !info->hwChroma) {
		const char* render422to444FP =
			#include "shaders/up422to444.h"

		shader render422to444Shader (renderVP, render422to444FP, precision, info->halfHeight ? "#define SEPARATE" : "");
		GLuint render422to444SP = render422to444Shader.loadProgram ();

		glUseProgram (render422to444SP);
		glUniform1i (glGetUniformLocation (render422to444SP, "YCbCr"), 0);
		if (info->halfHeight)
			glUniform1i (glGetUniformLocation (render422to444SP, "CbCr"), 1);
		glUniform1f (glGetUniformLocation (render422to444SP, "pitch"), (float) (1.0 / info->width));

		pass.push_back (new renderingPass (
			new frameGPUi (info->width, info->height, internalType, info),
			render422to444SP, 0));
	}

	// debanding
	bool debanding = false;
	const double debandThreshold = 16.0;

	if (debanding) {
		const GLfloat blurWeight[] = {
			0.0886884268, 0.1111888680, 0.0959098625, 0.0760291736,
			0.0553875087, 0.0370815002, 0.0228147565, 0.0128999036};
		const GLfloat blurOffsetX[] = {
			(float)  0.6643036225 / info->width, (float)  2.4867343814 / info->width,
			(float)  4.4761344303 / info->width, (float)  6.4655559368 / info->width,
			(float)  8.4550083353 / info->width, (float) 10.4445009501 / info->width,
			(float) 12.4340429621 / info->width, (float) 14.4236433777 / info->width};
		const GLfloat blurOffsetY[] = {
			(float)  0.6643036225 / info->height, (float)  2.4867343814 / info->height,
			(float)  4.4761344303 / info->height, (float)  6.4655559368 / info->height,
			(float)  8.4550083353 / info->height, (float) 10.4445009501 / info->height,
			(float) 12.4340429621 / info->height, (float) 14.4236433777 / info->height};
		const int blurTaps = sizeof (blurWeight) / sizeof (*blurWeight);

		const char* renderBlurXFP =
			#include "shaders/blurX.h"

		shader renderBlurXShader (renderVP, renderBlurXFP, precision, blurTaps);
		GLuint renderBlurXSP = renderBlurXShader.loadProgram ();

		glUseProgram (renderBlurXSP);
		glUniform1i  (glGetUniformLocation (renderBlurXSP, "video"), 0);
		glUniform1fv (glGetUniformLocation (renderBlurXSP, "weight"), blurTaps, blurWeight);
		glUniform1fv (glGetUniformLocation (renderBlurXSP, "offset"), blurTaps, blurOffsetX);

		pass.push_back (new renderingPass (
			new frameGPUi (info->width, info->height, internalType, info),
			renderBlurXSP, 1));

		const char* renderBlurYFP =
			#include "shaders/blurY.h"

		shader renderBlurYShader (renderVP, renderBlurYFP, precision, blurTaps);
		GLuint renderBlurYSP = renderBlurYShader.loadProgram ();

		glUseProgram (renderBlurYSP);
		glUniform1i  (glGetUniformLocation (renderBlurYSP, "video"), 1);
		glUniform1fv (glGetUniformLocation (renderBlurYSP, "weight"), blurTaps, blurWeight);
		glUniform1fv (glGetUniformLocation (renderBlurYSP, "offset"), blurTaps, blurOffsetY);

		pass.push_back (new renderingPass (
			new frameGPUi (info->width, info->height, internalType, info),
			renderBlurYSP, 1));

		const char* renderDebandFP =
			#include "shaders/deband.h"

		shader renderDebandShader (renderVP, renderDebandFP, precision);
		GLuint renderDebandSP = renderDebandShader.loadProgram ();

		glUseProgram (renderDebandSP);
		glUniform1i (glGetUniformLocation (renderDebandSP, "video"), 0);
		glUniform1i (glGetUniformLocation (renderDebandSP, "blur"), 1);
		glUniform3f (glGetUniformLocation (renderDebandSP, "threshold"),
			(float) (debandThreshold / 255.0), (float) (debandThreshold / 255.0), (float) (debandThreshold / 255.0));

		pass.push_back (new renderingPass (
			new frameGPUi (info->width, info->height, internalType, info),
			renderDebandSP, 0));
	}

	// convert YCbCr -> RGB
	if (info->fourCC != pFormat::RGBA) {
		const char* renderYuvToRgbFP =
			#include "shaders/yuvToRgb.h"

		shader renderYuvToRgbShader (renderVP, renderYuvToRgbFP, precision);
		GLuint renderYuvToRgbSP = renderYuvToRgbShader.loadProgram ();

		glUseProgram (renderYuvToRgbSP);
		glUniform1i			(glGetUniformLocation (renderYuvToRgbSP, "video"),		0);
		glUniformMatrix3fv	(glGetUniformLocation (renderYuvToRgbSP, "conversion"), 1, GL_FALSE, info->colorConversion);
		glUniform3fv		(glGetUniformLocation (renderYuvToRgbSP, "offset"),		1, info->colorOffset);

		pass.push_back (new renderingPass (
			new frameGPUi (info->width, info->height, internalType, info),
			renderYuvToRgbSP, 0));
	}

	// scale width
	if (info->targetWidth != info->width && !info->hwScale) {
		GLuint renderScaleWidthSP;
		if (info->targetWidth > info->width) {

		} else {

		}

		pass.push_back (new renderingPass (
			new frameGPUi (info->targetWidth, info->targetHeight, internalType, info),
			renderScaleWidthSP, 0));
	}

	// scale height
	if (info->targetHeight != info->height && !info->hwScale) {
		GLuint renderScaleHeightSP;
		if (info->targetHeight > info->height) {

		} else {

		}

		pass.push_back (new renderingPass (
			new frameGPUi (info->width, info->targetHeight, internalType, info),
			renderScaleHeightSP, 0));
	}

	LOGD ("Rendering chain: OK");
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
