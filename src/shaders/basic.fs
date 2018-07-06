#version 330

const vec3 _lightDirection = normalize(vec3(0.0,0.0,-1.0));
// de la meme manière, ‘out’ remplace ‘varying’ pour les variables en sortie.

in vec3 v_Normal;
in vec2 v_TexCoords;

in vec3 v_Position;
in vec3 v_CameraPosition;

uniform sampler2D u_SamplerDiffuse;
uniform sampler2D u_SamplerNorm;
uniform sampler2D u_SamplerSpec;

out vec4 Fragment;

void main(void) {
    // auparavant en OpenGL(ES) 2 on ne pouvait specifier que gl_FragColor
    // ou gl_FragData[]. On peut maintenant renommer la variable en sortie

	// obtain normal from normal map in range [0,1]
    vec3 normal = texture(u_SamplerNorm, v_TexCoords).rgb;
    // transform normal vector to range [-1,1]
    normal = normalize(normal * 2.0 - 1.0); 


	vec3 L = -_lightDirection;
	vec3 N = normalize(v_Normal);
	vec3 V = normalize(v_CameraPosition - v_Position);
	vec3 H = normalize(L+V);

	float diffuseFactor = max(dot(N,L),0.0);
	float specularFactor = pow(max(dot(N,H),0.0),64.0);

	// vec3 color = diffuseFactor * vec3(1.0)+ specularFactor* vec3(1.0);
	
	vec3 color = diffuseFactor * texture(u_SamplerDiffuse, v_TexCoords).rgb +
	 specularFactor* texture(u_SamplerSpec, v_TexCoords).rgb;

  	Fragment = vec4(color,1.0);
	
	//Fragment = vec4(1.0,0.0,0.0,1.0);

}