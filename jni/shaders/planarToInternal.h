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

R"(#version 300 es
precision %s float;
#define SHIFTCHROMA %i
#define DEPTH %i
#define TORGB %i

uniform sampler2D Y;
uniform sampler2D Cb;
uniform sampler2D Cr;
#if SHIFTCHROMA
uniform float chromaShift;
#else
#define chromaCoord coord
#endif
#if TORGB
uniform mat3 colorMatrix;
uniform vec3 colorOffset;
#endif
in vec2 coord;
out vec4 outColor;

void main () {
#if SHIFTCHROMA
	vec2 chromaCoord = vec2 (coord.x + chromaShift, coord.y);
#endif
	vec3 result = vec3 (
#if DEPTH == 8
		texture (Y,  coord      ).r,
		texture (Cb, chromaCoord).r,
		texture (Cr, chromaCoord).r);
#elif DEPTH == 10
		texture (Y,  coord      ).g * 64.0 + texture (Y,  coord      ).r * 0.25,
		texture (Cb, chromaCoord).g * 64.0 + texture (Cb, chromaCoord).r * 0.25,
		texture (Cr, chromaCoord).g * 64.0 + texture (Cr, chromaCoord).r * 0.25);
#elif DEPTH == 16
		texture (Y,  coord      ).g + texture (Y,  coord      ).r / 256.0,
		texture (Cb, chromaCoord).g + texture (Cb, chromaCoord).r / 256.0,
		texture (Cr, chromaCoord).g + texture (Cr, chromaCoord).r / 256.0);
#endif

#if TORGB
	result = result * colorMatrix + colorOffset;
#endif

	outColor = vec4 (result, 1.0);
})";
