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

	// --- Animated Bump mapping ---
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

	// --- Tangent space ---
	mat3 tangent_to_model;
	tangent_to_model[0] = b;
	tangent_to_model[1] = t;
	tangent_to_model[2] = n;
	vec3 n_prime = tangent_to_model * n_bump;
	n_prime = normalize(n_prime);

	vec3 R = normalize(reflect(-V, n_prime));
	vec4 reflection = texture(cube_map_texture, R);

	// --- Water color ---
	vec4 C_deep = vec4(0.0, 0.0, 0.1, 1.0);
	vec4 C_shallow = vec4(0.0, 0.5, 0.5, 1.0);
	float facing = 1 - max(dot(V, n_prime), 0.0);
	vec4 C_water = mix(C_deep, C_shallow, facing);

	// --- Fresnel terms
	float n_water = 1.33f;
	float n_air = 1.0f;
	float R_0 = pow((n_water - n_air)/(n_water + n_air), 2);
	float fresnel = R_0 + (1 - R_0) * pow(1 - max(dot(V, n_prime), 0), 5);

	// --- Refraction ---
	vec3 R_frac = refract(-V, n_prime, n_air / n_water);
	vec4 refraction = texture(cube_map_texture, R_frac);

	frag_color = C_water + reflection * fresnel + refraction * (1 - fresnel);
}
