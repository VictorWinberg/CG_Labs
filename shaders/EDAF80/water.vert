#version 410

layout (location = 0) in vec3 vertex;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec3 texcoord;
layout (location = 3) in vec3 tangent;
layout (location = 4) in vec3 binormal;

uniform mat4 vertex_model_to_world;
uniform mat4 normal_model_to_world;
uniform mat4 vertex_world_to_clip;

uniform vec3 light_position;
uniform vec3 camera_position;

uniform float A_1;
uniform vec3 D_1;
uniform float f_1;
uniform float p_1;
uniform float k_1;
uniform float time;

out VS_OUT {
	vec3 normal;
	vec2 texcoord;
	vec3 tangent;
	vec3 binormal;
	vec3 light_vector;
	vec3 camera_vector;
} vs_out;

void main()
{
	vec3 vertex_in_world = vec3(vertex_model_to_world * vec4(vertex, 1.0));
	vs_out.normal = vec3(normal_model_to_world * vec4(normal, 0.0));
	vs_out.tangent = vec3(normal_model_to_world * vec4(tangent, 0.0));
	vs_out.binormal = vec3(normal_model_to_world * vec4(binormal, 0.0));
	vs_out.texcoord = vec2(texcoord.x, texcoord.y);
	vs_out.light_vector = light_position - vertex_in_world;
	vs_out.camera_vector = camera_position - vertex_in_world;

	float next_pos = D_1.x * vertex_in_world.x + D_1.z * vertex_in_world.z;
	float next_sin = sin(next_pos * f_1 + time * p_1);
	float height = A_1 * pow(next_sin * 0.5 + 0.5, k_1);
	vertex_in_world.y = height;

	gl_Position = vertex_world_to_clip * vec4(vertex_in_world, 1.0);
}
