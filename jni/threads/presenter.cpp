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

presenter::presenter (videoInfo* info, config* cfg, queue<frameGPUo>* renderQueue,
		EGLDisplay display, EGLSurface mainSurface, EGLContext mainContext) {
	presenter::info = info;
	presenter::cfg = cfg;
	presenter::renderQueue = renderQueue;

	presenter::display = display;
	presenter::surface = mainSurface;
	presenter::context = mainContext;

	srand (time (NULL));
	videoFps = (double) info->fpsNumerator / info->fpsDenominator;

	eglMakeCurrent (display, surface, surface, context);
	glClearColor (0.0f, 0.0f, 0.0f, 0.0f);
	glDisable (GL_DITHER);

	const char* displayVP =
		#include "shaders/vertex.h"
	const char* displayFP =
		#include "shaders/present.h"

	shaderLoader shader;

	// dither and present 1 frame
	presentOne = shader.loadShaders (displayVP, displayFP, false);
	glUseProgram (presentOne);
	glUniform1i (glGetUniformLocation (presentOne, "video0"), 0);
	glUniform1i (glGetUniformLocation (presentOne, "dither"), 3);
	glUniform2f (glGetUniformLocation (presentOne, "ditherDepth"),
		(float) (pow (2.0, cfg->targetBitdepth) - 1.0),
		(float) (1.0 / (pow (2.0, cfg->targetBitdepth) - 1.0)));
	glUniform2f (glGetUniformLocation (presentOne, "ditherResize"),
		(float) (cfg->targetWidth / 32.0),
		(float) (cfg->targetHeight / 32.0));
	presentOneDither = glGetUniformLocation (presentOne, "ditherOffset");

	// dither and present 2 frames
	presentTwo = shader.loadShaders (displayVP, displayFP, true);
	glUseProgram (presentTwo);
	glUniform1i (glGetUniformLocation (presentTwo, "video0"), 0);
	glUniform1i (glGetUniformLocation (presentTwo, "video1"), 1);
	glUniform1i (glGetUniformLocation (presentTwo, "dither"), 3);
	glUniform2f (glGetUniformLocation (presentTwo, "ditherDepth"),
		(float) (pow (2.0, cfg->targetBitdepth) - 1.0),
		(float) (1.0 / (pow (2.0, cfg->targetBitdepth) - 1.0)));
	glUniform2f (glGetUniformLocation (presentTwo, "ditherResize"),
		(float) (cfg->targetWidth / 32.0),
		(float) (cfg->targetHeight / 32.0));
	presentTwoDither = glGetUniformLocation (presentTwo, "ditherOffset");
	presentTwoFactor = glGetUniformLocation (presentTwo, "blendingFactor");

	// dither
	#include "threads/helpers/ditherMatrix.h"
	dither = new frameGPUi (32, 32, iFormat::DITHER, false);
	glActiveTexture (GL_TEXTURE0 + 3);
	glBindTexture (GL_TEXTURE_2D, dither->plane);
	glTexSubImage2D (GL_TEXTURE_2D, 0, 0, 0, 32, 32, GL_RGBA, GL_UNSIGNED_INT_2_10_10_10_REV, (GLvoid*) ditherMatrix);
	glActiveTexture (GL_TEXTURE0);

	frame0 = new frameGPUo (info, cfg);
	frame1 = new frameGPUo (info, cfg);
	eglMakeCurrent (display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);

	playing = false;
	thread = std::thread (&presenter::present, this);
}

presenter::~presenter () {
	working = false;
	thread.join ();

	eglMakeCurrent (display, surface, surface, context);
	delete frame0;
	delete frame1;
	delete dither;
	glUseProgram (0);
	glDeleteProgram (presentOne);
	glDeleteProgram (presentTwo);
	eglMakeCurrent (display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
}

void presenter::play (int timecode) {
	start = nanotime () - (int64_t) timecode * 1000000LL;
	presentedFrames = 0;
	frameNumber = 0;
	repeat = 0.0;
	factor = (double) cfg->displayRefreshRate / videoFps;
	playing = true;
}

void presenter::pause () {
	playing = false;
}

void presenter::present () {
	eglMakeCurrent (display, surface, surface, context);

	working = !BENCHMARK;
	while (working) {
		glClear (GL_COLOR_BUFFER_BIT);

		if (!playing) {
			glBindTexture (GL_TEXTURE_2D, frame0->plane);
			glDrawArrays (GL_TRIANGLE_STRIP, 0, 4);
		} else {
			int tc = tcNow ();
			while (repeat <= 0.0)
				getNextFrame ();

			presentFrame ();

			if (repeat <= 0.0 && presentedFrames > cfg->displayRefreshRate)
				factor = (double) presentedFrames / frameNumber - (tcNow () - frame0->timecode) / 1000.0;
		}

		eglSwapBuffers (display, surface);
	}

	eglMakeCurrent (display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
}

void presenter::getNextFrame () {
	if (!renderQueue->isEmpty ()) {
		frame0->swap (*frame1);
		renderQueue->pop (frame1);
	} else {
		LOGW ("Render queue is empty");
	}

	frameNumber++;
	repeat += factor;
}

void presenter::presentFrame () {
	int64_t now = nanotime ();
	double deltaPrev = (now - prev) / 1000000.0;
	double deltaPrev2 = (now - prev2) / 1000000.0;
	prev2 = prev;
	prev = now;

	if (presentedFrames > 1) {
		if (deltaPrev2 >= 50.0)
			LOGW ("Display drop %5.2f ms", deltaPrev2);
	}

	if (cfg->logEachFrame)
		LOGD ("%3i tc %5i now %5i (+%5.2f +%5.2f) repeat %4.2f (%4.2f)",
			frameNumber, frame0->timecode, tcNow (),
			presentedFrames > 0 ? deltaPrev : 0.0,
			presentedFrames > 1 ? deltaPrev2 : 0.0,
			repeat, factor);

	glActiveTexture (GL_TEXTURE0);
	glBindTexture (GL_TEXTURE_2D, frame0->plane);

	if (cfg->blending && repeat < 1.0) {
		glActiveTexture (GL_TEXTURE0 + 1);
		glBindTexture (GL_TEXTURE_2D, frame1->plane);
		glUseProgram (presentTwo);
		glUniform1f (presentTwoFactor, repeat);
		glUniform3f (presentTwoDither,
			(float) ((rand() % 32) / 32.0),
			(float) ((rand() % 32) / 32.0),
			(float) (rand() % 3));
	} else {
		glUseProgram (presentOne);
		glUniform3f (presentOneDither,
			(float) ((rand() % 32) / 32.0),
			(float) ((rand() % 32) / 32.0),
			(float) (rand() % 3));
	}

	glDrawArrays (GL_TRIANGLE_STRIP, 0, 4);

	presentedFrames++;
	repeat -= 1.0;
}

int64_t presenter::nanotime () {
	struct timespec now;
	clock_gettime (CLOCK_MONOTONIC, &now);
	return (int64_t) now.tv_sec * 1000000000LL + now.tv_nsec;
}

int presenter::tcNow () {
	return (int) ((nanotime () - start) / 1000000);
}
