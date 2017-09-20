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

uniform float amplitude;
uniform vec3 direction;
uniform float frequency;
uniform float phase;
uniform float sharpness;
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

	gl_Position = vertex_world_to_clip * vertex_model_to_world * vec4(vertex, 1.0);
}
