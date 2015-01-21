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

uniform sampler2D Y;
uniform sampler2D Cb;
uniform sampler2D Cr;
#ifdef HWCHROMA
uniform float pitch;
#endif
in vec2 coord;
out vec4 outColor;

void main () {
#ifdef HWCHROMA
	vec2 newCoord = coord + vec2 (pitch, 0.0);
	outColor = vec4 (
		mix (texture (Y, coord).r, texture (Y, coord).g, 256.0 / 257.0),
		mix (texture (Cb, newCoord).r, texture (Cb, newCoord).g, 256.0 / 257.0),
		mix (texture (Cr, newCoord).r, texture (Cr, newCoord).g, 256.0 / 257.0),
		1.0);
#else
	outColor = vec4 (
		mix (texture (Y, coord).r, texture (Y, coord).g, 256.0 / 257.0),
		mix (texture (Cb, coord).r, texture (Cb, coord).g, 256.0 / 257.0),
		mix (texture (Cr, coord).r, texture (Cr, coord).g, 256.0 / 257.0),
		1.0);
#endif
})";
