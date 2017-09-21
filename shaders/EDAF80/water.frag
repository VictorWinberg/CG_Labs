#version 410

uniform sampler2D bump_texture;
uniform samplerCube cube_map_texture;

in VS_OUT {
	vec3 normal;
	vec2 texcoord;
	vec3 tangent;
	vec3 binormal;
	vec3 light_vector;
	vec3 camera_vector;
} fs_in;

out vec4 frag_color;

void main()
{
	vec3 n = normalize(fs_in.normal);
	vec3 t = normalize(fs_in.tangent);
	vec3 b = normalize(fs_in.binormal);
	vec3 L = normalize(fs_in.light_vector);
	vec3 V = normalize(fs_in.camera_vector);
	vec3 R = reflect(-V, n);

	// --- Water color ---
	vec4 C_deep = vec4(0.0, 0.0, 0.1, 1.0);
	vec4 C_shallow = vec4(0.0, 0.5, 0.5, 1.0);
	float facing = 1 - max(dot(V, n), 0);
	vec4 C_water = mix(C_deep, C_shallow, facing);

	frag_color = C_water + texture(cube_map_texture, R);
}
