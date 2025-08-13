#version 330 core
layout(location=0) in vec2 aPos;   // positions déjà en NDC [-1..1]
void main() { gl_Position = vec4(aPos, 0.0, 1.0); }
