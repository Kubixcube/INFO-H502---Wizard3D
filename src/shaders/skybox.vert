#version 330 core
layout (location = 0) in vec3 inPos;

out vec3 TexDir;

uniform mat4 V;
uniform mat4 P;

void main() {
    mat4 rotView = mat4(mat3(V));
    vec4 clip = P * rotView * vec4(inPos, 1.0);
    gl_Position = clip.xyww;
    TexDir = inPos;
}
