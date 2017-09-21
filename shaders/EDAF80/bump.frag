#version 410

uniform vec3 ambient;
// uniform vec3 diffuse;
uniform vec3 specular;
uniform float shininess;

uniform sampler2D diffuse_texture;
uniform sampler2D bump_texture;

in VS_OUT {
	vec3 normal;
	vec2 texcoord;
	vec3 tangent;
	vec3 binormal;
	vec3 light_vector;
	vec3 camera_vector;
} fs_in;

out vec4 frag_color;

vec4 diffuse_color;

void main()
{
	vec3 n = normalize(fs_in.normal);
	vec3 t = normalize(fs_in.tangent);
	vec3 b = normalize(fs_in.binormal);
	vec3 L = normalize(fs_in.light_vector);
	vec3 V = normalize(fs_in.camera_vector);

	vec3 n_bump = 2.0 * texture(bump_texture, fs_in.texcoord).rgb - 1;
	n_bump = normalize(n_bump);

	mat3 tangent_to_model;
	tangent_to_model[0] = t;
	tangent_to_model[1] = b;
	tangent_to_model[2] = n;
	vec3 n_prime = tangent_to_model * n_bump;

	vec3 R = normalize(reflect(-L, n_prime));

	vec4 texture_color = texture(diffuse_texture, fs_in.texcoord);

	vec3 fdiffuse = max(dot(n_prime, L), 0.0) * texture_color.rgb;
	vec3 fspecular = specular * pow(max(dot(R, V), 0.0), shininess);

	frag_color.rgb = ambient + fdiffuse + fspecular;
	frag_color.w = 0;
}
