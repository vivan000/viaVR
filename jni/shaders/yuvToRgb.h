"precision %s float;																	\n"
"																						\n"
"uniform   sampler2D texture;															\n"
"uniform   mat3      conversion;														\n"
"uniform   vec3      offset;															\n"
"varying   vec2      coord;																\n"
"																						\n"
"void main () {																			\n"
"    gl_FragColor = vec4 (texture2D (texture, coord).rgb * conversion + offset, 1.0);	\n"
"}																						\n";
