"attribute vec4 vertexCoord;											\n"
"attribute vec2 textureCoord;								 		    \n"
"																		\n"
"varying   vec2 coord;													\n"
"																		\n"
"void main () {															\n"
"    gl_Position = vertexCoord;											\n"
"    coord = textureCoord;												\n"
"}																		\n";
