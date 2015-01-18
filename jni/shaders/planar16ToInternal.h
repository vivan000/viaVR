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
"%s																		\n"
"																		\n"
"uniform	sampler2D	Y;												\n"
"uniform	sampler2D	Cb;												\n"
"uniform	sampler2D	Cr;												\n"
"#ifdef HWCHROMA														\n"
"uniform	float		pitch;											\n"
"#endif																	\n"
"in			vec2		coord;											\n"
"out		vec4		outColor;										\n"
"																				\n"
"void main () {																	\n"
"#ifdef HWCHROMA																\n"
"	vec2 newCoord = coord + vec2 (pitch, 0.0);									\n"
"	outColor = vec4 (															\n"
"		mix (texture (Y, coord).r, texture (Y, coord).g, 256.0 / 257.0),		\n"
"		mix (texture (Cb, newCoord).r, texture (Cb, newCoord).g, 256.0 / 257.0),\n"
"		mix (texture (Cr, newCoord).r, texture (Cr, newCoord).g, 256.0 / 257.0),\n"
"		1.0);																	\n"
"#else																			\n"
"	outColor = vec4 (															\n"
"		mix (texture (Y, coord).r, texture (Y, coord).g, 256.0 / 257.0),		\n"
"		mix (texture (Cb, coord).r, texture (Cb, coord).g, 256.0 / 257.0),		\n"
"		mix (texture (Cr, coord).r, texture (Cr, coord).g, 256.0 / 257.0),		\n"
"		1.0);																	\n"
"#endif																			\n"
"}																				\n";
