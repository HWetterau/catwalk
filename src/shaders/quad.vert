R"zzz(#version 330 core

in vec4 vertex_position;
out vec2 tex_coords;
void main() {
    gl_Position = vertex_position;
    tex_coords = vec2(vertex_position) * vec2(.5, .5) + vec2(.5, .5);
})zzz"
