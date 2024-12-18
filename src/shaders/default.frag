#version 430 core

in vec3 normal;
in vec3 FragPos;
in vec2 texCoords;
in vec3 tangents;
in vec3 bitangents;

out vec4 FragColor;

layout (location = 3) uniform vec3 camPos;
layout (location = 4) uniform uint type;
layout (location = 5) uniform mat4 lightSpaceMatrix;

layout (binding = 0) uniform sampler2D texSampler;
layout (binding = 1) uniform sampler2D normSampler;
layout (binding = 2) uniform sampler2D specSampler;
layout (binding = 3) uniform sampler2D shadowSampler;

vec3 lightPos = vec3(0.0, 10.0, 20.0);

// 計算陰影函數
float getShadow(vec4 fragPosLightSpace)
{
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w; // 投影空間的座標
    projCoords = projCoords * 0.5 + 0.5; // 轉換到 0~1 
    float currentDepth = projCoords.z;

    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(shadowSampler, 0); // 獲取貼圖的大小

    // 使用 3x3 PCF (Percentage Closer Filtering) 進行平滑陰影計算
    for(int x = -1; x <= 1; ++x)
    {
        for(int y = -1; y <= 1; ++y)
        {
            float pcfDepth = texture(shadowSampler, projCoords.xy + vec2(x, y) * texelSize).r; 
            shadow += currentDepth > pcfDepth ? 1.0 : 0.0;        
        }    
    }
    shadow /= 9.0; // 平均陰影值
    
    // 如果 Z 超出範圍，則不算陰影
    if(projCoords.z > 1.0)
        shadow = 0.0;
        
    return shadow;
}

// 簡單隨機抖動函數，用於視覺優化
float rand(vec2 co) {
    return fract(sin(dot(co.xy, vec2(12.9898,78.233))) * 43758.5453);
}

float dither(vec2 uv) {
    return (rand(uv)*2.0-1.0) / 256.0;
}

void main()
{
    // 將 FragPos 轉換到光源的空間
    vec4 fragPosLightSpace = lightSpaceMatrix * vec4(FragPos, 1.0);

    // 設置光的基本顏色和強度
    vec3 lightColor = vec3(1.0, 1.0, 1.0);
    float ambientStrength = 0.4; // 環境光強度
    vec3 ambient = ambientStrength * lightColor;

    // L (光源衰減) 計算參數
    float l_a = 0.001;
    float l_b = 0.01;
    float l_c = 0.001;
    float d = abs(distance(lightPos, FragPos)); // 光源到像素的距離
    float L = 1 / (l_a + d * l_b + d * d * l_c); // 計算衰減因子

    // 法向量處理
    vec3 norm = normalize(normal);
    if(type == 5) { // 如果是帶有法線貼圖的物件
        vec3 tangents = normalize(tangents);
        vec3 bitangents = normalize(bitangents);

        // TBN 矩陣處理法線貼圖
        mat3 TBN = mat3(tangents, bitangents, norm);
        norm = TBN * ((2 * vec3(texture(normSampler, texCoords))) - 1);
        norm = normalize(norm);
    }

    // 計算漫反射光
    vec3 lightDir = normalize(lightPos - FragPos);
    float diff = max(dot(norm, lightDir), 0.0) * L;
    vec3 diffuse = diff * lightColor * 0.5;

    // 計算高光反射
    vec3 reflectDir = reflect(-lightDir, norm);
    vec3 viewDir = normalize(camPos - FragPos);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32) * L;

    // 計算陰影
    float shadow = getShadow(fragPosLightSpace);

    if(type == 5) { // 帶紋理的物件
        vec4 tex = texture(texSampler, texCoords);
        vec3 specular = spec * vec3(texture(specSampler, texCoords)) * 0.5;
        vec3 result = (ambient + (diffuse + specular) * (1.0 - shadow)) * vec3(tex);
        FragColor = vec4(result, tex.w);
    } 
    else { // 黑白棋盤格物件 (例如地板)
        vec3 objectColor = vec3(1.0, 1.0, 1.0); // 預設白色
        // 棋盤格邏輯：根據 FragPos 的 X 和 Z 座標決定顏色
        if ((int(floor(FragPos.x * 2.0) + floor(FragPos.z * 2.0)) & 1) == 0) {
            objectColor = vec3(0.4, 0.4, 0.4); // 黑色格子
        }
        vec3 specular = spec * lightColor * 0.5;
        vec3 result = (ambient + (diffuse + specular) * (1.0 - shadow)) * objectColor + dither(vec2(FragPos.x, FragPos.z));
        FragColor = vec4(result, 1.0);
    }
}
