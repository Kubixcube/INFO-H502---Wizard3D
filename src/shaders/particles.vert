#version 330 core
layout(location=0) in vec2 inOffset;  // coin du quad dans [-0.5,0.5]
layout(location=1) in vec2 inUV;      // UV [0..1]
out vec2 vUV;

uniform mat4 V, P;
uniform vec3 center;   // centre du billboard
uniform float size;    // taille du quad

void main() {
    // axes caméra (billboarding)
    vec3 Right = vec3(V[0][0], V[1][0], V[2][0]);
    vec3 Up    = vec3(V[0][1], V[1][1], V[2][1]);
    vec3 world = center + (inOffset.x * Right + inOffset.y * Up) * size;
    vUV = inUV;
    gl_Position = P * V * vec4(world, 1.0);
}
