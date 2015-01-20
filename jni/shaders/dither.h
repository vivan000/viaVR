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
"uniform	sampler2D	video;											\n"
"uniform	sampler2D	dither;											\n"
"uniform	vec2		depth;											\n"
"uniform	vec2		resize;											\n"
"uniform	vec2		offset;											\n"
"in			vec2		coord;											\n"
"out		vec4		outColor;										\n"
"																		\n"
"void main () {															\n"
"	vec3 pattern = texture (dither, coord * resize + offset).rrr;		\n"
"	vec3 patternShift = pattern * 1023.0 / 1024.0;						\n"
"	patternShift.g = 1023.0 / 1024.0 - patternShift.g;					\n"
"	vec3 preQ = texture (video, coord).rgb * depth.x;					\n"
"	vec3 postQ = floor (preQ + patternShift);							\n"
"	outColor = vec4 (postQ * depth.y, 1.0);								\n"
"}																		\n";
