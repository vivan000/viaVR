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

#pragma once
#include <GLES3/gl3.h>

class shader {
public:
	shader (const char* vertexShaderSource, const char* fragmentShaderSource, ...);
	~shader ();

	GLuint loadProgram ();
private:
	GLuint loadShader (GLenum type, const char* shaderSource);

	GLuint vertexShader, fragmentShader;
	GLuint programObject;
};
