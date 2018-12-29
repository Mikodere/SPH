#include <GL/glew.h>
#include <GL/glut.h>
#include <iostream>
#include <fstream>
#include <string>
#include <cmath>
#include <math.h>
#include <cstdlib>
#include <ctime>
#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "tga.h"


void initCube();
void drawCube();
void initPoints(int);
void drawPoints(int);

void loadSource(GLuint &shaderID, std::string name);
void printCompileInfoLog(GLuint shadID);
void printLinkInfoLog(GLuint programID);
void validateProgram(GLuint programID);

bool init();
void display();
void resize(int, int);
void idle();
void keyboard(unsigned char, int, int);
void specialKeyboard(int, int, int);
void mouse(int, int, int, int);
void mouseMotion(int, int);

bool fullscreen = false;
bool animation = false; 

int g_Width = 500;                         
int g_Height = 500;                        

GLuint cubeVAOHandle, pointsVAOHandle;
GLuint graphicProgramID[2], computeProgramID[3];
GLuint locUniformMVPM, locUniformMVM, locUniformPM;
GLuint locUniformSpriteTex;
GLuint locUniformSize;

const int WORK_GROUP_SIZE = 256;
const int neighs = 27;

//Uniform variables for the SPH calculations
const int NUM_PARTICLES = 32 * 256;
float tCubo = 1.0;
float partSize = 0.1;
float density0 = 998.f;
float stiffness = 30.0f;
float viscosity = 10.0f;
float surfTens = 1.10f;
const float volume = (tCubo/2) * (tCubo/2) * (tCubo/2);
const float mass = (volume * density0) / NUM_PARTICLES;
const float smoothingLength = pow((3*volume*neighs)/(4*3.14159), (1/3));

//CAMERA
bool mouseLeftDown;
bool mouseRightDown;
float deltaAngleX = 3.14159f / 2;
float deltaAngleY = 3.14159f / 2;
float radio = tCubo + 7.0f;
int xOrigin = -1;
float mouseXPos;
float mouse_Xold_pos;
int yOrigin = -1;
float mouseYPos;
float mouse_Yold_pos;

inline float ranf( float min = 0.0f, float max = 1.0f )
{
	return ((max - min) * rand() / RAND_MAX + min);
}

// cube ///////////////////////////////////////////////////////////////////////
//    v6----- v5
//   /|      /|
//  v1------v0|
//  | |     | |
//  | |v7---|-|v4
//  |/      |/
//  v2------v3

// Vertex array coordiantes  =====================================
GLfloat vertices1[] = {
 tCubo, tCubo, tCubo, 1,  -tCubo, tCubo, tCubo, 1,  -tCubo,-tCubo, tCubo, 1,   tCubo,-tCubo, tCubo, 1,

 tCubo, tCubo, tCubo, 1,   tCubo,-tCubo, tCubo, 1,   tCubo,-tCubo,-tCubo, 1,   tCubo, tCubo,-tCubo, 1,

 tCubo, tCubo, tCubo, 1,   tCubo, tCubo,-tCubo, 1,  -tCubo, tCubo,-tCubo, 1,  -tCubo, tCubo, tCubo, 1,

-tCubo, tCubo, tCubo, 1,  -tCubo, tCubo,-tCubo, 1,  -tCubo,-tCubo,-tCubo, 1,  -tCubo,-tCubo, tCubo, 1,

-tCubo,-tCubo,-tCubo, 1,   tCubo,-tCubo,-tCubo, 1,   tCubo,-tCubo, tCubo, 1,  -tCubo,-tCubo, tCubo, 1,

 tCubo,-tCubo,-tCubo, 1,  -tCubo,-tCubo,-tCubo, 1,  -tCubo, tCubo,-tCubo, 1,   tCubo, tCubo,-tCubo, 1 };

GLfloat colors1[] = {
	0, 0, 0, 1,   0, 0, 0, 1,   0, 0, 0, 1,   0, 0, 0, 1,   // v0-v1-v2-v3 (front)

	0, 0, 0, 1,   0, 0, 0, 1,   0, 0, 0, 1,   0, 0, 0, 1,   // v0-v3-v4-v5 (right)

	0, 0, 0, 1,   0, 0, 0, 1,   0, 0, 0, 1,   0, 0, 0, 1,   // v0-v5-v6-v1 (top)

	0, 0, 0, 1,   0, 0, 0, 1,   0, 0, 0, 1,   0, 0, 0, 1,   // v1-v6-v7-v2 (left)

	0, 0, 0, 1,   0, 0, 0, 1,   0, 0, 0, 1,   0, 0, 0, 1,   // v7-v4-v3-v2 (bottom)

	0, 0, 0, 1,   0, 0, 0, 1,   0, 0, 0, 1,   0, 0, 0, 1 }; // v4-v7-v6-v5 (back)

// BEGIN: Load shaders functions//

void loadSource(GLuint &shaderID, std::string name) 
{
	std::ifstream f(name.c_str());
	if (!f.is_open()) 
	{
		std::cerr << "File not found " << name.c_str() << std::endl;
		system("pause");
		exit(EXIT_FAILURE);
	}

	// now read in the data
	std::string *source;
	source = new std::string( std::istreambuf_iterator<char>(f),   
						std::istreambuf_iterator<char>() );
	f.close();
   
	// add a null to the string
	*source += "\0";
	const GLchar * data = source->c_str();
	glShaderSource(shaderID, 1, &data, NULL);
	delete source;
}

void printCompileInfoLog(GLuint shadID) 
{
GLint compiled;
	glGetShaderiv( shadID, GL_COMPILE_STATUS, &compiled );
	if (compiled == GL_FALSE)
	{
		GLint infoLength = 0;
		glGetShaderiv( shadID, GL_INFO_LOG_LENGTH, &infoLength );

		GLchar *infoLog = new GLchar[infoLength];
		GLint chsWritten = 0;
		glGetShaderInfoLog( shadID, infoLength, &chsWritten, infoLog );

		std::cerr << "Shader compiling failed:" << infoLog << std::endl;
		system("pause");
		delete [] infoLog;

		exit(EXIT_FAILURE);
	}
}

void printLinkInfoLog(GLuint programID)
{
GLint linked;
	glGetProgramiv( programID, GL_LINK_STATUS, &linked );
	if(! linked)
	{
		GLint infoLength = 0;
		glGetProgramiv( programID, GL_INFO_LOG_LENGTH, &infoLength );

		GLchar *infoLog = new GLchar[infoLength];
		GLint chsWritten = 0;
		glGetProgramInfoLog( programID, infoLength, &chsWritten, infoLog );

		std::cerr << "Shader linking failed:" << infoLog << std::endl;
		system("pause");
		delete [] infoLog;

		exit(EXIT_FAILURE);
	}
}

void validateProgram(GLuint programID)
{
GLint status;
    glValidateProgram( programID );
    glGetProgramiv( programID, GL_VALIDATE_STATUS, &status );

    if( status == GL_FALSE ) 
	{
		GLint infoLength = 0;
		glGetProgramiv( programID, GL_INFO_LOG_LENGTH, &infoLength );

        if( infoLength > 0 ) 
		{
			GLchar *infoLog = new GLchar[infoLength];
			GLint chsWritten = 0;
            glGetProgramInfoLog( programID, infoLength, &chsWritten, infoLog );
			std::cerr << "Program validating failed:" << infoLog << std::endl;
			system("pause");
            delete [] infoLog;

			exit(EXIT_FAILURE);
		}
    }
}

// END:   Load shaders //

// BEGIN: Initialize primitives //

///////////////////////////////////////////////////////////////////////////////
// Init Cube
///////////////////////////////////////////////////////////////////////////////
void initCube() 
{
	GLuint vboHandle;

	glGenVertexArrays (1, &cubeVAOHandle);
	glBindVertexArray (cubeVAOHandle);

	glGenBuffers(1, &vboHandle); 
	glBindBuffer(GL_ARRAY_BUFFER, vboHandle);    
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices1) + sizeof(colors1), NULL, GL_STATIC_DRAW);
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices1), vertices1);
	glBufferSubData(GL_ARRAY_BUFFER, sizeof(vertices1), sizeof(colors1), colors1);

	GLuint loc = glGetAttribLocation(graphicProgramID[0], "aPosition");   
	glEnableVertexAttribArray(loc); 
	glVertexAttribPointer( loc, 4, GL_FLOAT, GL_FALSE, 0, (char *)NULL + 0 ); 
	GLuint loc2 = glGetAttribLocation(graphicProgramID[0], "aColor"); 
	glEnableVertexAttribArray(loc2); 
	glVertexAttribPointer( loc2, 4, GL_FLOAT, GL_FALSE, 0, (char *)NULL + sizeof(vertices1) );

	glBindVertexArray (0);
}

void initPoints(int numPoints) 
{
	std::vector<glm::vec4>	position(NUM_PARTICLES);
	std::vector<glm::vec4>	velocity(NUM_PARTICLES);
	std::vector<glm::vec4>	color(NUM_PARTICLES);
	std::vector<glm::vec3>	forces(NUM_PARTICLES);
	std::vector<float>		densities(NUM_PARTICLES);
	std::vector<float>		pressures(NUM_PARTICLES);

	for (int i = 0; i < NUM_PARTICLES; ++i) {
		// initial position
		position[i] = glm::vec4(ranf(-tCubo/2, tCubo/2), ranf(0, tCubo), ranf(-tCubo/2, tCubo/2), 1.0f);
		//velocity[i] = glm::vec4(ranf(-1.0f, 1.0f), ranf(-1.f, 1.f), ranf(-1.f, 1.f), 0.0f); // Particles move if given an inital velocity
		velocity[i] = glm::vec4(0.0f);
		color[i]	= glm::vec4(0.15f, 0.33f, 0.9f, 1.0f);
		forces[i]	= glm::vec3(0.0f);
		densities[i]= 0.0f;
		pressures[i]= 0.0f;
	}

	GLuint posSSbo;
	GLuint velSSbo;
	GLuint colSSbo;
	GLuint forSSbo;
	GLuint denSSbo;
	GLuint preSSbo;

	glGenBuffers( 1, &posSSbo);
	glGenBuffers( 1, &velSSbo);
	glGenBuffers( 1, &colSSbo);
	glGenBuffers( 1, &forSSbo);
	glGenBuffers( 1, &denSSbo);
	glGenBuffers( 1, &preSSbo);

	//Create SSBO instead of VBO
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, posSSbo);
	glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(glm::vec4) * NUM_PARTICLES, &position[0], GL_STATIC_DRAW);

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, velSSbo);
	glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(glm::vec4) * NUM_PARTICLES, &velocity[0], GL_STATIC_DRAW);

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, forSSbo);
	glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(glm::vec3) * NUM_PARTICLES, &forces[0], GL_STATIC_DRAW);

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, denSSbo);
	glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(float) * NUM_PARTICLES, &densities[0], GL_STATIC_DRAW);

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, preSSbo);
	glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(float) * NUM_PARTICLES, &forces[0], GL_STATIC_DRAW);

	// Activate and bind SSBO to Compute Shader
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, posSSbo);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, velSSbo);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, forSSbo);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, denSSbo);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, preSSbo);

	// Use SSBO as VBO

	glGenVertexArrays (1, &pointsVAOHandle);
	glBindVertexArray (pointsVAOHandle);
   
	glBindBuffer(GL_ARRAY_BUFFER, posSSbo);
	GLuint loc = glGetAttribLocation(graphicProgramID[1], "aPosition");   
	glEnableVertexAttribArray(loc); 
	glVertexAttribPointer( loc, 4, GL_FLOAT, GL_FALSE, 0, (char *)NULL + 0 ); 

	glBindBuffer(GL_ARRAY_BUFFER, denSSbo);
	GLuint den = glGetAttribLocation(graphicProgramID[1], "aDensity");
	glEnableVertexAttribArray(den);
	glVertexAttribPointer(den, 1, GL_FLOAT, GL_FALSE, 0, (char *)NULL + 0);

	glBindBuffer(GL_ARRAY_BUFFER, colSSbo);    
	glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec4) * NUM_PARTICLES, &color[0], GL_STATIC_DRAW);
	GLuint loc2 = glGetAttribLocation(graphicProgramID[1], "aColor"); 
	glEnableVertexAttribArray(loc2); 
	glVertexAttribPointer( loc2, 4, GL_FLOAT, GL_FALSE, 0, (char *)NULL + 0 );

	glBindVertexArray (0);

}

// END: Initialize primitives //

// BEGIN: Drawing functions //

void drawCube()
{
	glBindVertexArray(cubeVAOHandle);
    glDrawArrays(GL_QUADS, 0, 24);
	glBindVertexArray(0);
}

void drawPoints(int numPoints)
{
	glBindVertexArray(pointsVAOHandle);
    glDrawArrays(GL_POINTS, 0, numPoints);
	glBindVertexArray(0);
}

// END: Drawing Functions //

int main(int argc, char *argv[])
{
	glutInit(&argc, argv); 
	glutInitWindowPosition(50, 50);
	glutInitWindowSize(g_Width, g_Height);
	glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH);
	glutCreateWindow("SPH System");
	GLenum err = glewInit();
	if (GLEW_OK != err)
	{
	  /* Problem: glewInit failed, something is seriously wrong. */
	  std::cerr << "Error: " << glewGetErrorString(err) << std::endl;
	  system("pause");
	  exit(-1);
	}
	init();

	glutDisplayFunc(display);
	glutKeyboardFunc(keyboard);
	glutSpecialFunc(specialKeyboard);
	glutMouseFunc(mouse);
	glutMotionFunc(mouseMotion);
	glutReshapeFunc(resize);
	glutIdleFunc(idle);
 
	glutMainLoop();
 
	return EXIT_SUCCESS;
}

bool init()
{
	srand (time(NULL));

	glClearColor(0.79f, 0.79f, 0.79f, 0.0f);
 
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);
	glClearDepth(1.0f);

	glShadeModel(GL_SMOOTH);

	glEnable (GL_BLEND);
	glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	//Compute Densities
	computeProgramID[0] = glCreateProgram();

	GLuint computeShaderDensity = glCreateShader(GL_COMPUTE_SHADER);
	loadSource(computeShaderDensity, "shaders/densities.comp");
	std::cout << "Compiling Compute Shader (Densities)" << std::endl;
	glCompileShader(computeShaderDensity);
	printCompileInfoLog(computeShaderDensity);
	glAttachShader(computeProgramID[0], computeShaderDensity);

	glLinkProgram(computeProgramID[0]);
	printLinkInfoLog(computeProgramID[0]);
	validateProgram(computeProgramID[0]);

	//Compute Forces
	computeProgramID[1] = glCreateProgram();

	GLuint computeShaderForces = glCreateShader(GL_COMPUTE_SHADER);
	loadSource(computeShaderForces, "shaders/forces.comp");
	std::cout << "Compiling Compute Shader (Forces)" << std::endl;
	glCompileShader(computeShaderForces);
	printCompileInfoLog(computeShaderForces);
	glAttachShader(computeProgramID[1], computeShaderForces);

	glLinkProgram(computeProgramID[1]);
	printLinkInfoLog(computeProgramID[1]);
	validateProgram(computeProgramID[1]);

	//Integration step + Collisions
	computeProgramID[2] = glCreateProgram();

	GLuint computeShaderIntegrate = glCreateShader(GL_COMPUTE_SHADER);
	loadSource(computeShaderIntegrate, "shaders/integrate.comp");
	std::cout << "Compiling Compute Shader (Integration)" << std::endl;
	glCompileShader(computeShaderIntegrate);
	printCompileInfoLog(computeShaderIntegrate);
	glAttachShader(computeProgramID[2], computeShaderIntegrate);

	glLinkProgram(computeProgramID[2]);
	printLinkInfoLog(computeProgramID[2]);
	validateProgram(computeProgramID[2]);

	// Graphic shaders program (Cube)
	graphicProgramID[0] = glCreateProgram();

	GLuint vertexShaderCube = glCreateShader(GL_VERTEX_SHADER);
	loadSource(vertexShaderCube, "shaders/cube.vert");
	std::cout << "Compiling Vertex Shader" << std::endl;
	glCompileShader(vertexShaderCube);
	printCompileInfoLog(vertexShaderCube);
	glAttachShader(graphicProgramID[0], vertexShaderCube);

	GLuint fragmentShaderCube = glCreateShader(GL_FRAGMENT_SHADER);
	loadSource(fragmentShaderCube, "shaders/cube.frag");
	std::cout << "Compiling Fragment Shader" << std::endl;
	glCompileShader(fragmentShaderCube);
	printCompileInfoLog(fragmentShaderCube);
	glAttachShader(graphicProgramID[0], fragmentShaderCube);

	glLinkProgram(graphicProgramID[0]);
	printLinkInfoLog(graphicProgramID[0]);
	validateProgram(graphicProgramID[0]);

	// Graphic shaders program (Particles)
	graphicProgramID[1] = glCreateProgram();

	GLuint vertexShaderID = glCreateShader(GL_VERTEX_SHADER);
	loadSource(vertexShaderID, "shaders/particles.vert");
	std::cout << "Compiling Vertex Shader" << std::endl;
	glCompileShader(vertexShaderID);
	printCompileInfoLog(vertexShaderID);
	glAttachShader(graphicProgramID[1], vertexShaderID);

	GLuint fragmentShaderID = glCreateShader(GL_FRAGMENT_SHADER);
	loadSource(fragmentShaderID, "shaders/particles.frag");
	std::cout << "Compiling Fragment Shader" << std::endl;
	glCompileShader(fragmentShaderID);
	printCompileInfoLog(fragmentShaderID);
	glAttachShader(graphicProgramID[1], fragmentShaderID);

	GLuint geometryShaderID = glCreateShader(GL_GEOMETRY_SHADER);
	loadSource(geometryShaderID, "shaders/particles.geom");
	std::cout << "Compiling Geometry Shader" << std::endl;
	glCompileShader(geometryShaderID);
	printCompileInfoLog(geometryShaderID);
	glAttachShader(graphicProgramID[1], geometryShaderID);

	glLinkProgram(graphicProgramID[1]);
	printLinkInfoLog(graphicProgramID[1]);
	validateProgram(graphicProgramID[1]);

	// Load sprite for sphere into particles buffer
	TGAFILE tgaImage;
	GLuint textId;
	glGenTextures (1, &textId);
	char filename[] = "white_sphere.tga";
	if ( LoadTGAFile(filename, &tgaImage) )
	{
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, textId);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, tgaImage.imageWidth, tgaImage.imageHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, tgaImage.imageData);
		glTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	}

	initCube();
	locUniformMVPM = glGetUniformLocation(graphicProgramID[0], "uModelViewProjMatrix");

	initPoints(NUM_PARTICLES);
	locUniformMVM = glGetUniformLocation(graphicProgramID[1], "uModelViewMatrix");
	locUniformPM = glGetUniformLocation(graphicProgramID[1], "uProjectionMatrix");
	locUniformSize = glGetUniformLocation(graphicProgramID[1], "uSize2");
	locUniformSpriteTex = glGetUniformLocation(graphicProgramID[1], "uSpriteTex");

	return true;
}
 
void display()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glm::mat4 Projection = glm::perspective(45.0f, 1.0f * g_Width / g_Height, 1.0f, 1000.0f);

	glm::vec3 cameraPos = glm::vec3(radio * cos(deltaAngleX) * sin(deltaAngleY), radio * cos(deltaAngleY), radio * sin(deltaAngleX) * sin(deltaAngleY));

	float up = 1;
	if (sin(deltaAngleY) < 0)
		up = -1;

	glm::mat4 View = glm::lookAt(cameraPos, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, up, 0.0f));

	glm::mat4 ModelCube = glm::translate(glm::scale(glm::mat4(1.0f), glm::vec3(2.0f, 2.0f, 2.0f)), glm::vec3(0.0f, 0.0f, 0.0f));

	glm::mat4 mvp; // Model-view-projection matrix
	glm::mat4 mv;  // Model-view matrix

	mvp = Projection * View * ModelCube;
	mv = View * ModelCube;

	//Compute shader for densities & pressures
	glUseProgram(computeProgramID[0]);

	glUniform1i(0, NUM_PARTICLES);
	glUniform1f(1, density0);
	glUniform1f(2, mass);
	glUniform1f(3, stiffness);
	glUniform1f(4, smoothingLength);
	glUniform1f(5, partSize);

	glDispatchCompute(NUM_PARTICLES / WORK_GROUP_SIZE, 1, 1);

	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

	//Compute shader for resultant force
	glUseProgram(computeProgramID[1]);
	glUniform1i(0, NUM_PARTICLES);
	glUniform1f(1, density0);
	glUniform1f(2, mass);
	glUniform1f(3, viscosity);
	glUniform1f(4, smoothingLength);
	glUniform1f(5, surfTens);
	
	glDispatchCompute(NUM_PARTICLES / WORK_GROUP_SIZE, 1, 1);
	
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

	//Compute shader for integration step
	glUseProgram(computeProgramID[2]);
	glUniform1i(0, NUM_PARTICLES);
	glUniform1f(1, tCubo);
	glUniform1f(2, partSize);
	
	glDispatchCompute(NUM_PARTICLES / WORK_GROUP_SIZE, 1, 1);
	
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

	// Draw Cube
	glUseProgram(graphicProgramID[0]);

	glUniformMatrix4fv( locUniformMVPM, 1, GL_FALSE, &mvp[0][0]);
	
	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	drawCube();
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	
	// Draw Points
	glUseProgram(graphicProgramID[1]);
	
	glUniformMatrix4fv(locUniformMVM, 1, GL_FALSE, &mv[0][0]);
	glUniformMatrix4fv(locUniformPM, 1, GL_FALSE, &Projection[0][0]);
	glUniform1i(locUniformSpriteTex, 0);
	glUniform1f(locUniformSize, partSize);

	drawPoints(NUM_PARTICLES);

	glUseProgram(0);

	glutSwapBuffers();
}
 
void resize(int w, int h)
{
	g_Width = w;
	g_Height = h;
	glViewport(0, 0, g_Width, g_Height);
}
 
void idle()
{
	glutPostRedisplay();
}
 
void keyboard(unsigned char key, int x, int y)
{
	static bool wireframe = false;
	switch(key)
	{
	case 27 : case 'q': case 'Q':
		glDeleteProgram(graphicProgramID[0]);
		glDeleteProgram(graphicProgramID[1]);
		glDeleteProgram(computeProgramID[0]);
		glDeleteProgram(computeProgramID[1]);
		glDeleteProgram(computeProgramID[2]);
		exit(1); 
		break;
	case 'a': case 'A':
		animation = !animation;
		break;
	case 'r': case 'R':
		init();
		break;
	}
}
 
void specialKeyboard(int key, int x, int y)
{
	if (key == GLUT_KEY_F1)
	{
		fullscreen = !fullscreen;
 
		if (fullscreen)
			glutFullScreen();
		else
		{
			glutReshapeWindow(g_Width, g_Height);
			glutPositionWindow(50, 50);
		}
	}
}
 
void mouse(int button, int state, int x, int y)
{	
	if (button == GLUT_LEFT_BUTTON)
	{
		if (state == GLUT_DOWN)
		{
			mouseLeftDown = true;

			xOrigin = x;
			yOrigin = y;
		}
		else if (state == GLUT_UP)
		{
			mouseLeftDown = false;
		}
	}

	else if (button == GLUT_RIGHT_BUTTON)
	{
		if (state == GLUT_DOWN)
			mouseRightDown = true;
		else if (state == GLUT_UP)
			mouseRightDown = false;
	}

	glutPostRedisplay();
}
 
void mouseMotion(int x, int y)
{
	if (mouseLeftDown)
	{
		deltaAngleX += (x - xOrigin) * 0.01f;
		deltaAngleY -= (y - yOrigin) * 0.01f;
		xOrigin = x;
		yOrigin = y;
	}
	if (mouseRightDown)
	{
		radio -= (y - yOrigin) * (radio * 0.01f);
		yOrigin = y;
	}
}
