"precision %s float;															\n"
"																				\n"
"uniform   sampler2D Y;															\n"
"uniform   sampler2D Cb;														\n"
"uniform   sampler2D Cr;														\n"
"varying   vec2      coord;														\n"
"																				\n"
"void main () {																	\n"
"    gl_FragColor = vec4 (														\n"
"		mix (texture2D (Y, coord).r, texture2D (Y, coord).a, 256.0 / 257.0),	\n"
"		mix (texture2D (Cb, coord).r, texture2D (Cb, coord).a, 256.0 / 257.0),	\n"
"		mix (texture2D (Cr, coord).r, texture2D (Cr, coord).a, 256.0 / 257.0),	\n"
"		1.0);																	\n"
"}																				\n";