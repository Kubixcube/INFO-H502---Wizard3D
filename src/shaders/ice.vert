#version 330 core
layout(location=0) in vec3 aPos;
layout(location=1) in vec3 aNormal;

uniform mat4 M, V, P;
uniform vec4 uClipPlane; // world plane (A,B,C,D)

out vec3 vWorldPos;
out vec3 vWorldNormal;

void main(){
    vec4 wp = M * vec4(aPos,1.0);
    vWorldPos = wp.xyz;
    mat3 N = transpose(inverse(mat3(M)));
    vWorldNormal = normalize(N * aNormal);

    // Clip côté réflexion
    gl_ClipDistance[0] = dot(vec4(vWorldPos,1.0), uClipPlane);
    gl_Position = P * V * wp;
}
