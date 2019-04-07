R"zzz(#version 330 core

uniform mat4 ortho;
uniform vec2 offset;
in vec4 vertex_position;
out vec2 tex_coords;
void main() {
    gl_Position = ortho * (vertex_position + vec4(offset,0,0));
    tex_coords = vec2(vertex_position) * vec2(.5, .5) + vec2(.5, .5);
})zzz"
