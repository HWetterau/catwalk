R"zzz(#version 330 core

uniform mat4 ortho;
uniform float time;
in vec4 vertex_position;
void main() {
    gl_Position = ortho * (vertex_position + vec4(time * 0.0452,0,0, 0));
})zzz"
