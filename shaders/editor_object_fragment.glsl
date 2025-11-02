#version 460 core

struct Material
{
	sampler2D texture_diffuse0;
	sampler2D texture_diffuse1;
	sampler2D texture_diffuse2;
	sampler2D texture_diffuse3;
	sampler2D texture_diffuse4;
	sampler2D texture_specular0;
	sampler2D texture_specular1;
	sampler2D texture_specular2;
	sampler2D texture_specular3;
	sampler2D texture_specular4;
	float shininess;
};

out vec4 FragColor;

uniform Material material;
uniform vec3 color;

void main()
{
	FragColor = vec4(color, 1.0);
}

