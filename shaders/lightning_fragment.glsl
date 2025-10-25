#version 460 core

out vec4 FragColor;

uniform vec3 objectColor;
uniform vec3 lightColor;

void main()
{
	float ambientStrength = 0.1F;
	vec3 ambient = lightColor * ambientStrength;

	FragColor = vec4(ambient * objectColor, 1.0);
}
