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

#include <cstdio>
#include <string.h>
#include "log.h"
#include "threads/helpers/shaderLoader.h"

shaderLoader::shaderLoader () {
}

shaderLoader::~shaderLoader () {
}

GLuint shaderLoader::loadShaders (const char* vertexShaderSource, const char* fragmentShaderSource, ...) {
	int size = strlen (fragmentShaderSource) + 32;
	char* fragmentShaderSourceProcessed = new char[size];

	va_list argptr;
	va_start (argptr, fragmentShaderSource);
	vsprintf (fragmentShaderSourceProcessed, fragmentShaderSource, argptr);
	va_end (argptr);

	GLuint vertexShader = loadShader (GL_VERTEX_SHADER, vertexShaderSource);
	GLuint fragmentShader = loadShader (GL_FRAGMENT_SHADER, fragmentShaderSourceProcessed);
	delete[] fragmentShaderSourceProcessed;

	if (!vertexShader || !fragmentShader)
		return 0;

	GLuint programObject = glCreateProgram ();
	if (!programObject) {
		LOGE ("Error creating program");
		return 0;
	}

	glAttachShader (programObject, vertexShader);
	glAttachShader (programObject, fragmentShader);
	glLinkProgram (programObject);

	GLint linked;
	glGetProgramiv (programObject, GL_LINK_STATUS, &linked);
	if (!linked) {
		GLint infoLen = 0;
		glGetProgramiv (programObject, GL_INFO_LOG_LENGTH, &infoLen);
		if (infoLen > 1) {
			char* infoLog = new char[infoLen];
			glGetProgramInfoLog (programObject, infoLen, NULL, infoLog);
			LOGE ("Error linking program:\n%s", infoLog);
			delete[] infoLog;
		}
		return 0;
	}

	glDeleteShader (vertexShader);
	glDeleteShader (fragmentShader);
	return programObject;
}

GLuint shaderLoader::loadShader (GLenum type, const char* shaderSource) {
	GLuint shader = glCreateShader (type);
	if (!shader) {
		LOGE ("Error creating shader");
		return 0;
	}

	glShaderSource (shader, 1, &shaderSource, NULL);
	glCompileShader (shader);

	GLint compiled;
	glGetShaderiv (shader, GL_COMPILE_STATUS, &compiled);
	if (!compiled) {
		GLint infoLen = 0;
		glGetShaderiv (shader, GL_INFO_LOG_LENGTH, &infoLen);
		if (infoLen > 1) {
			char* infoLog = new char[infoLen];
			glGetShaderInfoLog (shader, infoLen, NULL, infoLog);
			LOGE ("Error compiling shader:\n%s", infoLog);
			delete[] infoLog;
		}
		glDeleteShader (shader);
		return 0;
	}

	return shader;
}
