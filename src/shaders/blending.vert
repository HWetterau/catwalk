R"zzz(
#version 330 core
uniform vec4 light_position;
uniform vec3 camera_position;

uniform vec3 joint_trans[128];
uniform vec4 joint_rot[128];

uniform mat4 blend_d_u[128];

in int jid0;
in int jid1;
in float w0;
in vec3 vector_from_joint0;
in vec3 vector_from_joint1;
in vec4 normal;
in vec2 uv;
in vec4 vert;

out vec4 vs_light_direction;
out vec4 vs_normal;
out vec2 vs_uv;
out vec4 vs_camera_direction;

vec3 qtransform(vec4 q, vec3 v) {
	return v + 2.0 * cross(cross(v, q.xyz) - q.w*v, q.xyz);
}

void main() {



	// linear skin blending	
	if (w0 == 1) {
		//one weight
		gl_Position = w0*(blend_d_u[jid0] * vert);
	} 
	else {
		gl_Position = (w0*(blend_d_u[jid0]* vert)) + ((1-w0)*(blend_d_u[jid1] * vert));
	}
	// FIXME: Implement linear skinning here
	//gl_Position = vert;
	vs_normal = normal;
	vs_light_direction = light_position - gl_Position;
	vs_camera_direction = vec4(camera_position, 1.0) - gl_Position;
	vs_uv = uv;
}
)zzz"
