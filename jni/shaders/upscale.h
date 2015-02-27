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

uniform sampler2D video;
uniform sampler2D weights;
uniform float pitch;
in vec2 coord;
out vec4 outColor;

#ifdef WIDTH
#define newcoord(c) (vec2 (coord.x + c * pitch, coord.y))
#else
#define newcoord(c) (vec2 (coord.x, coord.y + c * pitch))
#endif

void main () {
#ifdef WIDTH
	vec4 weightsL = texture (weights, vec2 (coord.x, 0.25));
	vec4 weightsR = texture (weights, vec2 (coord.x, 0.75));
#else
	vec4 weightsL = texture (weights, vec2 (coord.y, 0.25));
	vec4 weightsR = texture (weights, vec2 (coord.y, 0.75));
#endif

	vec3 result = vec3 (0.0);
	result += texture (video, newcoord (-2.5)).rgb * weightsL.b;
	result += texture (video, newcoord (-1.5)).rgb * weightsL.g;
	result += texture (video, newcoord (-0.5)).rgb * weightsL.r;
	result += texture (video, newcoord ( 0.5)).rgb * weightsR.r;
	result += texture (video, newcoord ( 1.5)).rgb * weightsR.g;
	result += texture (video, newcoord ( 2.5)).rgb * weightsR.b;

	outColor = vec4 (result, 1.0);
})";
