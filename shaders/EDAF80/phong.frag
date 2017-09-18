#version 410

uniform vec3 ambient;
uniform vec3 diffuse;
uniform vec3 specular;
uniform float shininess;

in VS_OUT {
	vec3 normal;
	vec3 light_vector;
	vec3 camera_vector;
} fs_in;

out vec4 frag_color;

void main()
{
	vec3 n = normalize(fs_in.normal);
	vec3 L = normalize(fs_in.light_vector);
	vec3 V = normalize(fs_in.camera_vector);
	vec3 R = normalize(reflect(-L, n));

	vec3 diffuse_shading = diffuse * dot(n, L);
	vec3 specular_shading = specular * pow(dot(R, V), shininess);
	frag_color = vec4(ambient + diffuse_shading + specular_shading, 1.0);
}
