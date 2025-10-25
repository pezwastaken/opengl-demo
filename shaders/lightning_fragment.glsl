#version 460 core

in vec3 normal;
in vec3 fragPos;

out vec4 FragColor;

uniform vec3 objectColor;
uniform vec3 lightColor;
uniform vec3 lightPosition;
uniform vec3 viewPosition;

void main()
{
	float ambientStrength = 0.1F;
	vec3 ambient = lightColor * ambientStrength;

	vec3 norm = normalize(normal);
	vec3 lightDirection = normalize(lightPosition - fragPos);

	float diff = max(dot(lightDirection, norm), 0.0F);
	vec3 diffuse = diff * lightColor;

	vec3 viewDir = normalize(viewPosition - fragPos);
	vec3 reflectDir = reflect(-lightDirection, norm);

	float spec = pow(max(dot(reflectDir, viewDir), 0.0F), 32);
	float specularStrength = 0.5F;
	vec3 reflected = lightColor * spec * specularStrength;

	vec3 result = (ambient + diffuse + reflected) * objectColor;
	FragColor = vec4(result, 1.0F);


	// FragColor = vec4(ambient * objectColor, 1.0);
}
