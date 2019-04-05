R"zzz(#version 330 core
uniform sampler2D renderedTexture;
in vec2 tex_coords;
out vec4 fragment_color;
void main() {
	fragment_color = texture(renderedTexture, tex_coords);
}
)zzz"