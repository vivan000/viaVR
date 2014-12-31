"precision lowp float;												\n"
"																	\n"
"uniform   sampler2D displayT;										\n"
"varying   vec2      displayTCI;									\n"
"																	\n"
"void main () {														\n"
"    gl_FragColor = texture2D (displayT, displayTCI);				\n"
"}																	\n";
