R"zzz(#version 330 core
uniform mat4 projection;
uniform mat4 view;
uniform vec4 offset;
in vec4 vertex_position;
void main() {
   vec4 temp = vertex_position+offset;
    temp.w = 1;
    gl_Position = projection * view *(temp);
    //gl_Position = projection * view *(vertex_position);
})zzz"
