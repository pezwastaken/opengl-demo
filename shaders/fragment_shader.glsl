#version 460 core
in vec2 texCoord;

out vec4 FragColor;

uniform sampler2D texture1;
uniform sampler2D texture2;

void main()
{
	// FragColor = vec4(ourColor, 1.0F);
	// FragColor = texture(texture1, texCoord) * vec4(ourColor, 1.0);
	FragColor = mix(texture(texture1, texCoord), texture(texture2, texCoord), 0.2);
}
