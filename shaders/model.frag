#version 330 core
in vec3 vNormal;
in vec3 vWorldPos;
uniform vec4  uColor;
uniform vec3  uLightDir;
uniform vec3  uViewPos;
uniform float uTime;
uniform int   uFogEnabled;
out vec4 fragColor;
uniform float uFogDensity;
void main(){
    vec3 N = normalize(vNormal);
    vec3 L = normalize(uLightDir);
    vec3 V = normalize(uViewPos - vWorldPos);

    float ambient = 0.15;
    float diffuse = max(dot(N, L), 0.0) * 0.8;
    vec3 H = normalize(L + V);
    float spec = pow(max(dot(N, H), 0.0), 32.0) * 0.5;
    float rim = pow(1.0 - max(dot(N, V), 0.0), 2.0) * 0.7;
    float pulse = 0.9 + 0.1 * sin(uTime * 3.0 + vWorldPos.z * 0.3);
    float lighting = (ambient + diffuse + spec + rim) * pulse;

    vec3 surfaceColor = uColor.rgb * lighting;
    if (uFogEnabled == 1) {
        float dens = 0.025;
        vec3  fogColor = vec3(0.15, 0.10, 0.08);
        float dist = length(vWorldPos - uViewPos);
        float f = clamp(exp(-uFogDensity * dist), 0.0, 1.0);
        surfaceColor = f * surfaceColor + (1.0 - f) * fogColor;
    }
    fragColor = vec4(surfaceColor, uColor.a);
}
