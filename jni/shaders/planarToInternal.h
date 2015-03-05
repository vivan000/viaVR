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
%s
#define DEPTH%i

uniform sampler2D Y;
uniform sampler2D Cb;
uniform sampler2D Cr;
#ifdef HWCHROMA
uniform float chromaShift;
#else
#define chroma coord
#endif
in vec2 coord;
out vec4 outColor;

void main () {
#ifdef HWCHROMA
	vec2 chroma = vec2 (coord.x + chromaShift, coord.y);
#endif
	vec3 result = vec3 (
#ifdef DEPTH8
		texture (Y,  coord ).r,
		texture (Cb, chroma).r,
		texture (Cr, chroma).r);
#endif
#ifdef DEPTH10
		texture (Y,  coord ).g * 64.0 + texture (Y,  coord ).r * 0.25,
		texture (Cb, chroma).g * 64.0 + texture (Cb, chroma).r * 0.25,
		texture (Cr, chroma).g * 64.0 + texture (Cr, chroma).r * 0.25);
#endif
#ifdef DEPTH16
		texture (Y,  coord ).g + texture (Y,  coord ).r / 256.0,
		texture (Cb, chroma).g + texture (Cb, chroma).r / 256.0,
		texture (Cr, chroma).g + texture (Cr, chroma).r / 256.0);
#endif

	outColor = vec4 (result, 1.0);
})";
