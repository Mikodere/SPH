#version 430

in vec4 gColor;
in vec2 gTexCoord;
in float gDensity;

uniform sampler2D uSpriteTex;

out vec4 fFragColor;

void main()
{
// Tarea por hacer: una vez activo el geometry shader, sustuir el anterior código por el siguiente:
	if ( length(gTexCoord - 0.5) > 0.38 ) discard;
	vec4 color = texture(uSpriteTex, gTexCoord);
    float density = clamp(gDensity, 0.0, 1.0);
	fFragColor = mix(color * vec4(vec3(1.0), 0.5), color * gColor, gDensity);
}