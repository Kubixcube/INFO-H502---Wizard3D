
#version 330 core
in vec3 vWorldPos; in vec3 vNormal; in vec2 vUV;
out vec4 FragColor;
uniform sampler2D diffuseMap;
uniform vec3 lightDir, lightColor, ambientColor;
uniform vec3 viewPos, specularColor;
uniform float shininess;
void main(){
    vec3 N=normalize(vNormal);
    vec3 L=normalize(-lightDir);
    float NdotL=max(dot(N,L),0.0);
    vec3 albedo=texture(diffuseMap,vUV).rgb;
    vec3 ambient=ambientColor*albedo;
    vec3 diffuse=(lightColor*NdotL)*albedo;
    vec3 V=normalize(viewPos-vWorldPos);
    vec3 H=normalize(L+V);
    vec3 specular=specularColor*pow(max(dot(N,H),0.0), shininess);
    FragColor=vec4(ambient+diffuse+specular,1.0);
}
