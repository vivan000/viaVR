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

	videoFps = round ((double) info->fpsNumerator / info->fpsDenominator);
	hardLate = -2000 / videoFps;
	softLate = 0;
	softEarly = 2000 / videoFps;
	hardEarly = 4000 / videoFps;
	repeatLim = cfg->displayRefreshRate / videoFps * videoFps;

	eglMakeCurrent (display, surface, surface, context);
	glClearColor (0.0f, 0.0f, 0.0f, 0.0f);
	glDisable (GL_DITHER);

	const char* displayVP =
		#include "shaders/displayVert.h"
	const char* displayFP =
		#include "shaders/displayFrag.h"

	shader displayShader (displayVP, displayFP);
	displaySP = displayShader.loadProgram ();
	glUseProgram (displaySP);
	glUniform1i (glGetUniformLocation (displaySP, "video"), 0);
	from = new frameGPUo (info, cfg);
	eglMakeCurrent (display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);

	playing = false;
	thread = std::thread (&presenter::present, this);
}

presenter::~presenter () {
	working = false;
	thread.join ();

	eglMakeCurrent (display, surface, surface, context);
	delete from;
	glUseProgram (0);
	glDeleteProgram (displaySP);
	eglMakeCurrent (display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
}

void presenter::play (int timecode) {
	start = nanotime () - (int64_t) timecode * 1000000LL;
	presentedFrames = 0;
	repeat = 0;
	frameNumber = 0;
	playing = true;
}

void presenter::pause () {
	playing = false;
}

void presenter::present () {
	eglMakeCurrent (display, surface, surface, context);

	working = true;
	while (working) {
		glClear (GL_COLOR_BUFFER_BIT);

		if (!playing) {
			glBindTexture (GL_TEXTURE_2D, from->plane);
			glDrawArrays (GL_TRIANGLE_STRIP, 0, 4);
		} else {
			int tc = tcNow ();
			while (repeat <= 0)
				getNextFrame ();

			if (from->timecode < tc + softLate) {
				if (from->timecode < tc + hardLate) {
					// if very late - drop current frame
					LOGI ("hard drop (repeat: %i)", repeat / videoFps);
					repeat -= videoFps;
				} else if (newFrame && (repeat >= cfg->displayRefreshRate)) {
					// if just a bit late - try to find best frame to drop (that is repeated more times than others)
					LOGD ("soft drop (repeat: %i)", repeat / videoFps);
					repeat -= videoFps;
				}
			} else if (from->timecode > tc + softEarly) {
				if (from->timecode > tc + hardEarly) {
					// if very early - repeat current frame
					LOGI ("hard repeat (repeat: %i)", repeat / videoFps);
					repeat += videoFps;
				} else if (newFrame && (repeat <= repeatLim)) {
					// if just a bit early - try to find best frame to repeat (that is repeated less times than others)
					LOGD ("soft repeat (repeat: %i)", repeat / videoFps);
					repeat += videoFps;
				}
			}

			presentFrame ();
		}

		eglSwapBuffers (display, surface);
	}

	eglMakeCurrent (display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
}

void presenter::getNextFrame () {
	if (!renderQueue->isEmpty ()) {
		renderQueue->pop (from);

		frameNumber++;
		newFrame = true;
	}

	repeat += cfg->displayRefreshRate;
}

void presenter::presentFrame () {
	int64_t now = nanotime ();
	double deltaPrev = (now - prev) / 1000000.0;
	double deltaPrev2 = (now - prev2) / 1000000.0;
	prev2 = prev;
	prev = now;

	if (presentedFrames > 1) {
		if (deltaPrev2 >= 50.0)
			LOGE ("display drop %5.2f ms", deltaPrev2);
	}
	/*
	LOGD ("frame %3i timecode %5i now %5i (+%5.2f +%5.2f) repeat %2i",
		frameNumber, from->timecode, tcNow (), presentedFrames > 0 ? deltaPrev : 0.0,
		presentedFrames > 1 ? deltaPrev2 : 0.0, repeat);
	*/

	glBindTexture (GL_TEXTURE_2D, from->plane);
	glDrawArrays (GL_TRIANGLE_STRIP, 0, 4);

	presentedFrames++;
	newFrame = false;

	repeat -= videoFps;
}

int64_t presenter::nanotime () {
	struct timespec now;
	clock_gettime (CLOCK_MONOTONIC, &now);
	return (int64_t) now.tv_sec * 1000000000LL + now.tv_nsec;
}

int presenter::tcNow () {
	return (int) ((nanotime () - start) / 1000000);
}
