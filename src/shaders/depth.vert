R"zzz(
#version 330 core
uniform mat4 light_view;
uniform mat4 projection;
in vec4 vertex_position;
void main() {
	gl_Position = projection* light_view * vertex_position;
	
}
)zzz"
