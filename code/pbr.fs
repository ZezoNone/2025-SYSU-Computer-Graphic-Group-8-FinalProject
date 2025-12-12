#version 330 core
out vec4 FragColor;
in vec2 TexCoords;
in vec3 WorldPos;
in vec3 Normal;

// material parameters
uniform sampler2D albedoMap;
uniform sampler2D normalMap;
uniform sampler2D metallicMap;
uniform sampler2D roughnessMap;
uniform sampler2D aoMap;
uniform sampler2D metallic_roughnessMap;

uniform bool gltf;

uniform float defaultMetallic; // 无metallicMap时的金属度
uniform float defaultRoughness;// 无roughnessMap时的粗糙度
uniform float defaultAO;       // 无aoMap时的AO值

// glass
uniform bool isGlass;
uniform vec4 glassBaseColor;
uniform float glassMetallic;
uniform float glassRoughness;
uniform bool doubleSided;

// lights
#define MAX_POINT_LIGHTS 4

struct PointLightBase {
    vec3 position;
    vec3 color;
    float far_plane;
};
// 2. 结构体数组（显式 uniform）
uniform PointLightBase pointLightsBase[MAX_POINT_LIGHTS];
// 3. 立方体贴图采样器数组（显式 uniform）
uniform samplerCube pointLightDepthCubemaps[MAX_POINT_LIGHTS];
// 4. 实际光源数量
uniform int pointLightCount;

// shadow

uniform vec3 camPos;

const float PI = 3.14159265359;

vec3 sampleOffsetDirections[20] = vec3[] (
    vec3( 1,  1,  1), vec3( 1, -1,  1), vec3(-1, -1,  1), vec3(-1,  1,  1),
    vec3( 1,  1, -1), vec3( 1, -1, -1), vec3(-1, -1, -1), vec3(-1,  1, -1),
    vec3( 1,  0,  1), vec3(-1,  0,  1), vec3( 1,  0, -1), vec3(-1,  0, -1),
    vec3( 0,  1,  1), vec3( 0, -1,  1), vec3( 0, -1, -1), vec3( 0,  1, -1),
    vec3( 1,  1,  0), vec3( 1, -1,  0), vec3(-1, -1,  0), vec3(-1,  1,  0)
);
// ----------------------------------------------------------------------------
// Easy trick to get tangent-normals to world-space to keep PBR code simplified.
// Don't worry if you don't get what's going on; you generally want to do normal 
// mapping the usual way for performance anyways; I do plan make a note of this 
// technique somewhere later in the normal mapping tutorial.
vec3 getNormalFromMap()
{
    vec3 tangentNormal = texture(normalMap, TexCoords).xyz * 2.0 - 1.0;

    if(tangentNormal == vec3(0))
    {
        vec3 Q1  = dFdx(WorldPos);
        vec3 Q2  = dFdy(WorldPos);
        vec2 st1 = dFdx(TexCoords);
        vec2 st2 = dFdy(TexCoords);

        vec3 N   = normalize(Normal);
        vec3 T  = normalize(Q1*st2.t - Q2*st1.t + 1e-6);
        vec3 B  = -normalize(cross(N, T));
        mat3 TBN = mat3(T, B, N);

        return normalize(TBN * tangentNormal);
    }
    else
    {
        return normalize(Normal);
    }
}
// ----------------------------------------------------------------------------
float DistributionGGX(vec3 N, vec3 H, float roughness)
{
    float a = roughness*roughness;
    float a2 = a*a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH*NdotH;

    float nom   = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return nom / denom;
}
// ----------------------------------------------------------------------------
float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r*r) / 8.0;

    float nom   = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return nom / denom;
}
// ----------------------------------------------------------------------------
float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}
// ----------------------------------------------------------------------------
vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}
// ----------------------------------------------------------------------------
float calculatePointShadow(vec3 fragPos, int lightIndex) {
    // 1. 从全局数组获取光源参数
    vec3 lightPos = pointLightsBase[lightIndex].position;
    float far_plane = pointLightsBase[lightIndex].far_plane;
    
    // 计算片段到光源的方向向量
    vec3 fragToLight = fragPos - lightPos;
    float currentDepth = length(fragToLight);

    // 2. PCF 软阴影计算
    float shadow = 0.0;
    float bias = isGlass ? 100.0 : 0.15;
    int samples = 20;
    float diskRadius = (1.0 + (length(camPos - fragPos) / far_plane)) / 25.0;
    
    for(int i = 0; i < samples; ++i) {
        float closestDepth = texture(pointLightDepthCubemaps[lightIndex], fragToLight + sampleOffsetDirections[i] * diskRadius).r;
        closestDepth *= far_plane;
        if(currentDepth - bias > closestDepth) {
            shadow += 1.0;
        }
    }
    shadow /= float(samples);

    // 超出光源范围无阴影
    if(currentDepth > far_plane) {
        shadow = 0.0;
    }

    return shadow;
}
// ----------------------------------------------------------------------------
void main()
{	
    if (isGlass) 
    {
        vec3 N = getNormalFromMap();
        vec3 V = normalize(camPos - WorldPos);

        if (doubleSided && dot(N, normalize(camPos - WorldPos)) < 0.0) 
        {
            N = -N;
        }

         // 1. 基础参数
        vec3 baseColor = glassBaseColor.rgb;
        float alpha = glassBaseColor.a; // 玻璃基础透明度
        float metallic = glassMetallic;
        float roughness = clamp(glassRoughness, 0.01, 0.99);

        vec3 F0 = vec3(0.04); 
        F0 = mix(F0, baseColor, metallic);

        // reflectance equation
        const float IOR_GLASS = 1.52; // 玻璃折射率
        const float IOR_AIR = 1.0;    // 空气折射率
        const float criticalAngle = asin(IOR_AIR / IOR_GLASS); 
        vec3 Lo = vec3(0.0);

        for(int i = 0; i < pointLightCount; ++i) 
        {   
            PointLightBase light = pointLightsBase[i];

            // calculate per-light radiance
            vec3 L = normalize(light.position - WorldPos);
            vec3 H = normalize(V + L);
            float distance = length(light.position - WorldPos);
            float attenuation = 1.0 / (distance * distance);
            vec3 radiance = light.color * attenuation;

            // Cook-Torrance BRDF
            float NDF = DistributionGGX(N, H, roughness);   
            float G   = GeometrySmith(N, V, L, roughness);      
            vec3 F    = fresnelSchlick(max(dot(H, V), 0.0), F0);
           
            vec3 numerator    = NDF * G * F; 
            float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001; // + 0.0001 to prevent divide by zero
            vec3 specular = numerator / denominator;
            
            // glass
            vec3 transmitLight = vec3(0.0);
            float NdotV = max(dot(N, V), 0.0);
            // 计算折射方向（玻璃→空气：从高IOR到低IOR）
            vec3 transmitDir = refract(-V, N, IOR_GLASS / IOR_AIR); 
            // 判断是否发生全反射（无透射光）
            bool isTotalReflection = length(transmitDir) < 0.001;

            if (!isTotalReflection) {
                // 透射光 = 光源颜色 * 玻璃染色 * 透明度 * 距离衰减 * 厚度因子
                float thicknessFactor = 1.0 - pow(NdotV, 2.0); // 厚度模拟（法线与视角夹角越小，厚度越大）
                transmitLight = light.color * baseColor * alpha * attenuation * thicknessFactor;
                
                // 额外：透射光随粗糙度衰减（粗糙玻璃散射更强）
                transmitLight *= (1.0 - roughness * 0.5);
            }

            // kS is equal to Fresnel
            vec3 kS = F;
            // for energy conservation, the diffuse and specular light can't
            // be above 1.0 (unless the surface emits light); to preserve this
            // relationship the diffuse component (kD) should equal 1.0 - kS.
            vec3 kD = vec3(1.0) - kS;
            // multiply kD by the inverse metalness such that only non-metals 
            // have diffuse lighting, or a linear blend if partly metal (pure metals
            // have no diffuse light).
            kD *= 1.0 - metallic;	  

            // scale light by NdotL
            float NdotL = max(dot(N, L), 0.0);        

            // add to outgoing radiance Lo
            Lo += specular * radiance * NdotL + transmitLight * kD;
            // note that we already multiplied the BRDF by the Fresnel (kS) so we won't multiply by kS again
        }   
    
        // ambient lighting (note that the next IBL tutorial will replace 
        // this ambient lighting with environment lighting).
        vec3 ambient = vec3(0.03) * baseColor * alpha;
    
        vec3 color = ambient + Lo;

        // HDR tonemapping
        color = color / (color + vec3(1.0));
        // gamma correct
        color = pow(color, vec3(1.0/2.2)); 

        FragColor = vec4(color, alpha);
    } 
    else
    {
        vec3 albedo;
        float metallic = 0.0;
        float roughness = 0.5;
        float ao = 1;

        if(!gltf)
        {
            albedo     = pow(texture(albedoMap, TexCoords).rgb, vec3(2.2));
            metallic  = texture(metallicMap, TexCoords).r;
            roughness = texture(roughnessMap, TexCoords).r;
            ao        = texture(aoMap, TexCoords).r;
        }
        else
        {
            albedo     = pow(texture(albedoMap, TexCoords).rgb, vec3(2.2));
            metallic  = texture(metallic_roughnessMap, TexCoords).b;
            roughness = texture(metallic_roughnessMap, TexCoords).g;
            ao = 1.0;
        }

        if(metallic < 0.001)
            metallic = defaultMetallic;
        if(roughness < 0.001)
            roughness = defaultRoughness;
        if(ao < 0.001)
            ao = defaultMetallic;


        vec3 N = getNormalFromMap();
        vec3 V = normalize(camPos - WorldPos);

        // calculate reflectance at normal incidence; if dia-electric (like plastic) use F0 
        // of 0.04 and if it's a metal, use the albedo color as F0 (metallic workflow)    
        vec3 F0 = vec3(0.04); 
        F0 = mix(F0, albedo, metallic);

        // reflectance equation
        vec3 Lo = vec3(0.0);

        for(int i = 0; i < pointLightCount; ++i) 
        {   
            PointLightBase light = pointLightsBase[i];

            // calculate per-light radiance
            vec3 L = normalize(light.position - WorldPos);
            vec3 H = normalize(V + L);
            float distance = length(light.position - WorldPos);
            float attenuation = 1.0 / (distance * distance);
            vec3 radiance = light.color * attenuation;

            // shadow
            float shadow = calculatePointShadow(WorldPos, i);
            float shadowFactor = 1.0 - shadow;

            // Cook-Torrance BRDF
            float NDF = DistributionGGX(N, H, roughness);   
            float G   = GeometrySmith(N, V, L, roughness);      
            vec3 F    = fresnelSchlick(max(dot(H, V), 0.0), F0);
           
            vec3 numerator    = NDF * G * F; 
            float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001; // + 0.0001 to prevent divide by zero
            vec3 specular = numerator / denominator;
        
            // kS is equal to Fresnel
            vec3 kS = F;
            // for energy conservation, the diffuse and specular light can't
            // be above 1.0 (unless the surface emits light); to preserve this
            // relationship the diffuse component (kD) should equal 1.0 - kS.
            vec3 kD = vec3(1.0) - kS;
            // multiply kD by the inverse metalness such that only non-metals 
            // have diffuse lighting, or a linear blend if partly metal (pure metals
            // have no diffuse light).
            kD *= 1.0 - metallic;	  

            // scale light by NdotL
            float NdotL = max(dot(N, L), 0.0);        

            // add to outgoing radiance Lo
            Lo += (kD * albedo / PI + specular) * radiance * NdotL * shadowFactor;
            // note that we already multiplied the BRDF by the Fresnel (kS) so we won't multiply by kS again
        }   
    
        // ambient lighting (note that the next IBL tutorial will replace 
        // this ambient lighting with environment lighting).
        vec3 ambient = vec3(0.03) * albedo * ao;
    
        vec3 color = ambient + Lo;

        // HDR tonemapping
        color = color / (color + vec3(1.0));
        // gamma correct
        color = pow(color, vec3(1.0/2.2)); 

        FragColor = vec4(color, 1.0);
    }
}