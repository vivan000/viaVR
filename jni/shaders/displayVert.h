"#version 300 es														\n"
"layout(location = 0) in vec4 vertexCoord;								\n"
"layout(location = 1) in vec2 textureCoord;					 		    \n"
"out		vec2		coord;											\n"
"																		\n"
"void main () {															\n"
"    gl_Position = vertexCoord;											\n"
"    coord = textureCoord;												\n"
"}																		\n";
