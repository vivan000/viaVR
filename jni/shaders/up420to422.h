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

uniform sampler2D video;
uniform vec2 chromaSize;
uniform vec2 chromaPitch;
in vec2 coord;
out vec4 outColor;

void main () {
	vec4 weight = fract (coord.y * chromaSize.y) > 0.5 ?
		vec4 (-0.0703125, 0.8671875, 0.2265625, -0.0234375) :
		vec4 (-0.0234375, 0.2265625, 0.8671875, -0.0703125);

	vec3 result = vec3 (texture (video, coord).r, 0.0, 0.0);
	result.gb += texture (video, vec2 (coord.x, coord.y - 1.5 * chromaPitch.y)).gb * weight.r;
	result.gb += texture (video, vec2 (coord.x, coord.y - 0.5 * chromaPitch.y)).gb * weight.g;
	result.gb += texture (video, vec2 (coord.x, coord.y + 0.5 * chromaPitch.y)).gb * weight.b;
	result.gb += texture (video, vec2 (coord.x, coord.y + 1.5 * chromaPitch.y)).gb * weight.a;

	outColor = vec4 (result, 1.0);
})";
