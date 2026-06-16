#version 410 core

// OLE-01/02/03/04: Cook-Torrance PBR + normal mapping (M1) + shadow mapping (M4).
// Varyings musza byc zgodne z pbr.vert, fish.vert (NED-03) i jellyfish.vert (NED-06).

in vec3 worldPos;
in vec3 worldNormal;
in vec2 texCoord;
in mat3 TBN;

uniform vec3 cameraPos;
uniform vec3 lightDir;
uniform vec3 lightColor;

uniform vec3  albedo;
uniform float metallic;
uniform float roughness;

uniform sampler2D albedoMap;
uniform sampler2D metallicMap;
uniform sampler2D roughnessMap;
uniform sampler2D aoMap;
uniform sampler2D normalMap;       // OLE-03: normal map w przestrzeni stycznej

uniform bool useAlbedoMap;
uniform bool useMetallicMap;
uniform bool useRoughnessMap;
uniform bool useAoMap;
uniform bool useNormalMap;          // OLE-03: flaga wlaczajaca normal mapping

uniform vec2 uvScale;

uniform vec3  fogColor;
uniform float fogDensity;

// --- OLE-04: Shadow mapping (metoda obowiazkowa M4) ---
uniform mat4 lightSpaceMatrix;     // projekcja ortho * view z perspektywy swiatla
uniform sampler2D shadowMap;       // tekstura glebi z przebiegu cieni (2048x2048)
uniform bool useShadows;           // flaga wlaczajaca cienie

out vec4 fragColor;

const float PI = 3.14159265359;

// --- OLE-04: obliczenie wspolczynnika cienia (0.0 = pelny cien, 1.0 = w swietle) ---
float ShadowCalculation(vec4 fragPosLightSpace, vec3 N, vec3 L)
{
    // Perspektywiczny dzielnik (dla ortho = 1, ale robimy poprawnie)
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;

    // Przeksztalcenie z NDC [-1,1] do texcoords [0,1]
    projCoords = projCoords * 0.5 + 0.5;

    // Fragmenty poza frustum swiatla - brak cienia
    if (projCoords.z > 1.0)
        return 1.0;

    // Bias zalezny od kata miedzy normalna a kierunkiem swiatla
    // Im bardziej pod katem, tym wiekszy bias (zapobiega shadow acne)
    float bias = max(0.05 * (1.0 - dot(N, L)), 0.005);

    // --- PCF 3x3: filtrowanie mapy cieni na miekkie krawedzie ---
    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(shadowMap, 0);

    for (int x = -1; x <= 1; ++x)
    {
        for (int y = -1; y <= 1; ++y)
        {
            float pcfDepth = texture(shadowMap, projCoords.xy + vec2(x, y) * texelSize).r;
            shadow += (projCoords.z - bias) > pcfDepth ? 1.0 : 0.0;
        }
    }
    shadow /= 9.0; // srednia z 9 probek

    return 1.0 - shadow; // 1.0 = w swietle, 0.0 = pelny cien
}

float DistributionGGX(vec3 N, vec3 H, float rough)
{
    float a  = rough * rough;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;
    return a2 / max(denom, 0.0001);
}

float GeometrySchlickGGX(float NdotV, float rough)
{
    float r = rough + 1.0;
    float k = (r * r) / 8.0;
    return NdotV / (NdotV * (1.0 - k) + k);
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float rough)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    return GeometrySchlickGGX(NdotV, rough) * GeometrySchlickGGX(NdotL, rough);
}

vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

vec3 ReinhardToneMapping(vec3 hdr)
{
    return hdr / (hdr + vec3(1.0));
}

void main()
{
    vec2 uv = texCoord * uvScale;

    vec3 albedoColor = albedo;
    if (useAlbedoMap)
        albedoColor = pow(texture(albedoMap, uv).rgb, vec3(2.2));

    float metallicVal = metallic;
    if (useMetallicMap)
        metallicVal = texture(metallicMap, uv).r;

    float roughnessVal = roughness;
    if (useRoughnessMap)
        roughnessVal = texture(roughnessMap, uv).r;

    float aoVal = 1.0;
    if (useAoMap)
        aoVal = texture(aoMap, uv).r;

    // --- OLE-03: Normal mapping (metoda obowiazkowa M1) ---
    vec3 N;
    if (useNormalMap)
    {
        vec3 tangentNormal = texture(normalMap, uv).rgb * 2.0 - 1.0;
        N = normalize(TBN * tangentNormal);
    }
    else
    {
        N = normalize(worldNormal);
    }

    vec3 V = normalize(cameraPos - worldPos);
    vec3 L = normalize(lightDir);
    vec3 H = normalize(V + L);

    vec3 F0 = mix(vec3(0.04), albedoColor, metallicVal);

    float NDF = DistributionGGX(N, H, roughnessVal);
    float G   = GeometrySmith(N, V, L, roughnessVal);
    vec3  F   = fresnelSchlick(max(dot(H, V), 0.0), F0);

    vec3 specular = (NDF * G * F) / max(4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0), 0.0001);

    vec3 kS = F;
    vec3 kD = (vec3(1.0) - kS) * (1.0 - metallicVal);

    float NdotL = max(dot(N, L), 0.0);
    vec3 Lo = (kD * albedoColor / PI + specular) * lightColor * NdotL;

    // --- OLE-04: wpiniecie wspolczynnika cienia w wyjscie PBR ---
    // Cien wplywa TYLKO na swiatlo kierunkowe (Lo), ambient pozostaje nienaruszony.
    if (useShadows)
    {
        vec4 fragPosLightSpace = lightSpaceMatrix * vec4(worldPos, 1.0);
        float shadowFactor = ShadowCalculation(fragPosLightSpace, N, L);
        Lo *= shadowFactor;
    }

    vec3 ambient = vec3(0.03) * albedoColor * aoVal;
    vec3 color = ambient + Lo;

    float dist = length(cameraPos - worldPos);
    float fog = 1.0 - exp(-fogDensity * dist);
    color = mix(color, fogColor, clamp(fog, 0.0, 1.0));

    color = ReinhardToneMapping(color);
    color = pow(color, vec3(1.0 / 2.2));

    fragColor = vec4(color, 1.0);
}

