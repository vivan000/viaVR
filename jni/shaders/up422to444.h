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
#ifdef SEPARATE
uniform sampler2D chroma;
#else
#define chroma video
#endif
uniform vec2 chromaSize;
uniform vec2 chromaPitch;
in vec2 coord;
out vec4 outColor;

void main () {
	vec4 weight = fract (coord.x * chromaSize.x) > 0.5 ?
		vec4 (-0.0625, 0.5625, 0.5625, -0.0625) :
		vec4 ( 0.0000, 1.0000, 0.0000,  0.0000);
	
	vec3 result = vec3 (texture (video, coord).r, 0.0, 0.0);
	result.gb += texture (chroma, vec2 (coord.x -       chromaPitch.x, coord.y)).gb * weight.r;
	result.gb += texture (chroma, coord                                        ).gb * weight.g;
	result.gb += texture (chroma, vec2 (coord.x +       chromaPitch.x, coord.y)).gb * weight.b;
	result.gb += texture (chroma, vec2 (coord.x + 2.0 * chromaPitch.x, coord.y)).gb * weight.a;

	outColor = vec4 (result, 1.0);
})";
