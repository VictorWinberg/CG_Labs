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

void main()
{
	vec3 n = normalize(fs_in.normal);
	vec3 L = normalize(fs_in.light_vector);
	vec3 V = normalize(fs_in.camera_vector);
	vec3 R = normalize(reflect(-L, n));

	vec3 diffuse_color;
	if (has_textures != 0)
		diffuse_color = vec3(texture(diffuse_texture, fs_in.texcoord));
	else
		diffuse_color = diffuse;

	vec3 diffuse_shading = diffuse_color * max(dot(n, L), 0.0);
	vec3 specular_shading = specular * pow(max(dot(R, V), 0.0), shininess);
	frag_color.rgb = ambient + diffuse_shading + specular_shading;
	frag_color.a = 1;
}
