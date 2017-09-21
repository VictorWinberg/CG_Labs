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

uniform float A_2;
uniform vec3 D_2;
uniform float f_2;
uniform float p_2;
uniform float k_2;

uniform float t;

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
	// --- WAVE ONE ---
	vec4 G_1 = vertex_model_to_world * vec4(vertex, 1.0);
	float dPos1 = D_1.x * G_1.x + D_1.z * G_1.z;
	float dSin1 = sin(dPos1 * f_1 + t * p_1) * 0.5 + 0.5;
	float y1 = A_1 * pow(dSin1, k_1);
	G_1.y = y1;

	float dG_1 = 0.5 * k_1 * f_1 * A_1 * pow(dSin1, k_1 - 1) * cos(dPos1 * f_1 + t * p_1);
	float dG_1dx = dG_1 * D_1.x;
	float dG_1dz = dG_1 * D_1.z;

	// --- WAVE TWO ---
	vec4 G_2 = vertex_model_to_world * vec4(vertex, 1.0);
	float dPos2 = D_2.x * G_2.x + D_2.z * G_2.z;
	float dSin2 = sin(dPos2 * f_2 + t * p_2) * 0.5 + 0.5;
	float y2 = A_2 * pow(dSin2, k_2);
	G_2.y = y2;

	float dG_2 = 0.5 * k_2 * f_2 * A_2 * pow(dSin2, k_2 - 1) * cos(dPos2 * f_2 + t * p_2);
	float dG_2dx = dG_2 * D_2.x;
	float dG_2dz = dG_2 * D_2.z;

	// --- ALL WAVES ---
	vec4 H = G_1 + G_2;
	float dHdx = dG_1dx + dG_2dx;
	float dHdz = dG_1dz + dG_2dz;

	vs_out.normal = vec3(-dHdx, 1.0, -dHdz);
	vs_out.binormal = vec3(1.0, dHdx, 0.0);
	vs_out.tangent = vec3(0.0, dHdz, 1.0);
	vs_out.light_vector = light_position - H.xyz;
	vs_out.camera_vector = camera_position - H.xyz;
	vs_out.texcoord = texcoord.xy;

	gl_Position = vertex_world_to_clip * H;
}
