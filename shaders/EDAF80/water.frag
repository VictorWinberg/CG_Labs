#version 410

uniform sampler2D bump_texture;
uniform samplerCube cube_map_texture;

uniform float t;

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

	vec2 texScale = vec2(8, 4);
	vec2 bumpTime = vec2(mod(t, 100.0));
	vec2 bumpSpeed = vec2(-0.05, 0);

	vec2 bumpCoord0 = fs_in.texcoord * texScale + bumpTime * bumpSpeed;
 	vec2 bumpCoord1 = fs_in.texcoord * texScale * 2 + bumpTime * bumpSpeed * 4;
	vec2 bumpCoord2 = fs_in.texcoord * texScale * 4 + bumpTime * bumpSpeed * 8;

	vec4 n0 = texture(bump_texture, bumpCoord0) * 2 - 1;
	vec4 n1 = texture(bump_texture, bumpCoord1) * 2 - 1;
	vec4 n2 = texture(bump_texture, bumpCoord2) * 2 - 1;

	vec3 n_bump = vec3(normalize(n0 + n1 + n2));

	mat3 tangent_to_model;
	tangent_to_model[0] = t;
	tangent_to_model[1] = b;
	tangent_to_model[2] = n;
	vec3 n_prime = tangent_to_model * n_bump;

	vec3 R = normalize(reflect(-L, n_prime));

	// --- Water color ---
	vec4 C_deep = vec4(0.0, 0.0, 0.1, 1.0);
	vec4 C_shallow = vec4(0.0, 0.5, 0.5, 1.0);
	float facing = 1 - max(dot(V, n), 0);
	vec4 C_water = mix(C_deep, C_shallow, facing);

	frag_color = C_water + texture(cube_map_texture, R);
}
