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

int64_t nanotime () {
	struct timespec now;
	clock_gettime (CLOCK_MONOTONIC, &now);
	return (int64_t) now.tv_sec * 1000000000LL + now.tv_nsec;
}

renderer::renderer (videoInfo* info, config* cfg, queue<frameGPUu>* uploadQueue, queue<frameGPUo>* renderQueue,
		EGLDisplay display, EGLSurface renderPbuffer, EGLContext renderContext, GLuint* vboIds) {
	renderer::info = info;
	renderer::cfg = cfg;
	renderer::uploadQueue = uploadQueue;
	renderer::renderQueue = renderQueue;

	renderer::display = display;
	renderer::pbuffer = renderPbuffer;
	renderer::context = renderContext;
	renderer::vboIds = vboIds;

	eglMakeCurrent (display, pbuffer, pbuffer, context);
	glClearColor (0.0f, 0.0f, 0.0f, 0.0f);
	glDisable (GL_DITHER);

	from = new frameGPUu (info, cfg);
	to = new frameGPUo (info, cfg);

	if (renderInit ())
		LOGD ("Rendering chain: OK");

	glGenFramebuffers (1, &framebuffer);
	glBindFramebuffer (GL_FRAMEBUFFER, framebuffer);
	eglMakeCurrent (display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);

	start ();
}

renderer::~renderer () {
	if (!joined)
		stop ();

	eglMakeCurrent (display, pbuffer, pbuffer, context);
	glUseProgram (0);
	for (rPass::iterator passIt = pass.begin (); passIt != pass.end (); ++passIt)
		delete *passIt;

	delete from;
	delete to;

	glDeleteFramebuffers (1, &framebuffer);
	eglMakeCurrent (display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
}

void renderer::start () {
	thread = std::thread (&renderer::render, this);
}

void renderer::stop () {
	working = false;
	thread.join ();
	joined = true;
}

void renderer::render () {
	eglMakeCurrent (display, pbuffer, pbuffer, context);

#ifdef PERFOMANCE
	int64_t rstart = nanotime ();
	int rcount = 0;
	uploadQueue->pop (from);
#endif
	joined = false;
	working = true;
	while (working) {
		if (!uploadQueue->isEmpty () && !renderQueue->isFull ()) {
#ifndef PERFOMANCE
			uploadQueue->pop (from);
#endif
			to->timecode = from->timecode;

			for (int i = 0; i < info->planes; i++) {
				glActiveTexture (GL_TEXTURE0 + i);
				glBindTexture (GL_TEXTURE_2D, from->plane[i]);
			}

			for (rPass::iterator passIt = pass.begin (); passIt != pass.end (); ++passIt)
				if (!(*passIt)->dither)
					(*passIt)->execute ();
				else
					(*passIt)->execute (to->plane, cfg->targetWidth, cfg->targetHeight);

			glFlush ();
#ifdef PERFOMANCE
			rcount++;
			if (rcount == 100) {
				LOGD ("Rendering: %6.3f ms", (nanotime () - rstart) / rcount / 1000000.0);
				rcount = 0;
				rstart = nanotime ();
			}
#else
			renderQueue->push (to);
#endif
		} else {
			usleep (10000);
		}
	}

	eglMakeCurrent (display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
}

bool renderer::renderInit () {
	const char* precision = cfg->highp ? "highp" : "mediump";

	// enable coordinates
	const GLuint vertexCoordLoc = 0;
	glBindBuffer (GL_ARRAY_BUFFER, vboIds[0]);
	glVertexAttribPointer (vertexCoordLoc, 4, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray (vertexCoordLoc);

	const GLuint textureCoordLoc = 1;
	glBindBuffer (GL_ARRAY_BUFFER, vboIds[2]);
	glVertexAttribPointer (textureCoordLoc, 2, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray (textureCoordLoc);

	const char* renderVP =
		#include "shaders/displayVert.h"

	LOGD ("Rendering chain:");

	// convert to internal format
	switch (info->planes) {
		case 3: {
			const char* renderToInternalFP =
				#include "shaders/planarToInternal.h"

			shader renderToInternalShader (renderVP, renderToInternalFP,
				precision, info->halfWidth && cfg->hwChroma ? "#define HWCHROMA" : "", info->bitdepth);
			GLuint renderToInternalSP = renderToInternalShader.loadProgram ();
			if (!renderToInternalSP)
				return false;

			glUseProgram (renderToInternalSP);
			glUniform1i (glGetUniformLocation (renderToInternalSP, "Y"),  0);
			glUniform1i (glGetUniformLocation (renderToInternalSP, "Cb"), 1);
			glUniform1i (glGetUniformLocation (renderToInternalSP, "Cr"), 2);
			if (info->halfWidth && cfg->hwChroma)
				glUniform1f (glGetUniformLocation (renderToInternalSP, "pitch"), (float) (0.5 / info->width));

			pass.push_back (new renderingPass (
				new frameGPUi (info->width, info->height, cfg->internalType, cfg->debanding),
				renderToInternalSP, 0, 0));
			LOGD ("Planar to internal");

			break;
		}

		case 1: {
			const char* renderRgbToInteralFP =
				#include "shaders/displayFrag.h"

			shader renderToInternalShader (renderVP, renderRgbToInteralFP);
			GLuint renderToInternalSP = renderToInternalShader.loadProgram ();
			if (!renderToInternalSP)
				return false;

			glUseProgram (renderToInternalSP);
			glUniform1i (glGetUniformLocation (renderToInternalSP, "video"), 0);

			pass.push_back (new renderingPass (
				new frameGPUi (info->width, info->height, cfg->internalType, false),
				renderToInternalSP, 0, 0));
			LOGD ("RGBA to internal");

			break;
		}
	}

	// convert 4:2:0 -> 4:2:2
	if (info->halfHeight && !cfg->hwChroma) {
		const char* render420to422FP =
			#include "shaders/up420to422.h"

		shader render420to422Shader (renderVP, render420to422FP, precision);
		GLuint render420to422SP = render420to422Shader.loadProgram ();
		if (!render420to422SP)
			return false;

		glUseProgram (render420to422SP);
		glUniform1i (glGetUniformLocation (render420to422SP, "YCbCr"), 0);
		glUniform4f (glGetUniformLocation (render420to422SP, "pitch"),
			(float) ( 2.0 / info->height), (float) (1.0 / info->height),
			(float) (-0.5 / info->height), (float) (0.5 * info->height));

		pass.push_back (new renderingPass (
			new frameGPUi (info->chromaWidth, info->height, cfg->internalType, false),
			render420to422SP, 1, 0));
		LOGD ("Upsample chroma height");
	}

	// convert 4:2:2 -> 4:4:4
	if (info->halfWidth && !cfg->hwChroma) {
		const char* render422to444FP =
			#include "shaders/up422to444.h"

		shader render422to444Shader (renderVP, render422to444FP, precision, info->halfHeight ? "#define SEPARATE" : "");
		GLuint render422to444SP = render422to444Shader.loadProgram ();
		if (!render422to444SP)
			return false;

		glUseProgram (render422to444SP);
		glUniform1i (glGetUniformLocation (render422to444SP, "YCbCr"), 0);
		if (info->halfHeight)
			glUniform1i (glGetUniformLocation (render422to444SP, "CbCr"), 1);
		glUniform1f (glGetUniformLocation (render422to444SP, "pitch"), (float) (1.0 / info->width));

		pass.push_back (new renderingPass (
			new frameGPUi (info->width, info->height, cfg->internalType, false),
			render422to444SP, 0, 0));
		LOGD ("Upsample chroma width");
	}

	// debanding
	if (cfg->debanding) {
		const char* renderDownscaleFP =
			#include "shaders/displayFrag.h"

		shader renderDownscaleShader (renderVP, renderDownscaleFP);
		GLuint renderDownscaleSP = renderDownscaleShader.loadProgram ();
		if (!renderDownscaleSP)
			return false;

		glUseProgram (renderDownscaleSP);
		glUniform1i (glGetUniformLocation (renderDownscaleSP, "video"), 0);

		pass.push_back (new renderingPass (
			new frameGPUi (info->width / 2, info->height / 2, cfg->internalType, true),
			renderDownscaleSP, 1, 0));
		LOGD ("Debanding: downscale");

		const GLfloat blurWeight[] ={
		   0.1499282118,
		   0.1703959693,
		   0.1161194010,
		   0.0635564179
		};
		const GLfloat blurOffsetX[] ={
		  (float)  0.6604655169 / info->width * 2,
		  (float)  2.4653334866 / info->width * 2,
		  (float)  4.4378234991 / info->width * 2,
		  (float)  6.4106906239 / info->width * 2
		};
		const GLfloat blurOffsetY[] ={
		  (float)  0.6604655169 / info->height * 2,
		  (float)  2.4653334866 / info->height * 2,
		  (float)  4.4378234991 / info->height * 2,
		  (float)  6.4106906239 / info->height * 2
		};
		const int blurTaps = sizeof (blurWeight) / sizeof (*blurWeight);

		const char* renderBlurXFP =
			#include "shaders/blurX.h"

		shader renderBlurXShader (renderVP, renderBlurXFP, precision, blurTaps);
		GLuint renderBlurXSP = renderBlurXShader.loadProgram ();
		if (!renderBlurXSP)
			return false;

		glUseProgram (renderBlurXSP);
		glUniform1i  (glGetUniformLocation (renderBlurXSP, "video"), 1);
		glUniform1fv (glGetUniformLocation (renderBlurXSP, "weight"), blurTaps, blurWeight);
		glUniform1fv (glGetUniformLocation (renderBlurXSP, "offset"), blurTaps, blurOffsetX);

		pass.push_back (new renderingPass (
			new frameGPUi (info->width / 2, info->height / 2, cfg->internalType, true),
			renderBlurXSP, 1, 0));
		LOGD ("Debanding: blur width");

		const char* renderBlurYFP =
			#include "shaders/blurY.h"

		shader renderBlurYShader (renderVP, renderBlurYFP, precision, blurTaps);
		GLuint renderBlurYSP = renderBlurYShader.loadProgram ();
		if (!renderBlurYSP)
			return false;

		glUseProgram (renderBlurYSP);
		glUniform1i  (glGetUniformLocation (renderBlurYSP, "video"), 1);
		glUniform1fv (glGetUniformLocation (renderBlurYSP, "weight"), blurTaps, blurWeight);
		glUniform1fv (glGetUniformLocation (renderBlurYSP, "offset"), blurTaps, blurOffsetY);

		pass.push_back (new renderingPass (
			new frameGPUi (info->width / 2, info->height / 2, cfg->internalType, true),
			renderBlurYSP, 1, 0));
		LOGD ("Debanding: blur height");

		const char* renderDebandFP =
			#include "shaders/deband.h"

		shader renderDebandShader (renderVP, renderDebandFP, precision);
		GLuint renderDebandSP = renderDebandShader.loadProgram ();
		if (!renderDebandSP)
			return false;

		glUseProgram (renderDebandSP);
		glUniform1i (glGetUniformLocation (renderDebandSP, "video"), 0);
		glUniform1i (glGetUniformLocation (renderDebandSP, "blur"), 1);
		glUniform3f (glGetUniformLocation (renderDebandSP, "threshold"),
			(float) (cfg->debandThreshold / 255.0),
			(float) (cfg->debandThreshold / 255.0),
			(float) (cfg->debandThreshold / 255.0));

		pass.push_back (new renderingPass (
			new frameGPUi (info->width, info->height, cfg->internalType, false),
			renderDebandSP, 0, 0));
		LOGD ("Debanding: debanding");
	}

	// convert YCbCr -> RGB
	if (info->fourCC != pFormat::RGBA) {
		const char* renderYuvToRgbFP =
			#include "shaders/yuvToRgb.h"

		shader renderYuvToRgbShader (renderVP, renderYuvToRgbFP, precision);
		GLuint renderYuvToRgbSP = renderYuvToRgbShader.loadProgram ();
		if (!renderYuvToRgbSP)
			return false;

		glUseProgram (renderYuvToRgbSP);
		glUniform1i			(glGetUniformLocation (renderYuvToRgbSP, "video"),		0);
		glUniformMatrix3fv	(glGetUniformLocation (renderYuvToRgbSP, "conversion"), 1, GL_FALSE, info->colorConversion);
		glUniform3fv		(glGetUniformLocation (renderYuvToRgbSP, "offset"),		1, info->colorOffset);

		pass.push_back (new renderingPass (
			new frameGPUi (info->width, info->height, cfg->internalType,
				((info->width != cfg->targetWidth) || (info->height != cfg->targetHeight)) && cfg->hwScaleLinear),
			renderYuvToRgbSP, 0, 0));
		LOGD ("YCbCr -> RGB conversion");
	}

	// dither
	const char* renderDitherFP =
		#include "shaders/dither.h"

	shader renderDitherShader (renderVP, renderDitherFP, precision);
	GLuint renderDitherSP = renderDitherShader.loadProgram ();
	if (!renderDitherSP)
		return false;

	glUseProgram (renderDitherSP);
	glUniform1i (glGetUniformLocation (renderDitherSP, "video"), 0);
	glUniform1i (glGetUniformLocation (renderDitherSP, "dither"), 3);

	glUniform2f (glGetUniformLocation (renderDitherSP, "depth"),
		(float) (pow (2.0, cfg->targetBitdepth) - 1.0),
		(float) (1.0 / (pow (2.0, cfg->targetBitdepth) - 1.0)));
	glUniform2f (glGetUniformLocation (renderDitherSP, "resize"),
		(float) ((double) cfg->targetWidth / 32.0),
		(float) ((double) cfg->targetHeight / 32.0));

	#include "threads/helpers/ditherMatrix.h"

	frameGPUi* dither = new frameGPUi (32, 32, iFormat::DITHER, false);
	glActiveTexture (GL_TEXTURE0 + 3);
	glBindTexture (GL_TEXTURE_2D, dither->plane);
	glTexSubImage2D (GL_TEXTURE_2D, 0, 0, 0, 32, 32, GL_RGBA, GL_UNSIGNED_INT_2_10_10_10_REV, (GLvoid*) ditherMatrix);
	glActiveTexture (GL_TEXTURE0);

	pass.push_back (new renderingPass (
		dither,	renderDitherSP, 0, glGetUniformLocation (renderDitherSP, "offset")));
	LOGD ("Dither");

	return true;
}
