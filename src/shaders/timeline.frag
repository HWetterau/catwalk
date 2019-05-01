R"zzz(#version 330 core
uniform sampler2D text;
in vec2 tex_coords;
out vec4 fragment_color;
void main() {
	//fragment_color = vec4(0.64, 0.66, 0.68, 1.0);
    fragment_color = texture(text, tex_coords);
}
)zzz"