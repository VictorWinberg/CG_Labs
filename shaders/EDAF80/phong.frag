#version 410

uniform vec3 ambient;
uniform vec3 diffuse;
uniform vec3 specular;
uniform float shininess;

uniform sampler2D diffuse_texture;
uniform int has_textures;

in VS_OUT {
	vec3 normal;
	vec3 light_vector;
	vec3 camera_vector;
	vec2 texcoord;
} fs_in;

out vec4 frag_color;

vec4 diffuse_color;

void main()
{
	vec3 n = normalize(fs_in.normal);
	vec3 L = normalize(fs_in.light_vector);
	vec3 V = normalize(fs_in.camera_vector);
	vec3 R = normalize(reflect(-L, n));

	if (has_textures != 0)
		diffuse_color = texture(diffuse_texture, fs_in.texcoord);
	else
		diffuse_color = vec4(diffuse, 1.0);

	vec4 diffuse_shading = diffuse_color * dot(n, L);
	vec3 specular_shading = specular * pow(dot(R, V), shininess);
	frag_color = vec4(ambient, 0) + diffuse_shading + vec4(specular_shading, 0);
}
