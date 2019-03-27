R"zzz(#version 330 core
uniform mat4 bone_transform; // transform the cylinder to the correct configuration
const float kPi = 3.1415926535897932384626433832795;
uniform mat4 projection;
uniform mat4 model;
uniform mat4 view;
in vec4 vertex_position;
void main() {
    vec4 cyl = vec4(cos(2*kPi*vertex_position.x), vertex_position.y, sin(2*kPi*vertex_position.x), 1);
    gl_Position = projection* view *bone_transform* cyl;
// FIXME: Implement your vertex shader for cylinders
// Note: you need call sin/cos to transform the input mesh to a cylinder
})zzz"
