#version 330 core
out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoord;

uniform vec3 lightDir;
uniform vec3 lightColor;
uniform vec3 objectColor;
uniform sampler2D texture1;

void main() {
    vec3 norm = normalize(Normal);
    float diff = max(dot(norm, -lightDir), 0.0);
    vec3 diffuse = diff * lightColor;
    vec3 result = (diffuse + 0.1) * texture(texture1, TexCoord).rgb;
    FragColor = vec4(result, 1.0);
}
