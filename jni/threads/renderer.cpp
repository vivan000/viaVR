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

#include "threads.h"

renderer::renderer (videoInfo* info, queue<frameGPUu>* uploadQueue, queue<frameGPUo>* renderQueue,
		EGLDisplay display, EGLSurface renderPbuffer, EGLContext renderContext, GLuint* vboIds) {
	renderer::info = info;
	renderer::uploadQueue = uploadQueue;
	renderer::renderQueue = renderQueue;

	renderer::display = display;
	renderer::pbuffer = renderPbuffer;
	renderer::context = renderContext;
	renderer::vboIds = vboIds;

	eglMakeCurrent (display, pbuffer, pbuffer, context);
	from = new frameGPUu (info);
	to = new frameGPUo (info);

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

	joined = false;
	working = true;
	while (working) {
		if (!uploadQueue->isEmpty () && !renderQueue->isFull ()) {
			uploadQueue->pop (from);

			to->timecode = from->timecode;

			for (int i = 0; i < info->planes; i++) {
				glActiveTexture (GL_TEXTURE0 + i);
				glBindTexture (GL_TEXTURE_2D, from->plane[i]);
			}

			for (rPass::iterator passIt = pass.begin (); passIt != pass.end (); ++passIt)
				if (!(*passIt)->dither)
					(*passIt)->execute ();
				else
					(*passIt)->execute (to->plane, info->targetWidth, info->targetHeight);

			glFlush ();

			renderQueue->push (to);
		} else {
			usleep (10000);
		}
	}

	eglMakeCurrent (display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
}

bool renderer::renderInit () {
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
				"highp", info->halfWidth && info->hwChroma ? "#define HWCHROMA" : "", info->bitdepth);
			GLuint renderToInternalSP = renderToInternalShader.loadProgram ();
			if (!renderToInternalSP)
				return false;

			glUseProgram (renderToInternalSP);
			glUniform1i (glGetUniformLocation (renderToInternalSP, "Y"),  0);
			glUniform1i (glGetUniformLocation (renderToInternalSP, "Cb"), 1);
			glUniform1i (glGetUniformLocation (renderToInternalSP, "Cr"), 2);
			if (info->halfWidth && info->hwChroma)
				glUniform1f (glGetUniformLocation (renderToInternalSP, "pitch"), (float) (0.5 / info->width));

			pass.push_back (new renderingPass (
				new frameGPUi (info->width, info->height, internalType, info),
				renderToInternalSP, 0, 0));
			LOGD ("Planar to internal");

			break;
		}

		case 1: {
			const char* renderRgbToInteralFP =
				#include "shaders/displayFrag.h"

			shader renderToInternalShader (renderVP, renderRgbToInteralFP, "highp");
			GLuint renderToInternalSP = renderToInternalShader.loadProgram ();
			if (!renderToInternalSP)
				return false;

			glUseProgram (renderToInternalSP);
			glUniform1i (glGetUniformLocation (renderToInternalSP, "video"), 0);

			pass.push_back (new renderingPass (
				new frameGPUi (info->width, info->height, internalType, info),
				renderToInternalSP, 0, 0));
			LOGD ("RGBA to internal");

			break;
		}
	}

	// convert 4:2:0 -> 4:2:2
	if (info->halfHeight && !info->hwChroma) {
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
			new frameGPUi (info->chromaWidth, info->height, internalType, info),
			render420to422SP, 1, 0));
		LOGD ("Upsample chroma height");
	}

	// convert 4:2:2 -> 4:4:4
	if (info->halfWidth && !info->hwChroma) {
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
			new frameGPUi (info->width, info->height, internalType, info),
			render422to444SP, 0, 0));
		LOGD ("Upsample chroma width");
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
		if (!renderBlurXSP)
			return false;

		glUseProgram (renderBlurXSP);
		glUniform1i  (glGetUniformLocation (renderBlurXSP, "video"), 0);
		glUniform1fv (glGetUniformLocation (renderBlurXSP, "weight"), blurTaps, blurWeight);
		glUniform1fv (glGetUniformLocation (renderBlurXSP, "offset"), blurTaps, blurOffsetX);

		pass.push_back (new renderingPass (
			new frameGPUi (info->width, info->height, internalType, info),
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
			new frameGPUi (info->width, info->height, internalType, info),
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
			(float) (debandThreshold / 255.0), (float) (debandThreshold / 255.0), (float) (debandThreshold / 255.0));

		pass.push_back (new renderingPass (
			new frameGPUi (info->width, info->height, internalType, info),
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
			new frameGPUi (info->width, info->height, internalType, info),
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
		(float) (pow (2.0, bitdepth) - 1.0),
		(float) (1.0 / (pow (2.0, bitdepth) - 1.0)));
	glUniform2f (glGetUniformLocation (renderDitherSP, "resize"),
		(float) ((double) info->targetWidth / 32.0),
		(float) ((double) info->targetHeight / 32.0));

	#include "ditherMatrix.h"

	frameGPUi* dither = new frameGPUi (32, 32, pFormat::DITHER, info);
	glActiveTexture (GL_TEXTURE0 + 3);
	glBindTexture (GL_TEXTURE_2D, dither->plane);
	glTexSubImage2D (GL_TEXTURE_2D, 0, 0, 0, 32, 32, GL_RGBA, GL_UNSIGNED_INT_2_10_10_10_REV, (GLvoid*) ditherMatrix);
	glActiveTexture (GL_TEXTURE0);

	pass.push_back (new renderingPass (
		dither,	renderDitherSP, 0, glGetUniformLocation (renderDitherSP, "offset")));
	LOGD ("Dither");

	return true;
}
