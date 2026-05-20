#version 330 core
in float vAlpha;
in vec3 vWorldPos;
uniform vec4 uColor;
uniform vec3 uViewPos;
uniform int  uFogEnabled;
uniform float uFogDensity;
uniform sampler2D uTex;
in vec2 vTexCoord;
out vec4 fragColor;
void main(){
    vec3 texColor = vec3(texture(uTex, vTexCoord)) * uColor.rgb;
    if (uFogEnabled == 1) {
        float dens = 0.025;
        vec3  fogColor = vec3(0.15, 0.10, 0.08);
        float dist = length(vWorldPos - uViewPos);
        float f = clamp(exp(-uFogDensity * dist), 0.0, 1.0);
        texColor = f * texColor + (1.0 - f) * fogColor;
    }
    fragColor = vec4(texColor, uColor.a * vAlpha);
}
