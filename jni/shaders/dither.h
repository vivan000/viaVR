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
uniform sampler2D dither;
uniform vec2 depth;
uniform vec2 resize;
uniform vec3 offset;
in vec2 coord;
out vec4 outColor;

void main () {
	vec3 pattern = texture (dither, coord * resize + offset.xy).rgb - 0.5;
	pattern = (offset.z == 0.0) ? pattern.rrr : ((offset.z == 1.0) ? pattern.ggg : pattern.bbb);
	pattern.g = -pattern.g;
	vec3 preQ = texture (video, coord).rgb * depth.x;
	vec3 postQ = round (preQ + pattern);
	outColor = vec4 (postQ * depth.y, 1.0);
})";
