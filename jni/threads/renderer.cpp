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
#include "threads/helpers/scalers.h"

#define textureDither 3
#define textureScaleWeightsY 4
#define textureScaleWeightsX 5

int64_t nanotime () {
	struct timespec now;
	clock_gettime (CLOCK_MONOTONIC, &now);
	return (int64_t) now.tv_sec * 1000000000LL + now.tv_nsec;
}

renderer::renderer (videoInfo* info, config* cfg, queue<frameCPU>* decodeQueue, queue<frameGPUo>* renderQueue,
		EGLDisplay display, EGLSurface renderPbuffer, EGLContext renderContext, GLuint* vboIds) {
	renderer::info = info;
	renderer::cfg = cfg;
	renderer::decodeQueue = decodeQueue;
	renderer::renderQueue = renderQueue;

	renderer::display = display;
	renderer::pbuffer = renderPbuffer;
	renderer::context = renderContext;
	renderer::vboIds = vboIds;

	eglMakeCurrent (display, pbuffer, pbuffer, context);
	glClearColor (0.0f, 0.0f, 0.0f, 0.0f);
	glDisable (GL_DITHER);

	from = new frameCPU (info, cfg);
	up = new frameGPUu (info, cfg);
	to = new frameGPUo (info, cfg);

	glGenFramebuffers (1, &framebuffer);
	glBindFramebuffer (GL_FRAMEBUFFER, framebuffer);

	if (renderInit ())
		LOGD ("Rendering chain: OK");
	eglMakeCurrent (display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);

	start ();
}

renderer::~renderer () {
	if (!joined)
		stop ();

	eglMakeCurrent (display, pbuffer, pbuffer, context);
	glUseProgram (0);
	for (rPass::iterator it = pass.begin (); it != pass.end (); ++it)
		delete *it;
	for (std::vector<frameGPUi*>::iterator it = helperFrames.begin (); it != helperFrames.end (); ++it)
		delete *it;

	delete from;
	delete up;
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
#if BENCHMARK
	int64_t rstart = nanotime ();
	int rcount = 0;
	decodeQueue->pop (from);
#endif
	joined = false;
	working = true;
	while (working) {
		if (!decodeQueue->isEmpty () && !renderQueue->isFull ()) {
#if !BENCHMARK
			decodeQueue->pop (from);
#endif
			to->timecode = from->timecode;

			glActiveTexture (GL_TEXTURE0);
			glBindTexture (GL_TEXTURE_2D, up->plane[0]);
			glTexSubImage2D (GL_TEXTURE_2D, 0, 0, 0, info->width, info->height,
				info->lumaFormat, info->lumaType, (GLvoid*) from->plane);

			if (info->planes > 1) {
				glActiveTexture (GL_TEXTURE1);
				glBindTexture (GL_TEXTURE_2D, up->plane[1]);
				glTexSubImage2D (GL_TEXTURE_2D, 0, 0, 0, info->chromaWidth, info->chromaHeight,
					info->chromaFormat, info->chromaType, (GLvoid*) (from->plane + info->offset1));
			}

			if (info->planes > 2) {
				glActiveTexture (GL_TEXTURE2);
				glBindTexture (GL_TEXTURE_2D, up->plane[2]);
				glTexSubImage2D (GL_TEXTURE_2D, 0, 0, 0, info->chromaWidth, info->chromaHeight,
					info->chromaFormat, info->chromaType, (GLvoid*) (from->plane + info->offset2));
			}

			for (rPass::iterator passIt = pass.begin (); passIt != pass.end (); ++passIt)
				switch ((*passIt)->type) {
					case passType::Default:
						(*passIt)->executeDefault ();
						break;
					case passType::Dither:
						(*passIt)->executeDither (to->plane, cfg->targetWidth, cfg->targetHeight);
						break;
				}

			glFlush ();

#if BENCHMARK
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
	shaderLoader shader;
	LOGD ("Rendering chain:");

	bool needsHwScaling = ((info->width != cfg->targetWidth) || (info->height != cfg->targetHeight)) && cfg->hwScaleLinear;

	// convert to internal format
	switch (info->planes) {
		case 3: {
			bool shiftChroma = info->halfWidth && cfg->hwChroma;
			bool convertToRGB = cfg->hwChroma && !cfg->debanding;

			const char* renderToInternalFP =
				#include "shaders/planarToInternal.h"

			GLuint renderToInternalSP = shader.loadShaders (renderVP, renderToInternalFP, precision, shiftChroma, info->bitdepth, convertToRGB);
			if (!renderToInternalSP)
				return false;

			glUseProgram (renderToInternalSP);
			glUniform1i (glGetUniformLocation (renderToInternalSP, "Y"),  0);
			glUniform1i (glGetUniformLocation (renderToInternalSP, "Cb"), 1);
			glUniform1i (glGetUniformLocation (renderToInternalSP, "Cr"), 2);
			if (shiftChroma)
				glUniform1f (glGetUniformLocation (renderToInternalSP, "chromaShift"), (float) (0.5 / info->width));
			if (convertToRGB) {
				glUniformMatrix3fv (glGetUniformLocation (renderToInternalSP, "colorMatrix"),
					1, GL_FALSE, info->colorConversion);
				glUniform3fv (glGetUniformLocation (renderToInternalSP, "colorOffset"),
					1, info->colorOffset);
			}

			pass.push_back (new renderingPass (
				new frameGPUi (info->width, info->height, cfg->internalType, needsHwScaling && convertToRGB),
				renderToInternalSP));
			LOGD ("Planar to internal%s%s", shiftChroma ? " + shift chroma" : "", convertToRGB ? " + convert to RGB" : "");

			break;
		}

		case 1: {
			const char* renderRgbToInteralFP =
				#include "shaders/displayFrag.h"

			GLuint renderToInternalSP = shader.loadShaders (renderVP, renderRgbToInteralFP);
			if (!renderToInternalSP)
				return false;

			glUseProgram (renderToInternalSP);
			glUniform1i (glGetUniformLocation (renderToInternalSP, "video"), 0);

			pass.push_back (new renderingPass (
				new frameGPUi (info->width, info->height, cfg->internalType, needsHwScaling && !cfg->debanding),
				renderToInternalSP));
			LOGD ("RGBA to internal");

			break;
		}
	}

	// upsample chroma height (4:2:0 -> 4:2:2)
	if (info->halfHeight && !cfg->hwChroma) {
		const char* renderUpHeightFP =
			#include "shaders/upsample.h"

		GLuint renderUpHeightSP = shader.loadShaders (renderVP, renderUpHeightFP, precision, true, false, false);
		if (!renderUpHeightSP)
			return false;

		glUseProgram (renderUpHeightSP);
		glUniform1i (glGetUniformLocation (renderUpHeightSP, "video"), 0);
		glUniform2f (glGetUniformLocation (renderUpHeightSP, "chromaSize"),
			(float) info->chromaWidth, (float) info->chromaHeight);
		glUniform2f (glGetUniformLocation (renderUpHeightSP, "chromaPitch"),
			(float) 1.0 / info->chromaWidth, (float) 1.0 / info->chromaHeight);

		pass.push_back (new renderingPass (
			new frameGPUi (info->chromaWidth, info->height, cfg->internalType, false),
			renderUpHeightSP, 1));
		LOGD ("Upsample chroma height");
	}

	// upsample chroma width (4:2:2 -> 4:4:4)
	if (info->halfWidth && !cfg->hwChroma) {
		bool separateChroma = info->halfHeight;
		bool convertToRGB = !cfg->debanding;

		const char* renderUpWidthFP =
			#include "shaders/upsample.h"

		GLuint renderUpWidthSP = shader.loadShaders (renderVP, renderUpWidthFP, precision, false, separateChroma, convertToRGB);
		if (!renderUpWidthSP)
			return false;

		glUseProgram (renderUpWidthSP);
		glUniform1i (glGetUniformLocation (renderUpWidthSP, "video"), 0);
		if (separateChroma)
			glUniform1i (glGetUniformLocation (renderUpWidthSP, "chroma"), 1);
		glUniform2f (glGetUniformLocation (renderUpWidthSP, "chromaSize"),
			(float) info->chromaWidth, (float) info->chromaHeight);
		glUniform2f (glGetUniformLocation (renderUpWidthSP, "chromaPitch"),
			(float) 1.0 / info->chromaWidth, (float) 1.0 / info->chromaHeight);
		if (convertToRGB) {
			glUniformMatrix3fv (glGetUniformLocation (renderUpWidthSP, "colorMatrix"),
				1, GL_FALSE, info->colorConversion);
			glUniform3fv (glGetUniformLocation (renderUpWidthSP, "colorOffset"),
				1, info->colorOffset);
		}

		pass.push_back (new renderingPass (
			new frameGPUi (info->width, info->height, cfg->internalType, needsHwScaling && convertToRGB),
			renderUpWidthSP));
		LOGD ("Upsample chroma width%s", convertToRGB ? " + convert to RGB" : "");
	}

	// debanding
	if (cfg->debanding) {
		bool convertToRGB = info->fourCC != pFormat::RGBA;

		const char* renderDebandFP =
			#include "shaders/deband.h"

		GLuint renderDebandSP = shader.loadShaders (renderVP, renderDebandFP, precision, convertToRGB);
		if (!renderDebandSP)
			return false;

		glUseProgram (renderDebandSP);
		glUniform1i (glGetUniformLocation (renderDebandSP, "video"), 0);
		glUniform1i (glGetUniformLocation (renderDebandSP, "dither"), textureDither);
		glUniform2f (glGetUniformLocation (renderDebandSP, "pitch"),
			(float) (1.0 / info->width),
			(float) (1.0 / info->height));
		glUniform2f (glGetUniformLocation (renderDebandSP, "debandResize"),
			(float) (cfg->targetWidth / 32.0),
			(float) (cfg->targetHeight / 32.0));
		glUniform3f (glGetUniformLocation (renderDebandSP, "debandThresh"),
			(float) (-3.0 * 255.0 / cfg->debandAvgDif),
			(float) (-3.0 * 255.0 / cfg->debandMaxDif),
			(float) (-3.0 * 255.0 / cfg->debandMidDif));
		if (convertToRGB) {
			glUniformMatrix3fv (glGetUniformLocation (renderDebandSP, "colorMatrix"),
				1, GL_FALSE, info->colorConversion);
			glUniform3fv (glGetUniformLocation (renderDebandSP, "colorOffset"),
				1, info->colorOffset);
		}

		pass.push_back (new renderingPass (
			new frameGPUi (info->width, info->height, cfg->internalType, needsHwScaling),
			renderDebandSP));
		LOGD ("Debanding%s", convertToRGB ? " + convert to RGB" : "");
	}

	// upscale height
	if (cfg->targetHeight > info->height && !cfg->hwScale) {
		const char* renderUpHeightFP =
			#include "shaders/upscale.h"

		GLuint renderUpHeightSP = shader.loadShaders (renderVP, renderUpHeightFP, precision, cfg->scaleTaps, true, cfg->antiring);
		if (!renderUpHeightSP)
			return false;

		glUseProgram (renderUpHeightSP);
		glUniform1i (glGetUniformLocation (renderUpHeightSP, "video"), 0);
		glUniform1i (glGetUniformLocation (renderUpHeightSP, "weights"), textureScaleWeightsY);
		glUniform2f (glGetUniformLocation (renderUpHeightSP, "pitch"),
			(float) (1.0 / info->width), (float) (1.0 / info->height));

		scalers s (cfg->scaleKernel, cfg->scaleTaps, info->height, cfg->targetHeight);

		frameGPUi* weights = new frameGPUi (cfg->targetHeight, 2, iFormat::FLOAT16, false);
		helperFrames.push_back (weights);

		glActiveTexture (GL_TEXTURE0 + textureScaleWeightsY);
		glBindTexture (GL_TEXTURE_2D, weights->plane);
		glTexSubImage2D (GL_TEXTURE_2D, 0, 0, 0, cfg->targetHeight, 2, GL_RGBA, GL_FLOAT, (GLvoid*) s.getWeights ());
		glActiveTexture (GL_TEXTURE0);

		pass.push_back (new renderingPass (
			new frameGPUi (info->width, cfg->targetHeight, cfg->internalType, false),
			renderUpHeightSP));
		LOGD ("Upscale height%s", cfg->antiring ? " + antiring" : "");
	}

	// upscale width
	if (cfg->targetWidth > info->width && !cfg->hwScale) {
		const char* renderUpWidthFP =
			#include "shaders/upscale.h"

		GLuint renderUpWidthSP = shader.loadShaders (renderVP, renderUpWidthFP, precision, cfg->scaleTaps, false, cfg->antiring);
		if (!renderUpWidthSP)
			return false;

		glUseProgram (renderUpWidthSP);
		glUniform1i (glGetUniformLocation (renderUpWidthSP, "video"), 0);
		glUniform1i (glGetUniformLocation (renderUpWidthSP, "weights"), textureScaleWeightsX);
		glUniform2f (glGetUniformLocation (renderUpWidthSP, "pitch"),
			(float) (1.0 / info->width), (float) (1.0 / info->height));

		scalers s (cfg->scaleKernel, cfg->scaleTaps, info->width, cfg->targetWidth);

		frameGPUi* weights = new frameGPUi (cfg->targetWidth, 2, iFormat::FLOAT16, false);
		helperFrames.push_back (weights);

		glActiveTexture (GL_TEXTURE0 + textureScaleWeightsX);
		glBindTexture (GL_TEXTURE_2D, weights->plane);
		glTexSubImage2D (GL_TEXTURE_2D, 0, 0, 0, cfg->targetWidth, 2, GL_RGBA, GL_FLOAT, (GLvoid*) s.getWeights ());
		glActiveTexture (GL_TEXTURE0);

		pass.push_back (new renderingPass (
			new frameGPUi (cfg->targetWidth, cfg->targetHeight, cfg->internalType, false),
			renderUpWidthSP));
		LOGD ("Upscale width%s", cfg->antiring ? " + antiring" : "");
	}

	// dither
	const char* renderDitherFP =
		#include "shaders/dither.h"

	GLuint renderDitherSP = shader.loadShaders (renderVP, renderDitherFP, precision);
	if (!renderDitherSP)
		return false;

	glUseProgram (renderDitherSP);
	glUniform1i (glGetUniformLocation (renderDitherSP, "video"), 0);
	glUniform1i (glGetUniformLocation (renderDitherSP, "dither"), textureDither);
	glUniform2f (glGetUniformLocation (renderDitherSP, "ditherDepth"),
		(float) (pow (2.0, cfg->targetBitdepth) - 1.0),
		(float) (1.0 / (pow (2.0, cfg->targetBitdepth) - 1.0)));
	glUniform2f (glGetUniformLocation (renderDitherSP, "ditherResize"),
		(float) (cfg->targetWidth / 32.0),
		(float) (cfg->targetHeight / 32.0));

	#include "threads/helpers/ditherMatrix.h"

	frameGPUi* dither = new frameGPUi (32, 32, iFormat::DITHER, false);
	helperFrames.push_back (dither);

	glActiveTexture (GL_TEXTURE0 + textureDither);
	glBindTexture (GL_TEXTURE_2D, dither->plane);
	glTexSubImage2D (GL_TEXTURE_2D, 0, 0, 0, 32, 32, GL_RGBA, GL_UNSIGNED_INT_2_10_10_10_REV, (GLvoid*) ditherMatrix);
	glActiveTexture (GL_TEXTURE0);

	pass.push_back (new renderingPass (
		nullptr, renderDitherSP, 0, passType::Dither, glGetUniformLocation (renderDitherSP, "ditherOffset")));
	LOGD ("Dither");

	return true;
}
