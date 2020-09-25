/*
 * FinalProject.cpp
 *
 *  Created on: Aug 11, 2020
 *      Author: tyler
 */
//Header Inclusions
#include <iostream>
#include <GL/glew.h>
#include <GL/freeglut.h>

//GLM Math Header Inclusions
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

//SOIL image loader inclusion
#include "SOIL2/SOIL2.h"

using namespace std; //Standard namespace

#define WINDOW_TITLE "Modern OpenGL" //Window title Macro

//Shader program Macro
#ifndef GLSL
#define GLSL(Version, Source) "#version " #Version "\n" #Source
#endif

//Variable declarations for shader, window size initialization, buffer and array objects
GLint shaderProgram, WindowWidth = 800, WindowHeight = 600;
GLuint VBO, VAO, texture;
GLfloat degrees = glm::radians(0.0f); //converts float to degrees

//Cube and light color
glm::vec3 lightColor(1.0f, 0.0f, 0.0f);
glm::vec3 secondLightColor(1.0f, 1.0f, 1.0f);
//Light position and scale
glm::vec3 lightPosition(1.0f, 0.5f, -3.0f);
                      //ambient   specular    highlight
glm::vec3 lightStrength(1.0f,     1.0f,       0.5f);

GLfloat lastMouseX = 400, lastMouseY = 300; //Locks mouse cursor at the center of the screen
GLfloat mouseXOffset, mouseYOffset, yaw = 0.0f, pitch = 0.0f, zoom = 0.0f; //Mouse offset, yaw, and pitch variables
GLfloat sensitivity = 0.005f; //used for mouse / camera rotation sensitivity
bool mouseDetected = true; //Initially true when mouse movement is detected
bool leftMouseButton = false;
bool rightMouseButton = false;
bool altDown = false;
bool projectionType = true;

//Global vector declarations
glm::vec3 cameraPosition = glm::vec3(0.0f, 0.0f, 8.0f); //Initial camera position. placed -8 units in z
glm::vec3 CameraUpY = glm::vec3(0.0f, 1.0f, 0.0f); //Temporary y unit vector
glm::vec3 CameraForwardZ = glm::vec3(0.0f, 0.0f, -1.0f); //Temporary z unit vector
glm::vec3 front; //Temporary z unit vector for mouse
glm::vec3 cameraRotateAmt;

//function prototypes
void UResizeWindow(int, int);
void URenderGraphics(void);
void UCreateShader(void);
void UCreateBuffers(void);
void UMouseClick(int button, int state, int x, int y);
void UMousePressedMove(int x, int y);
void UKeyboardButton(unsigned char key, int x, int y);
void UGenerateTexture(void);

//Vertex shader source code
const GLchar * vertexShaderSource = GLSL(330,
	layout (location = 0) in vec3 position; //Vertex data from vertex attrib pointer 0
	layout (location = 2) in vec2 textureCoordinate;

	out vec2 mobileTextureCoordinate;

	//global variables for the transform matrices
	uniform mat4 model;
	uniform mat4 view;
	uniform mat4 projection;

	void main(){
			gl_Position = projection * view * model * vec4(position, 1.0f); //transforms vertices to clip coordinates
			mobileTextureCoordinate = vec2(textureCoordinate.x, 1.0f - textureCoordinate.y); //flips the texture horizontal
		}
);

//fragment shader source code
const GLchar * fragmentShaderSource = GLSL(330,
	in vec2 mobileTextureCoordinate;

	out vec4 gpuTexture; //variable to pass color data to the GPU

	uniform sampler2D uTexture; //Useful when working with multiple textures

	void main(){
		gpuTexture = texture(uTexture, mobileTextureCoordinate);
	}
);

const GLchar * lightVertexShaderSource = GLSL(330,
	layout (location = 0) in vec3 position; //VAP position 0 for vertex position data
	layout (location = 1) in vec3 normal; //VAP position 1 for normals
	layout (location = 2) in vec2 textureCoordinate;

	out vec3 Normal; //for outgoing normals to fragment shader
	out vec3 FragmentPos; // for outgoing color / pixels to fragment shader
	out vec2 mobileTextureCoordinate; // uv coords for texture

	//uniform / global variables for the transform matrices
	uniform mat4 model;
	uniform mat4 view;
	uniform mat4 projection;

    void main(){
        gl_Position = projection * view * model * vec4(position, 1.0f);//Transforms vertices into clip coordinates
        Normal = mat3(transpose(inverse(model))) * normal; // get normal vectors in world space only and exclude normal translation properties
        FragmentPos = vec3(model * vec4(position, 1.0f)); //Gets fragment / pixel position in world space only (exclude view and projection)
		mobileTextureCoordinate = vec2(textureCoordinate.x, 1.0f - textureCoordinate.y); //flips the texture horizontal
	}
);

const GLchar * lightFragmentShaderSource = GLSL(330,
	in vec3 Normal; //For incoming normals
	in vec3 FragmentPos; //for incoming fragment position
	in vec2 mobileTextureCoordinate;

	out vec4 result; //for outgoing light color to the GPU

	//Uniform / Global variables for object color, light color, light position and camera/view position
	uniform vec3 lightColor;
	uniform vec3 secondLightColor;
	uniform vec3 lightPos;
	uniform vec3 viewPosition;
    uniform vec3 lightStrength;
	uniform sampler2D uTexture; //Useful when working with multiple textures

    void main(){
    	vec3 norm = normalize(Normal); //Normalize vectors to 1 unit
    	vec3 ambient = lightStrength.x * lightColor; //Generate ambient light color
    	vec3 ambientTwo = lightStrength.x * secondLightColor;//Generate second ambient light color
    	vec3 lightDirection = normalize(lightPos - FragmentPos); //Calculate distance (light direction) between light source and fragments/pixels on
    	float impact = max(dot(norm, lightDirection), 0.0); //Calculate diffuse impact by generating dot product of normal and light
    	vec3 diffuse = impact * lightColor; //Generate diffuse light color
    	vec3 viewDir = normalize(viewPosition - FragmentPos); //Calculate view direction
    	vec3 reflectDir = reflect(-lightDirection, norm); //Calculate reflection vector
    	float specularComponent = pow(max(dot(viewDir, reflectDir), 0.0), lightStrength.z);
    	vec3 specular = lightStrength.y * specularComponent * lightColor;

    	//Calculate phong result
    	vec3 phongOne = (ambient + diffuse + specular) * vec3(texture(uTexture, mobileTextureCoordinate));

    	// hardcode second light position
    	lightDirection = normalize(vec3(6.0f, 0.0f, -3.0f)- FragmentPos);
    	impact = max(dot(norm, lightDirection), 0.0); //Calculate diffuse impact by generating dot product of normal and light
    	diffuse = impact * secondLightColor; //Generate diffuse light color
    	viewDir = normalize(viewPosition - FragmentPos); //Calculate view direction
    	reflectDir = reflect(-lightDirection, norm); //Calculate reflection vector
    	specularComponent = pow(max(dot(viewDir, reflectDir), 0.0), lightStrength.z);
    	// hardcode second light spec
    	vec3 specularTwo = 0.1f * specularComponent * secondLightColor;

    	vec3 phongTwo = (ambientTwo + diffuse + specularTwo) * vec3(texture(uTexture, mobileTextureCoordinate));

    	result = vec4(phongOne + phongTwo, 1.0f); //Send lighting results to GPU
	}
);

//main program
int main(int argc, char* argv[])
{
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DEPTH | GLUT_DOUBLE | GLUT_RGBA);
	glutInitWindowSize(WindowWidth, WindowHeight);
	glutCreateWindow(WINDOW_TITLE);
	glutKeyboardFunc(UKeyboardButton);

	glutReshapeFunc(UResizeWindow);

	glewExperimental = GL_TRUE;
			if (glewInit() != GLEW_OK)
			{
				std::cout << "Failed to initialize GLEW" << std::endl;
				return -1;
			}

	UCreateShader();

	UCreateBuffers();

	UGenerateTexture();

	//use the shader program
	glUseProgram(shaderProgram);

	glClearColor(0.0f, 0.0f, 0.0f, 1.0f); //Set background color

	glutDisplayFunc(URenderGraphics);

	glutMouseFunc(UMouseClick);//detects mouse movement

	glutMotionFunc(UMousePressedMove); //Detects mouse press and movement.

	glutMainLoop();

	//Destroys buffer objects once used
	glDeleteVertexArrays(1, &VAO);
	glDeleteBuffers(1, &VBO);

	return 0;
}

//Resize the window
void UResizeWindow(int w, int h)
{
	WindowWidth = w;
	WindowHeight = h;
	glViewport(0, 0, WindowWidth, WindowHeight);
}

//Renders Graphics
void URenderGraphics(void)
{
	glEnable(GL_DEPTH_TEST); //Enable z-depth

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); //clears the screen

	glBindVertexArray(VAO); //Activate the Vertex array object before rendering and transforming them
	front.x = 10.0f * cos(yaw);
	front.y = 10.0f * sin(pitch);
	front.z = sin(yaw) * cos(pitch) * 10.0f;

	//Transforms the object
	glm::mat4 model;
	model = glm::scale(model, glm::vec3(2.0f, 2.0f, 2.0f)); //Increase the object size by a scale of 2
	model = glm::translate(model, glm::vec3(0.0, 0.0f, 0.0f)); //Place the object at the center of the view port

	//Transforms the camera
	glm::mat4 view;
	view = glm::lookAt(cameraPosition, CameraForwardZ, CameraUpY);
	view = glm::rotate(view, cameraRotateAmt.x, glm::vec3(0.0f, 1.0f, 0.0f));
	view = glm::rotate(view, cameraRotateAmt.y, glm::vec3(1.0f, 0.0f, 0.0f));
	view = glm::translate(view, glm::vec3(0.0f, 0.0f, zoom));

	glm::mat4 projection;

	//ortho \ perspective switch
	if (projectionType == true){
		//Creates a perspective projection
		projection = glm::perspective(45.0f, (GLfloat)WindowWidth / (GLfloat)WindowHeight, 0.1f, 100.0f);
	}
	else {
		projection = glm::ortho(-3.5f, 5.0f, -3.5f, 5.0f,0.0f,100.0f);
	};

	//retrieves and passes transform matrices to the shader program
	GLint modelLoc = glGetUniformLocation(shaderProgram, "model");
	GLint viewLoc = glGetUniformLocation(shaderProgram, "view");
	GLint projLoc = glGetUniformLocation(shaderProgram, "projection");
	GLint secondLightColorLoc, lightColorLoc, lightPositionLoc, lightStrengthLoc, viewPositionLoc;

	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
	glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
	glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

	lightColorLoc = glGetUniformLocation(shaderProgram, "lightColor");
	lightPositionLoc = glGetUniformLocation(shaderProgram, "lightPos");
    lightStrengthLoc = glGetUniformLocation(shaderProgram, "lightStrength");
    secondLightColorLoc = glGetUniformLocation(shaderProgram, "secondLightColor");
	viewPositionLoc = glGetUniformLocation(shaderProgram, "viewPosition");

	//pass color, light, and camera data to the cube shader programs corresponding uniforms
	glUniform3f(lightColorLoc, lightColor.r, lightColor.g, lightColor.b);
	glUniform3f(secondLightColorLoc, secondLightColor.r, secondLightColor.g, secondLightColor.b);
	glUniform3f(lightPositionLoc, lightPosition.x, lightPosition.y, lightPosition.z);
	glUniform3f(lightStrengthLoc, lightStrength.x, lightStrength.y, lightStrength.z);
	glUniform3f(viewPositionLoc, cameraPosition.x, cameraPosition.y, cameraPosition.z);

	glutPostRedisplay();

	glBindTexture(GL_TEXTURE_2D, texture);

	//Draws the triangles
	glDrawArrays(GL_TRIANGLES, 0, 144);

	glBindVertexArray(0); //deactivate the vertex array object

	glutSwapBuffers(); //flips the back buffer with the front buffer every frame. similar to gl flush
}

//Creates the Shader Program
void UCreateShader()
{
	//Vertex shader
	GLint vertexShader = glCreateShader(GL_VERTEX_SHADER); // creates the vertex shader
	glShaderSource(vertexShader, 1, &lightVertexShaderSource, NULL); //Attaches the vertex shader to the source code
	glCompileShader(vertexShader); //compiles the vertex shader

	//Fragment shader
	GLint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER); //Creates the fragment shader
	glShaderSource(fragmentShader, 1, &lightFragmentShaderSource, NULL); //Attaches the fragment shader to the source code
	glCompileShader(fragmentShader); //compiles the fragment shader

	//Shader Program
	shaderProgram = glCreateProgram(); //Creates the shader program and returns an id
	glAttachShader(shaderProgram, vertexShader); //Attach vertex shader to the shader program
	glAttachShader(shaderProgram, fragmentShader); //Attach fragment shader to the shader program
	glLinkProgram(shaderProgram); //link vertex and fragment shader to shader program

	//delete the vertex and fragment shaders once linked
	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);
}

void UCreateBuffers()
{
	GLfloat vertices[] = {
				//X   //Y   //Z				//Normals					//Texture Coordinates
				//BookShelf Left outer Wall
				-0.05f,  1.1f,  -0.25f,    	-1.0f,  0.0f, 0.0f,			1.0f, 1.0f, // Vertex 0
				-0.05f,  1.1f,   0.25f,    	-1.0f,  0.0f, 0.0f,			0.0f, 1.0f, // 1
				-0.05f, -0.1f,  -0.25f,    	-1.0f,  0.0f, 0.0f,			0.0f, 0.0f, // 2
				-0.05f, -0.1f,  -0.25f,    	-1.0f,  0.0f, 0.0f,			0.0f, 0.0f, // 3
				-0.05f, -0.1f,   0.25f,    	-1.0f,  0.0f, 0.0f,			1.0f, 0.0f, // 4
				-0.05f,  1.1f,   0.25f,    	-1.0f,  0.0f, 0.0f,			1.0f, 1.0f, // 5
				//BookShelf Left Inner Wall
				 0.05f, 1.0f, -0.25f,	 	-1.0f,	0.0f, 0.0f,			0.0f,  1.0f, //6
				 0.05f, 1.0f,  0.25f,	 	-1.0f,	0.0f, 0.0f,			0.0f,  0.0f, //7
				 0.05f, 0.0f, -0.25f,	 	-1.0f,	0.0f, 0.0f,			1.0f,  1.0f, //8
				 0.05f, 0.0f, -0.25f,	 	-1.0f,	0.0f, 0.0f,			1.0f,  1.0f, //9
				 0.05f, 0.0f,  0.25f,	 	-1.0f,	0.0f, 0.0f,			0.0f,  0.0f, //10
				 0.05f, 1.0f,  0.25f,	 	-1.0f,	0.0f, 0.0f,			1.0f,  0.0f, //11
				//BookShelf Left Face Wall
				-0.05f, 1.0f, 0.25,		 	-1.0f,  0.0f, 0.0f, 		0.0f, 1.0f, //12
				-0.05f, 0.0f, 0.25,		 	-1.0f,  0.0f, 0.0f, 		0.0f, 0.0f, //13
				 0.05f, 1.0f, 0.25,		 	-1.0f,  0.0f, 0.0f, 		1.0f, 1.0f, //14
				 0.05f, 1.0f, 0.25,		 	-1.0f,  0.0f, 0.0f, 		1.0f, 1.0f, //15
				-0.05f, 0.0f, 0.25,		 	-1.0f,  0.0f, 0.0f, 		0.0f, 0.0f, //16
				 0.05f, 0.0f, 0.25,		 	-1.0f,  0.0f, 0.0f, 		1.0f, 0.0f, //17
				//BookShelf Left Back Wall
				-0.05f, 1.0f, -0.25,	 	-1.0f,  0.0f, 0.0f, 		0.0f, 1.0f, //18
				-0.05f, 0.0f, -0.25,	 	-1.0f,  0.0f, 0.0f, 		0.0f, 0.0f, //19
				 0.05f, 1.0f, -0.25,	 	-1.0f,  0.0f, 0.0f, 		1.0f, 1.0f, //20
				 0.05f, 1.0f, -0.25,	 	-1.0f,  0.0f, 0.0f, 		1.0f, 1.0f, //21
				-0.05f, 0.0f, -0.25,	 	-1.0f,  0.0f, 0.0f, 		0.0f, 0.0f, //22
				 0.05f, 0.0f, -0.25,	 	-1.0f,  0.0f, 0.0f, 		1.0f, 0.0f, //23
				//BookShelf Right Inner Wall
				 0.95f, 1.0f, -0.25f,    	1.0f,  0.0f,  0.0f,			1.0f, 1.0f, //24
				 0.95f, 1.0f,  0.25f,    	1.0f,  0.0f,  0.0f,			0.0f, 1.0f, //25
				 0.95f, 0.0f, -0.25f,    	1.0f,  0.0f,  0.0f,			0.0f, 0.0f, //26
				 0.95f, 0.0f, -0.25f,    	1.0f,  0.0f,  0.0f,			0.0f, 0.0f, //27
				 0.95f, 0.0f,  0.25f,    	1.0f,  0.0f,  0.0f,			1.0f, 0.0f, //28
				 0.95f, 1.0f,  0.25f,    	1.0f,  0.0f,  0.0f,			1.0f, 1.0f, //29
				//BookShelf Right Outer Wall
				 1.05f,  1.1f,  -0.25f, 	1.0f,  0.0f,  0.0f,			1.0f, 1.0f, // 30
				 1.05f,  1.1f,   0.25f, 	1.0f,  0.0f,  0.0f,			0.0f, 1.0f, // 31
				 1.05f, -0.1f,  -0.25f, 	1.0f,  0.0f,  0.0f,			0.0f, 0.0f, // 32
				 1.05f, -0.1f,  -0.25f, 	1.0f,  0.0f,  0.0f,			0.0f, 0.0f, // 33
				 1.05f, -0.1f,   0.25f, 	1.0f,  0.0f,  0.0f,			1.0f, 0.0f, // 34
				 1.05f,  1.1f,   0.25f, 	1.0f,  0.0f,  0.0f,			1.0f, 1.0f, // 35
				//BookShelf Right Face Wall
				 0.95f, 1.0f, 0.25f,		1.0f,  0.0f,  0.0f,			0.0f, 1.0f, //36
				 0.95f, 0.0f, 0.25f,		1.0f,  0.0f,  0.0f,			0.0f, 0.0f, //37
				 1.05f, 1.0f, 0.25f,		1.0f,  0.0f,  0.0f,			1.0f, 1.0f, //38
				 1.05f, 1.0f, 0.25f,		1.0f,  0.0f,  0.0f,			1.0f, 1.0f, //39
				 0.95f, 0.0f, 0.25f,		1.0f,  0.0f,  0.0f,			0.0f, 0.0f, //40
				 1.05f, 0.0f, 0.25f,		1.0f,  0.0f,  0.0f,			0.0f, 1.0f, //41
				//BookShelf Right Back Wall
				 0.95f, 1.0f, -0.25f,		1.0f,  0.0f,  0.0f,			0.0f, 1.0f, //42
				 0.95f, 0.0f, -0.25f,		1.0f,  0.0f,  0.0f,			0.0f, 0.0f, //43
				 1.05f, 1.0f, -0.25f,		1.0f,  0.0f,  0.0f,			1.0f, 1.0f, //44
				 1.05f, 1.0f, -0.25f,		1.0f,  0.0f,  0.0f,			1.0f, 1.0f, //45
				 0.95f, 0.0f, -0.25f,		1.0f,  0.0f,  0.0f,			0.0f, 0.0f, //46
				 1.05f, 0.0f, -0.25f,		1.0f,  0.0f,  0.0f,			0.0f, 1.0f, //47
				//BookShelf Top Inner Wall
				-0.05f, 1.0f, -0.25f, 		0.0f,  1.0f,  0.0f,			0.0f, 1.0f, //48
				-0.05f, 1.0f,  0.25f, 		0.0f,  1.0f,  0.0f,			0.0f, 0.0f, //49
				 1.05f, 1.0f, -0.25f, 		0.0f,  1.0f,  0.0f,			1.0f, 1.0f, //50
				 1.05f, 1.0f, -0.25f, 		0.0f,  1.0f,  0.0f,			1.0f, 1.0f, //51
				-0.05f, 1.0f,  0.25f, 		0.0f,  1.0f,  0.0f,			0.0f, 0.0f, //52
				 1.05f, 1.0f,  0.25f, 		0.0f,  1.0f,  0.0f,			1.0f, 0.0f, //53
				//BookShelf Top Outer Wall
				-0.05f, 1.1f, -0.25f, 		0.0f,  1.0f,  0.0f,			0.0f, 1.0f, //54
				-0.05f, 1.1f,  0.25f, 		0.0f,  1.0f,  0.0f,			0.0f, 0.0f, //55
				 1.05f, 1.1f, -0.25f, 		0.0f,  1.0f,  0.0f,			1.0f, 1.0f, //56
				 1.05f, 1.1f, -0.25f, 		0.0f,  1.0f,  0.0f,			1.0f, 1.0f, //57
				-0.05f, 1.1f,  0.25f, 		0.0f,  1.0f,  0.0f,			0.0f, 0.0f, //58
				 1.05f, 1.1f,  0.25f, 		0.0f,  1.0f,  0.0f,			1.0f, 0.0f, //59
				//BookShelf Top Face Wall
				-0.05f, 1.1f, 0.25f,		0.0f,  1.0f,  0.0f,			0.0f, 1.0f, //60
				-0.05f, 1.0f, 0.25f,		0.0f,  1.0f,  0.0f,			0.0f, 0.0f, //61
				 1.05f, 1.1f, 0.25f,		0.0f,  1.0f,  0.0f,			1.0f, 1.0f, //62
				 1.05f, 1.1f, 0.25f,		0.0f,  1.0f,  0.0f,			1.0f, 1.0f, //63
				-0.05f, 1.0f, 0.25f,		0.0f,  1.0f,  0.0f,			0.0f, 0.0f, //64
				 1.05f, 1.0f, 0.25f,		0.0f,  1.0f,  0.0f,			1.0f, 0.0f, //65
				//BookShelf Top Back Wall
				-0.05f, 1.1f, -0.25f,		0.0f,  1.0f,  0.0f,			0.0f, 1.0f, //66
				-0.05f, 1.0f, -0.25f,		0.0f,  1.0f,  0.0f,			0.0f, 0.0f, //67
				 1.05f, 1.1f, -0.25f,		0.0f,  1.0f,  0.0f,			1.0f, 1.0f, //68
				 1.05f, 1.1f, -0.25f,		0.0f,  1.0f,  0.0f,			1.0f, 1.0f, //69
				-0.05f, 1.0f, -0.25f,		0.0f,  1.0f,  0.0f,			0.0f, 0.0f, //70
				 1.05f, 1.0f, -0.25f,		0.0f,  1.0f,  0.0f,			1.0f, 0.0f, //71
				//BookShelf Bottom Inner Wall
				-0.05f, 0.0f, -0.25f, 	 	0.0f, -1.0f,  0.0f,			0.0f, 1.0f, //72
				-0.05f, 0.0f,  0.25f, 		0.0f, -1.0f,  0.0f,			0.0f, 0.0f, //73
				 1.05f, 0.0f, -0.25f, 		0.0f, -1.0f,  0.0f,			1.0f, 1.0f, //74
				 1.05f, 0.0f, -0.25f, 	 	0.0f, -1.0f,  0.0f,			1.0f, 1.0f, //75
				-0.05f, 0.0f,  0.25f, 		0.0f, -1.0f,  0.0f,			0.0f, 0.0f, //76
				 1.05f, 0.0f,  0.25f, 	 	0.0f, -1.0f,  0.0f,			1.0f, 0.0f, //77
				//BookShelf Bottom Outer Wall
				-0.05f, -0.1f, -0.25f, 	 	0.0f, -1.0f,  0.0f,			0.0f, 1.0f, //78
				-0.05f, -0.1f,  0.25f, 	 	0.0f, -1.0f,  0.0f,			0.0f, 0.0f, //79
				 1.05f, -0.1f, -0.25f, 	 	0.0f, -1.0f,  0.0f,			1.0f, 1.0f, //80
				 1.05f, -0.1f, -0.25f, 	 	0.0f, -1.0f,  0.0f,			1.0f, 1.0f, //81
				-0.05f, -0.1f,  0.25f, 	 	0.0f, -1.0f,  0.0f,			0.0f, 0.0f, //82
				 1.05f, -0.1f,  0.25f, 	 	0.0f, -1.0f,  0.0f,			1.0f, 0.0f, //83
				//BookShelf Bottom Face Wall
				-0.05f,  0.0f, 0.25f,		0.0f, -1.0f,  0.0f,			0.0f, 1.0f, //84
				-0.05f, -0.1f, 0.25f,		0.0f, -1.0f,  0.0f,			0.0f, 0.0f, //85
				 1.05f,  0.0f, 0.25f,		0.0f, -1.0f,  0.0f,			1.0f, 1.0f, //86
				 1.05f,  0.0f, 0.25f,		0.0f, -1.0f,  0.0f,			1.0f, 1.0f, //87
				-0.05f, -0.1f, 0.25f,		0.0f, -1.0f,  0.0f,			0.0f, 0.0f, //88
				 1.05f, -0.1f, 0.25f,		0.0f, -1.0f,  0.0f,			1.0f, 0.0f, //89
				//BookShelf Bottom Back Wall
				-0.05f,  0.0f, -0.25f,		0.0f, -1.0f,  0.0f,			0.0f, 1.0f, //90
				-0.05f, -0.1f, -0.25f,		0.0f, -1.0f,  0.0f,			0.0f, 0.0f, //91
				 1.05f,  0.0f, -0.25f,		0.0f, -1.0f,  0.0f,			1.0f, 1.0f, //92
				 1.05f,  0.0f, -0.25f,		0.0f, -1.0f,  0.0f,			1.0f, 1.0f, //93
				-0.05f, -0.1f, -0.25f,		0.0f, -1.0f,  0.0f,			0.0f, 0.0f, //94
				 1.05f, -0.1f, -0.25f,		0.0f, -1.0f,  0.0f,			1.0f, 0.0f, //95
				//BookShelf Center Shelf Bottom
				-0.05f, 0.45f, -0.25f,		0.0f, -1.0f,  0.0f,			0.0f, 1.0f, //96
				-0.05f, 0.45f,  0.25f,		0.0f, -1.0f,  0.0f,			0.0f, 0.0f, //97
				 1.05f, 0.45f, -0.25f,		0.0f, -1.0f,  0.0f,			1.0f, 1.0f, //98
				 1.05f, 0.45f, -0.25f,		0.0f, -1.0f,  0.0f,			1.0f, 1.0f, //99
				-0.05f, 0.45f,  0.25f,		0.0f, -1.0f,  0.0f,			0.0f, 0.0f, //100
				 1.05f, 0.45f,  0.25f,		0.0f, -1.0f,  0.0f,			0.0f, 1.0f, //101
				//BookShelf Center Shelf Top
				-0.05f, 0.55f, -0.25f,		0.0f, -1.0f,  0.0f,			0.0f, 1.0f, //102
				-0.05f, 0.55f,  0.25f,		0.0f, -1.0f,  0.0f,			0.0f, 0.0f, //103
				 1.05f, 0.55f, -0.25f,		0.0f, -1.0f,  0.0f,			1.0f, 1.0f, //104
				 1.05f, 0.55f, -0.25f,		0.0f, -1.0f,  0.0f,			1.0f, 1.0f, //105
				-0.05f, 0.55f,  0.25f,		0.0f, -1.0f,  0.0f,			0.0f, 0.0f, //106
				 1.05f, 0.55f,  0.25f,		0.0f, -1.0f,  0.0f,			0.0f, 1.0f, //107
				//BookShelf Center Face
				-0.05f, 0.55f, 0.25f,		0.0f, -1.0f, 0.0f,			0.0f, 1.0f, //108
				-0.05f, 0.45f, 0.25f,		0.0f, -1.0f, 0.0f,			0.0f, 0.0f, //109
				 1.05f, 0.55f, 0.25f,		0.0f, -1.0f, 0.0f,			1.0f, 1.0f, //110
				 1.05f, 0.55f, 0.25f,		0.0f, -1.0f, 0.0f,			1.0f, 1.0f, //111
				-0.05f, 0.45f, 0.25f,		0.0f, -1.0f, 0.0f,			0.0f, 0.0f, //112
				 1.05f, 0.45f, 0.25f,		0.0f, -1.0f, 0.0f,			1.0f, 0.0f, //113
				//BookShelf Center Back
				-0.05f, 0.55f, -0.25f,		0.0f, -1.0f, 0.0f,			0.0f, 1.0f, //114
				-0.05f, 0.45f, -0.25f,		0.0f, -1.0f, 0.0f,			0.0f, 0.0f, //115
				 1.05f, 0.55f, -0.25f,		0.0f, -1.0f, 0.0f,			1.0f, 1.0f, //116
				 1.05f, 0.55f, -0.25f,		0.0f, -1.0f, 0.0f,			1.0f, 1.0f, //117
				-0.05f, 0.45f, -0.25f,		0.0f, -1.0f, 0.0f,			0.0f, 0.0f, //118
				 1.05f, 0.45f, -0.25f,		0.0f, -1.0f, 0.0f,			1.0f, 0.0f, //119
				//Center Divider Left Wall
				 0.45f, 1.0f, -0.25f, 		1.0f,  0.0f,  0.0f,			1.0f, 1.0f, //120
				 0.45f, 1.0f,  0.25f, 		1.0f,  0.0f,  0.0f,			0.0f, 1.0f, //121
				 0.45f, 0.0f, -0.25f, 		1.0f,  0.0f,  0.0f,			0.0f, 0.0f, //122
				 0.45f, 0.0f, -0.25f, 		1.0f,  0.0f,  0.0f,			0.0f, 0.0f, //123
				 0.45f, 0.0f,  0.25f, 		1.0f,  0.0f,  0.0f,			1.0f, 0.0f, //124
				 0.45f, 1.0f,  0.25f, 		1.0f,  0.0f,  0.0f,			1.0f, 1.0f, //125
				//BookShelf Center Divider Right Wall
				 0.55f, 1.0f, -0.25f, 		1.0f,  0.0f,  0.0f,			1.0f, 1.0f, //126
				 0.55f, 1.0f,  0.25f, 		1.0f,  0.0f,  0.0f,			0.0f, 1.0f, //127
				 0.55f, 0.0f, -0.25f, 		1.0f,  0.0f,  0.0f,			0.0f, 0.0f, //128
				 0.55f, 0.0f, -0.25f, 		1.0f,  0.0f,  0.0f,			0.0f, 0.0f, //129
				 0.55f, 0.0f,  0.25f, 		1.0f,  0.0f,  0.0f,			1.0f, 0.0f, //130
				 0.55f, 1.0f,  0.25f, 		1.0f,  0.0f,  0.0f,			1.0f, 1.0f, //131
				//BookShelf Center Divider Face Wall
				 0.45f, 1.0f, 0.25f, 		1.0f,  0.0f,  0.0f,			1.0f, 1.0f, //132
				 0.45f, 0.0f, 0.25f, 		1.0f,  0.0f,  0.0f,			0.0f, 1.0f, //133
				 0.55f, 1.0f, 0.25f, 		1.0f,  0.0f,  0.0f,			0.0f, 0.0f, //134
				 0.55f, 1.0f, 0.25f, 		1.0f,  0.0f,  0.0f,			0.0f, 0.0f, //135
				 0.45f, 0.0f, 0.25f, 		1.0f,  0.0f,  0.0f,			1.0f, 0.0f, //136
				 0.55f, 0.0f, 0.25f, 		1.0f,  0.0f,  0.0f,			1.0f, 1.0f, //137
				//BookShelf Center Divider Back Wall
				 0.45f, 1.0f, -0.25f, 		1.0f,  0.0f,  0.0f,			1.0f, 1.0f, //138
				 0.45f, 0.0f, -0.25f, 		1.0f,  0.0f,  0.0f,			0.0f, 1.0f, //139
				 0.55f, 1.0f, -0.25f, 		1.0f,  0.0f,  0.0f,			0.0f, 0.0f, //140
				 0.55f, 1.0f, -0.25f, 		1.0f,  0.0f,  0.0f,			0.0f, 0.0f, //141
				 0.45f, 0.0f, -0.25f, 		1.0f,  0.0f,  0.0f,			1.0f, 0.0f, //142
				 0.55f, 0.0f, -0.25f, 		1.0f,  0.0f,  0.0f,			1.0f, 1.0f  //143
		};

	//Generate buffer ids
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);

	//Activate the VAO before binding and setting VBOs and VAPs
	glBindVertexArray(VAO);

	//Activate the VBO
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW); //Copy vertices to VBO

	//set attribute pointer 0 to hold position data
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid*)0);
	glEnableVertexAttribArray(0); //Enables vertex attribute

	//Set attribute pointer 1 to hold Normal data
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat)));
	glEnableVertexAttribArray(1);

	//Set attribute pointer 2 to hold Texture coordinate data
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid*)(6 * sizeof(GLfloat)));
	glEnableVertexAttribArray(2);

	glBindVertexArray(0); //Unbind the VAO
}

//Generate and load the texture
void UGenerateTexture(){
	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);

	int width, height;

	unsigned char* image = SOIL_load_image("BookcaseTexture.jpg", &width, &height, 0, SOIL_LOAD_RGB);//loads texture file

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
	glGenerateMipmap(GL_TEXTURE_2D);
	SOIL_free_image_data(image);
	glBindTexture(GL_TEXTURE_2D, 0); //Unbind the texture
}

//Implements the UMouseClick function
void UMouseClick(int button, int state, int x, int y)
{
	//sets the state for the alt key; true or false
	altDown = glutGetModifiers();

	//Sets the state for the mouse click
	if((button == GLUT_LEFT_BUTTON) && (state == GLUT_DOWN)){
		cout<<"Left Mouse Button Clicked!"<<endl; // for testing purposes
		leftMouseButton = true;
	}

	//Sets the state for the mouse click
	if((button == GLUT_LEFT_BUTTON) && (state == GLUT_UP)){
		cout<<"Left Mouse Button Released!"<<endl;// for testing purposes
		leftMouseButton = false;
	}

	//Sets the state for the mouse click
	if((button == GLUT_RIGHT_BUTTON) && (state == GLUT_DOWN)){
		cout<<"Right Mouse Button Clicked!"<<endl;// for testing purposes
		rightMouseButton = true;
	}

	//Sets the state for the mouse click
	if((button == GLUT_RIGHT_BUTTON) && (state == GLUT_UP)){
		cout<<"Right Mouse Button Released!"<<endl;// for testing purposes
		rightMouseButton = false;
	}

}
//Function to switch between projections.
void UKeyboardButton(unsigned char key, int x, int y){
	if (key == 'p'){
		projectionType = !projectionType;
	}
}
//Implements the UMouseMove function
void UMousePressedMove(int x, int y)
{
	//Immediately replaces center locked coordinates with new mouse coordinates
	if(mouseDetected)
	{
		lastMouseX = x;
		lastMouseY = y;
		mouseDetected = false;
	}

	//Gets the direction the mouse was moved in x and y
	mouseXOffset = x - lastMouseX;
	mouseYOffset = lastMouseY - y; //Inverted Y

	//Updates with new mouse coordinates
	lastMouseX = x;
	lastMouseY = y;

	//Applies sensitivity to mouse direction
	mouseXOffset *= sensitivity;
	mouseYOffset *= sensitivity;

	//Orbits around the center
	if(altDown == true) {
		//Accumulates the yaw and pitch variables
		if(leftMouseButton == true){
			// rotate around the model
			cameraRotateAmt.x += mouseXOffset;
			cameraRotateAmt.y += mouseYOffset;
		}

	if(rightMouseButton == true){
		// zoom feature
		zoom += mouseYOffset;
		}
	} else {
		yaw += mouseXOffset;
		pitch += mouseYOffset;
	}

}







