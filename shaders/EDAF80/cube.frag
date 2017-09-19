#version 410

uniform samplerCube cube_map_texture;

in VS_OUT {
	vec3 normal;
} fs_in;

out vec4 frag_color;

void main()
{
	vec3 N = normalize(fs_in.normal);
	frag_color = texture(cube_map_texture, N);
}
