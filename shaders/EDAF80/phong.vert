#version 410

layout (location = 0) in vec3 vertex;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec3 texcoord;

uniform mat4 vertex_model_to_world;
uniform mat4 normal_model_to_world;
uniform mat4 vertex_world_to_clip;

uniform vec3 light_position;
uniform vec3 camera_position;

out VS_OUT {
	vec3 normal;
	vec3 light_vector;
	vec3 camera_vector;
	vec2 texcoord;
} vs_out;

void main()
{
	vec3 vertex = vec3(vertex_model_to_world * vec4(vertex, 1.0));
	vs_out.normal = vec3(normal_model_to_world * vec4(normal, 0.0));
	vs_out.light_vector = light_position - vertex;
	vs_out.camera_vector = camera_position - vertex;
	vs_out.texcoord = vec2(texcoord.x, texcoord.y);

	gl_Position = vertex_world_to_clip * vertex_model_to_world * vec4(vertex, 1.0);
}
