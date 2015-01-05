"precision %s float;													\n"
"																		\n"
"uniform   sampler2D Y;													\n"
"uniform   sampler2D Cb;												\n"
"uniform   sampler2D Cr;												\n"
"varying   vec2      coord;												\n"
"																		\n"
"void main () {															\n"
"    gl_FragColor = vec3 (texture2D (Y, coord).r,						\n"
"		texture2D (Cb, coord).r, texture2D (Cr, coord).r);				\n"
"}																		\n";
