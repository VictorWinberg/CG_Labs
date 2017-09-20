#version 410

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

void main()
{
	vec3 n = normalize(fs_in.normal);
	vec3 t = normalize(fs_in.tangent);
	vec3 b = normalize(fs_in.binormal);
	vec3 L = normalize(fs_in.light_vector);
	vec3 V = normalize(fs_in.camera_vector);

	frag_color = texture(bump_texture, fs_in.texcoord);
}
