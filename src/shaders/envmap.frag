#version 330 core
in vec3 vWorldPos;
in vec3 vNormal;
out vec4 FragColor;

uniform vec3 viewPos;
uniform samplerCube skybox;
uniform float mixFactor;
uniform vec3  tint;

void main() {
    vec3 N = normalize(vNormal);
    vec3 V = normalize(viewPos - vWorldPos);
    vec3 R = reflect(-V, N);
    vec3 env = texture(skybox, R).rgb;
    vec3 base = vec3(0.7);
    vec3 color = mix(base, env * tint, clamp(mixFactor,0.0,1.0));
    FragColor = vec4(color, 1.0);
}
