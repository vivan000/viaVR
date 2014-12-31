"precision lowp float;												\n"
"																	\n"
"uniform   sampler2D texture;										\n"
"varying   vec2      coord;											\n"
"																	\n"
"void main () {														\n"
"    gl_FragColor = texture2D (texture, coord);						\n"
"}																	\n";
