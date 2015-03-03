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

#include "threads/helpers/renderingPass.h"

renderingPass::renderingPass (frameGPUi* frame, GLuint program, int target, passType type, GLint uniform) :
	frame (frame),
	program (program),
	target (target),
	type (type),
	uniform (uniform) {}

renderingPass::~renderingPass () {
	if (renderingPass::type != passType::Dither)
		delete frame;
	glDeleteProgram (program);
}

// default pass
void renderingPass::executeDefault () {
	glViewport (0, 0, frame->width, frame->height);
	glFramebufferTexture2D (GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, frame->plane, 0);

	glUseProgram (program);
	glDrawArrays (GL_TRIANGLE_STRIP, 0, 4);

	glActiveTexture (GL_TEXTURE0 + target);
	glBindTexture (GL_TEXTURE_2D, frame->plane);
}

// dither pass
void renderingPass::executeDither (GLuint plane, int targetWidth, int targetHeight) {
	glViewport (0, 0, targetWidth, targetHeight);
	glFramebufferTexture2D (GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, plane, 0);

	glUseProgram (program);
	glUniform3f (uniform,
		(float) ((rand() % 32) / 32.0),
		(float) ((rand() % 32) / 32.0),
		(float) (rand() % 3));
	glDrawArrays (GL_TRIANGLE_STRIP, 0, 4);
}
