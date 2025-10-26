#version 460 core

struct Material
{
	sampler2D diffuse;
	sampler2D reflection;
	float shininess;
};

struct DirectionalLight
{
	vec3 direction;
	vec3 ambient;
	vec3 diffuse;
	vec3 reflection;
};


struct SpotLight
{
	vec3 position;
	vec3 direction;

	vec3 ambient;
	vec3 diffuse;
	vec3 reflection;

	float innerCutOff;
	float outerCutOff;
};

struct PointLight
{
	vec3 position;

	vec3 ambient;
	vec3 diffuse;
	vec3 reflection;

	float constant;
	float linear;
	float quadratic;
};

in vec3 normal;
in vec3 fragPos;
in vec2 texCoords;

out vec4 FragColor;

#define MAX_POINT_LIGHTS 10U

uniform vec3 viewPosition;
uniform Material material;

uniform SpotLight spotLight;
uniform DirectionalLight directionalLight;
uniform PointLight pointLights[MAX_POINT_LIGHTS];
uniform int nPointLights;

vec3 computeDirectionalLight(vec3 viewDir, vec3 normal)
{
	//TODO: dirLight.direction must be normalized
	float diff = max(dot(normal, -directionalLight.direction), 0.0F);

	vec3 reflectDir = reflect(-directionalLight.direction, normal);
	float spec = pow(max(dot(reflectDir, viewDir), 0.0F), material.shininess);

	vec3 ambient = directionalLight.ambient * vec3(texture(material.diffuse, texCoords));
	vec3 diffuse = directionalLight.diffuse * diff * vec3(texture(material.diffuse, texCoords));
	vec3 reflection = directionalLight.reflection * spec *
		vec3(texture(material.reflection, texCoords));

	return ambient + diffuse + reflection;
}

vec3 computePointLight(int lightId, vec3 viewDir, vec3 normal)
{
	PointLight pointLight = pointLights[lightId];

	vec3 lightDirection = normalize(pointLight.position - fragPos);

	float diff = max(dot(normal, lightDirection), 0.0F);

	vec3 reflectDir = reflect(lightDirection, normal);
	float spec = pow(max(dot(reflectDir, viewDir), 0.0F), material.shininess);

	float distance = length(fragPos - pointLight.position);
	float attenuation = 1.0 /
		(pointLight.constant + pointLight.linear * distance +
		pointLight.quadratic * distance * distance);

	vec3 ambient = pointLight.ambient * vec3(texture(material.diffuse, texCoords));
	vec3 diffuse = pointLight.diffuse * diff * vec3(texture(material.diffuse, texCoords));
	vec3 reflection = pointLight.reflection * spec * vec3(texture(material.reflection, texCoords));

	return attenuation * (ambient + diffuse + reflection);
}

vec3 computeSpotLight(vec3 viewDir, vec3 normal)
{
	vec3 ambient = spotLight.ambient * vec3(texture(material.diffuse, texCoords));

	vec3 norm = normalize(normal);
	vec3 fragToLightDirection = normalize(spotLight.position - fragPos);

	//if within the spotlight
	float theta = dot(fragToLightDirection, normalize(-spotLight.direction));

	float epsilon = spotLight.innerCutOff - spotLight.outerCutOff;
	float intensity = clamp((theta - spotLight.outerCutOff) / epsilon, 0.0F, 1.0F);

	float diff = max(dot(fragToLightDirection, norm), 0.0F);
	vec3 diffuse = spotLight.diffuse * diff * vec3(texture(material.diffuse, texCoords));
	diffuse *= intensity;

	vec3 reflectDir = reflect(-fragToLightDirection, norm);

	float spec = pow(max(dot(reflectDir, viewDir), 0.0F), material.shininess);
	vec3 reflected = spotLight.reflection * spec * vec3(texture(material.reflection, texCoords));
	reflected *= intensity;

	vec3 result = ambient + diffuse + reflected;
	return result;
};


void main()
{

	vec3 norm = normalize(normal);
	vec3 viewDir = normalize(viewPosition - fragPos);

	// vec3 result = computeDirectionalLight(viewDir, norm);
	vec3 result = vec3(0,0,0);

	for(int i = 0; i < nPointLights; i++)
	{
		result += computePointLight(i, viewDir, norm);
	}

	result += computeSpotLight(viewDir, norm);
	FragColor = vec4(result, 1.0F);

	// float distance = length(fragPos - light.position);
	// float attenuation = 1.0 /
	// 	(light.constant + light.linear * distance + light.quadratic * distance * distance);
	//
	// vec3 ambient = light.ambient * vec3(texture(material.diffuse, texCoords));
	//
	// vec3 norm = normalize(normal);
	// vec3 lightDirection = normalize(light.position - fragPos);
	//
	// //if within the spotlight
	// float theta = dot(lightDirection, normalize(-light.direction));
	//
	// float epsilon = light.innerCutOff - light.outerCutOff;
	// float intensity = clamp((theta - light.outerCutOff) / epsilon, 0.0F, 1.0F);
	//
	// float diff = max(dot(lightDirection, norm), 0.0F);
	// vec3 diffuse = light.diffuse * diff * vec3(texture(material.diffuse, texCoords));
	// diffuse *= intensity;
	//
	// vec3 viewDir = normalize(viewPosition - fragPos);
	// vec3 reflectDir = reflect(-lightDirection, norm);
	//
	// float spec = pow(max(dot(reflectDir, viewDir), 0.0F), material.shininess);
	// vec3 reflected = light.reflection * spec * vec3(texture(material.reflection, texCoords));
	// reflected *= intensity;
	//
	// vec3 result = ambient + diffuse + reflected;
	// FragColor = vec4(result, 1.0F);


	// vec3 result = attenuation * (ambient + diffuse + reflected);
	// FragColor = vec4(result, 1.0F);
}
