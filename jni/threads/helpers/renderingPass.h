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
#include "frames/frames.h"

enum class passType {
	Default,
	Last,
};

class renderingPass {
public:
	renderingPass (frameGPUi* frame, GLuint program, int target = 0, passType type = passType::Default);
	~renderingPass ();
	void executeDefault ();
	void executeLast (frameGPUo* to);

	const passType type;

private:
	const frameGPUi* frame;
	const GLuint program;
	const int target;
};
