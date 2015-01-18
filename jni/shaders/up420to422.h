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

"#version 300 es														\n"
"precision %s float;													\n"
"																		\n"
"uniform	sampler2D	YCbCr;											\n"
"uniform	vec4		pitch;											\n"
"in			vec2		coord;											\n"
"out		vec4		outColor;										\n"
"																		\n"
"void main () {															\n"
"	float c = round (coord.y * pitch.a) * pitch.r + pitch.b;			\n"
"	outColor = vec4 (													\n"
"		0.0,															\n"
"		mix (															\n"
"			texture (YCbCr, vec2 (coord.x, c + pitch.g)).gb,			\n"
"			texture (YCbCr, vec2 (coord.x, c          )).gb,			\n"
"			fract (coord.y * pitch.a)),									\n"
"		0.0);															\n"
"}																		\n";
