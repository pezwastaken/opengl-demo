#version 460 core

struct Material
{
	sampler2D diffuse;
	sampler2D reflection;
	float shininess;
};

struct Light
{
	vec3 position;

	vec3 ambient;
	vec3 diffuse;
	vec3 reflection;
};


in vec3 normal;
in vec3 fragPos;
in vec2 texCoords;

out vec4 FragColor;

uniform vec3 viewPosition;
uniform Material material;
uniform Light light;

void main()
{
	vec3 ambient = light.ambient * vec3(texture(material.diffuse, texCoords));

	vec3 norm = normalize(normal);
	vec3 lightDirection = normalize(light.position - fragPos);

	float diff = max(dot(lightDirection, norm), 0.0F);
	vec3 diffuse = light.diffuse * diff * vec3(texture(material.diffuse, texCoords));

	vec3 viewDir = normalize(viewPosition - fragPos);
	vec3 reflectDir = reflect(-lightDirection, norm);

	float spec = pow(max(dot(reflectDir, viewDir), 0.0F), material.shininess);
	vec3 reflected = light.reflection * spec * vec3(texture(material.reflection, texCoords));

	vec3 result = (ambient + diffuse + reflected);
	FragColor = vec4(result, 1.0F);
}
