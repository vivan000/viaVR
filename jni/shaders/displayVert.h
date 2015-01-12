"#version 300 es														\n"
"in			vec4		vertexCoord;									\n"
"in			vec2		textureCoord;						 		    \n"
"out		vec2		coord;											\n"
"																		\n"
"void main () {															\n"
"    gl_Position = vertexCoord;											\n"
"    coord = textureCoord;												\n"
"}																		\n";
