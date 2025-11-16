#version 330 core  
  
in vec3 fragColor;  
in vec2 fragTexCoord;  
in vec3 fragPosition;  
  
out vec4 FragColor;  
  
uniform sampler2D texture0;      // Textura 0  
uniform sampler2D texture1;      // Textura 1  
uniform sampler2D texture2;      // Textura 2  
uniform sampler2D texture3;      // Textura 3  
uniform sampler2D splatMap;      // Mapa de pesos (RGBA)  
uniform bool useSplatting;       // Si está activo el splatting  
uniform bool useTexture;         // Si hay textura simple  
  
void main()  
{  
    if (useSplatting) {  
        // Leer pesos del splatmap  
        vec4 weights = texture(splatMap, fragTexCoord);  
          
        // Muestrear las 4 texturas  
        vec3 tex0 = texture(texture0, fragTexCoord * 10.0).rgb;  
        vec3 tex1 = texture(texture1, fragTexCoord * 10.0).rgb;  
        vec3 tex2 = texture(texture2, fragTexCoord * 10.0).rgb;  
        vec3 tex3 = texture(texture3, fragTexCoord * 10.0).rgb;  
          
        // Mezclar según pesos  
        vec3 finalColor = tex0 * weights.r +  
                         tex1 * weights.g +  
                         tex2 * weights.b +  
                         tex3 * weights.a;  
          
        FragColor = vec4(finalColor, 1.0);  
    } else if (useTexture) {  
        // Textura simple (modo antiguo)  
        FragColor = texture(texture0, fragTexCoord);  
    } else {  
        // Color por vértice (sin textura)  
        FragColor = vec4(fragColor, 1.0);  
    }  
}
