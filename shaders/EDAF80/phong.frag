#version 410

uniform vec3 ambient;
uniform vec3 diffuse;
uniform vec3 specular;
uniform float shininess;

uniform vec3 light_position;
uniform vec3 camera_position;

in VS_OUT {
	vec3 vertex;
	vec3 normal;
} fs_in;

out vec4 frag_color;

void main()
{
	vec3 n = fs_in.normal;
	vec3 L = normalize(light_position - fs_in.vertex);
	vec3 V = normalize(camera_position - fs_in.vertex);

	vec3 diffuse_shading = diffuse * dot(n, L);
	vec3 specular_shading = specular * pow(dot(reflect(-L, n), V), shininess);
	frag_color = vec4(ambient + diffuse_shading + specular_shading, 1.0);
}
