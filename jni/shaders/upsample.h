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
#define HEIGHT %i
#define SEPARATE %i

uniform sampler2D video;
#if SEPARATE
uniform sampler2D chroma;
#else
#define chroma video
#endif
uniform vec2 chromaSize;
uniform vec2 chromaPitch;
in vec2 coord;
out vec4 outColor;

#if HEIGHT
#define newcoord(c) (vec2 (coord.x, coord.y + c * chromaPitch.y))
#else
#define newcoord(c) (vec2 (coord.x + c * chromaPitch.x, coord.y))
#endif

void main () {
	vec3 result = vec3 (0.0);

#if HEIGHT
	vec2 pix1 = texture (video, newcoord (-1.5)).gb;
	vec2 pix2 = texture (video, newcoord (-0.5)).gb;
	vec2 pix3 = texture (video, newcoord ( 0.5)).gb;
	vec2 pix4 = texture (video, newcoord ( 1.5)).gb;
	
	vec4 weight = fract (coord.y * chromaSize.y) > 0.5 ?
		vec4 (-0.0703125, 0.8671875, 0.2265625, -0.0234375) :
		vec4 (-0.0234375, 0.2265625, 0.8671875, -0.0703125);
#else
	vec2 pix1 = texture (chroma, newcoord (-1.0)).gb;
	vec2 pix2 = texture (chroma, coord).gb;
	vec2 pix3 = texture (chroma, newcoord ( 1.0)).gb;
	vec2 pix4 = texture (chroma, newcoord ( 2.0)).gb;

	vec4 weight = fract (coord.x * chromaSize.x) > 0.5 ?
		vec4 (-0.0625, 0.5625, 0.5625, -0.0625) :
		vec4 ( 0.0000, 1.0000, 0.0000,  0.0000);

	result.r = texture (video, coord).r;
#endif

	result.gb += pix1 * weight.r;
	result.gb += pix2 * weight.g;
	result.gb += pix3 * weight.b;
	result.gb += pix4 * weight.a;

	outColor = vec4 (result, 1.0);
})";
