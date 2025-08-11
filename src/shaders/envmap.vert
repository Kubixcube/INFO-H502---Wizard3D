#version 330 core
layout (location = 0) in vec3 inPos;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec2 inUV;

uniform mat4 M, V, P;

out vec3 vWorldPos;
out vec3 vNormal;

void main() {
    vec4 wp = M * vec4(inPos, 1.0);
    vWorldPos = wp.xyz;
    vNormal   = mat3(transpose(inverse(M))) * inNormal;
    gl_Position = P * V * wp;
}
