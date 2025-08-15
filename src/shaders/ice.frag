#version 330 core
in vec3 vWorldPos;
in vec3 vWorldNormal;
out vec4 FragColor;

uniform vec3 viewPos;
uniform samplerCube skybox;
uniform sampler2D  uReflectTex; // planar reflection
uniform mat4 uRefVP;            // reflection ViewProj

uniform float roughness;        // 0..1
uniform vec3  tint;             // blueish
uniform float mixEnv;           // combien d'env cubemap on ajoute (fallback/offscreen)

float FresnelSchlick(float cosTheta, float F0){
    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}

void main(){
    vec3 N = normalize(vWorldNormal);
    vec3 V = normalize(viewPos - vWorldPos);
    vec3 I = -V;

    // Perturbation "glace" simple (procédurale légère)
    float n = fract(sin(dot(vWorldPos.xy, vec2(12.9898,78.233))) * 43758.5453);
    float bump = (n - 0.5) * 0.12 * (0.3 + 0.7*(1.0-roughness)); // -/+6%
    N = normalize(N + bump);

    // Coordonnées planaires : projeter la position monde dans le clip reflété
    vec4 rc = uRefVP * vec4(vWorldPos, 1.0);
    vec2 ruv = rc.xy / rc.w * 0.5 + 0.5;

    // Echantillon reflet (si hors 0..1 → use skybox)
    vec3 reflPlanar = textureLod(uReflectTex, ruv, roughness * 5.0).rgb;

    // Fallback cubemap (et mélange pour hors-champ/reflets larges)
    vec3 R = reflect(I, N);
    vec3 reflEnv = texture(skybox, R).rgb;

    // Fresnel pour glace (F0 ~ 0.06)
    float F = FresnelSchlick(max(dot(N, V), 0.0), 0.06);
    // Mix : base glacée bleutée + reflet (planar + un peu d'env)
    vec3 base = tint * 0.35;
    vec3 refl = mix(reflPlanar, reflEnv, mixEnv);
    vec3 col  = mix(base, refl, F);

    // Légère désaturation / voile
    col = mix(col, vec3(dot(col, vec3(0.299,0.587,0.114))), 0.08);

    FragColor = vec4(col, 1.0);
}
