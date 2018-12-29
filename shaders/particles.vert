#version 430  

in vec4 aPosition;
in vec4 aColor;
in float aDensity;

uniform mat4 uModelViewMatrix;

out vec4 vColor;
out float vDensity;

void main()
{
	vColor = aColor;
    vDensity = aDensity;

    gl_Position = uModelViewMatrix * aPosition;
// Tarea por hacer: modificar la lú‹ea anterior, multipicanodo aPosition por uModelViewMatrix (en vez de por uModelViewProjMatrix)
    
}