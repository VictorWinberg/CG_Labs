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
	vec3 T = normalize(fs_in.tangent);
	vec3 B = normalize(fs_in.binormal);
	vec3 L = normalize(fs_in.light_vector);
	vec3 V = normalize(fs_in.camera_vector);

	vec3 normal = 2.0 * texture(bump_texture, fs_in.texcoord).rgb - 1;
	normal = normalize(normal);

	mat3 vector_transform;
	vector_transform[0] = T;
	vector_transform[1] = B;
	vector_transform[2] = n;
	normal = vector_transform * normal;

	vec3 R = normalize(reflect(-L, normal));

	vec4 texture_color = texture(diffuse_texture, fs_in.texcoord);

	vec3 fdiffuse = max(dot(normal, L), 0.0) * texture_color.rgb;
	vec3 fspecular = specular * pow(max(dot(R, V), 0.0), shininess);

	frag_color.rgb = ambient + fdiffuse + fspecular;
	frag_color.w = 0;
}
