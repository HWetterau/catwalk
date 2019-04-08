R"zzz(#version 330 core
uniform mat4 ortho;
uniform vec2 offset;
in vec4 vertex_position;

void main() {
    gl_Position = ortho * (vertex_position + vec4(offset,0,0));
   
})zzz"
