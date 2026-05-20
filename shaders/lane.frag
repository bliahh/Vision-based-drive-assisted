#version 330 core
in float vAlpha;
in vec3 vWorldPos;
uniform vec4  uColor;
uniform float uTime;
uniform vec3  uViewPos;
uniform int   uFogEnabled;
uniform float uFogDensity;

out vec4 fragColor;
void main(){
    float pulse = 0.85 + 0.15 * sin(uTime * 2.5);
    vec3 surfaceColor = uColor.rgb * pulse;
    if (uFogEnabled == 1) {
        float dens = 0.025;
        vec3  fogColor = vec3(0.15, 0.10, 0.08);
        float dist = length(vWorldPos - uViewPos);
        float f = clamp(exp(-uFogDensity * dist), 0.0, 1.0);
        surfaceColor = f * surfaceColor + (1.0 - f) * fogColor;
    }
    fragColor = vec4(surfaceColor, uColor.a * vAlpha);
}
