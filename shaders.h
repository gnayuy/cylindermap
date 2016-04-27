//
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <limits>
using namespace std;

//
const char* vertex_shader =
"#version 410 \n"
"in vec3 vp;"
"void main () {"
"	gl_Position = vec4 (vp, 1.0);"
"}";

const char* fragment_shader =
"#version 410 \n"
"uniform vec3 color;"
"out vec4 fragColor;"
"void main () {"
"	fragColor = vec4 (color, 1.0);"
"}";

//
const char* post_vertex_shader =
"#version 410 \n"
"in vec2 vp;"
"out vec2 st;"
"void main () {"
"    st = (vp + 1.0) * 0.5;"
"    gl_Position = vec4 (vp, 0.0, 1.0);"
"}";

const char* post_fragment_shader =
"#version 410 \n"
"in vec2 st;"
"uniform sampler2D tex;"
"out vec4 fragColor;"
"void main () {"
"    fragColor = texture (tex, st);"
"}";

// warp
const char* vsWarp =
"#version 410 \n"
"in vec2 vPos;"
"out vec2 st;"
"void main () {"
"    st = (vPos + 1.0) * 0.5;"
"    gl_Position = vec4(vPos, 0.0, 1.0);"
"}";

const char* fsWarp =
"#version 410 \n"
"in vec2 st;"
"uniform float w;"
"uniform float h;"
"uniform sampler2D tex0;"
"uniform sampler2D tex1;"
"out vec4 fragColor;"
"void main () {"
"    vec4 texc = texture(tex1, st);"
"    fragColor = texture(tex0, vec2(texc.s/w, texc.t/h));"
"}";
