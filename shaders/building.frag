#version 330 core
out vec4 fragColor;
in vec3 normal;
in vec3 pos;
in vec2 texCoord;
uniform sampler2D texture1;
uniform sampler2D normalMapTex;
uniform vec3 lightPos;
uniform vec3 viewPos;
uniform int  uFogEnabled;
uniform float uFogDensity;
vec3 lighting(vec3 objectColor, vec3 p, vec3 n, vec3 lp, vec3 vp,
              vec3 ambient, vec3 lightColor, vec3 specular, float specPower){
    vec3 L = normalize(lp - p);
    vec3 V = normalize(vp - p);
    vec3 N = normalize(n);
    vec3 R = reflect(-L, N);
    float diffCoef = max(dot(N, L), 0.0);
    float specCoef = pow(max(dot(V, R), 0.0), specPower);
    vec3 ambientColor  = ambient * lightColor;
    vec3 diffuseColor  = diffCoef * lightColor;
    vec3 specularColor = specCoef * specular * lightColor;
    vec3 col = (ambientColor + diffuseColor + specularColor) * objectColor;
    return clamp(col, 0.0, 1.0);
}
void main(){
    vec3 objectColor = vec3(texture(texture1, texCoord));
    vec3 normalFromMap = texture(normalMapTex, texCoord).rgb;
    normalFromMap.g = 1.0 - normalFromMap.g;
    normalFromMap = normalFromMap * 2.0 - 1.0;
    vec3 lightColor = vec3(1.0, 1.0, 1.0);
    vec3 ambient    = vec3(0.15);
    vec3 specular   = vec3(0.5);
    float specPower = 32.0;
    vec3 color = lighting(objectColor, pos, normalFromMap, lightPos, viewPos,
                          ambient, lightColor, specular, specPower);
    if (uFogEnabled == 1) {
        float dens = 0.025;
        vec3  fogColor = vec3(0.15, 0.10, 0.08);
        float dist = length(pos - viewPos);
        float f = clamp(exp(-uFogDensity * dist), 0.0, 1.0);
        color = f * color + (1.0 - f) * fogColor;
    }
    fragColor = vec4(color, 1.0);
}
