R"zzz(#version 330 core
out vec4 fragment_color;
uniform bool cursor;
void main() {
	if (cursor){
		fragment_color = vec4(0.0,0.0,1.0,1.0);
	} else {
		fragment_color = vec4(0.0, 1.0, 0.0, 1.0);
	}
	
}
)zzz"