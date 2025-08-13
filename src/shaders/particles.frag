#version 330 core
in vec2 vUV;
out vec4 FragColor;

uniform vec4 color; // rgb + alpha

void main() {
    // petit disque doux
    float r = length(vUV - vec2(0.5));
    float a = smoothstep(0.6, 0.0, r) * color.a;
    FragColor = vec4(color.rgb, a);
}
