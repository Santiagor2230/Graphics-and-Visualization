/*Santiago Ramirez
06/18/2022
CS-330-H7601 Comp Graphic and Visualization */
#include <iostream>         // cout, cerr
#include <cstdlib>          // EXIT_FAILURE
#include <GL/glew.h>        // GLEW library
#include <GLFW/glfw3.h>     // GLFW library
#include <windows.h>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"    // Image loading Utility functions

#include "cylinder.h"


// GLM Math Header inclusions
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "camera.h" // Camera class

using namespace std; // Standard namespace

/*Shader program Macro*/
#ifndef GLSL
#define GLSL(Version, Source) "#version " #Version " core \n" #Source
#endif

// Unnamed namespace
namespace
{
	const char* const WINDOW_TITLE = "Santiago Ramirez"; // Macro for window title

	// Variables for window width and height
	const int WINDOW_WIDTH = 800;
	const int WINDOW_HEIGHT = 600;

	// Stores the GL data relative to a given mesh
	struct GLMesh
	{
		//vaos
		GLuint vao;         // Handle for the vertex array object
		GLuint vaoCube;		// Handle for the vertex array object 2
		GLuint vaoCylinder[4]; //handles cylinders vertex array object
		GLuint vaoPlane;    // handles vao vertex array
		GLuint vaoLight;    // handles light cube vertex array

		//vbos
		GLuint vbos;     // Handles for the vertex buffer objects
		GLuint vbosCube;     // Handles for the vertex buffer objects
		GLuint vbosCylinder[4]; // Handles for the vertex buffer objects
		GLuint vbosPlane;   // Handles vertex buffer for plane
		GLuint vbosLight;   // Handles light cube buffer array

		//indices
		GLuint nIndices;    // Number of indices of the Pyramid
		GLuint cubeIndices;  // Number of indices of the cube
		GLuint cylinderIndices[2]; // Number of indices of cylinders
		GLuint planeIndices; //Number of indices in the plane
		GLuint lightIndices;  // Number of inidces in the cube 
	};

	// Main GLFW window
	GLFWwindow* gWindow = nullptr;
	// Triangle mesh data
	GLMesh gMesh;
	// Texture id
	GLuint gTextureIdBox;
	GLuint gTextureIdMineral;
	GLuint gTextureIdTable;
	GLuint gTextureIdCoin;
	GLuint gTextureIdCoinWrap;
	GLuint gTextureIdRing;

	// Shader program
	GLuint gProgramId;
	GLuint gLampProgramId;

	// camera
	Camera gCamera(glm::vec3(0.0f, 2.0f, 10.0f));
	float gLastX = WINDOW_WIDTH / 2.0f;
	float gLastY = WINDOW_HEIGHT / 2.0f;
	bool gFirstMouse = true;

	// timing
	float gDeltaTime = 0.0f; // time between current frame and last frame
	float gLastFrame = 0.0f;

	//cube color
	glm::vec3 gObjectColor[] = {
		glm::vec3(1.0f, 1.0f, 1.0f),
		glm::vec3(1.0f, 1.0f, 1.0f)
	};

	//light color
	glm::vec3 gLightColor[] = {
		glm::vec3(1.0f, 1.0f, 1.0f),
		glm::vec3(1.0f, 1.0f, 1.0f)
	};

	//Light position and scale
	glm::vec3 pointLightPosition[] = {
		glm::vec3(0.0f, 0.5f, -2.5f),
		glm::vec3(0.0f, 0.5f,  1.5f)
	};
	//light scale
	glm::vec3 gLightScale[] = {
		glm::vec3(0.3f),
		glm::vec3(0.3f)
	};
};

/* User-defined Function prototypes to:
 * initialize the program, set the window size,
 * redraw graphics on the window when resized,
 * and render graphics on the screen
 */
bool UInitialize(int, char*[], GLFWwindow** window);
void UResizeWindow(GLFWwindow* window, int width, int height);
void UProcessInput(GLFWwindow* window);
void UMousePositionCallback(GLFWwindow* window, double xpos, double ypos);
void UMouseScrollCallback(GLFWwindow* window, double xoffset, double yoffset);
void UMouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
void UCreateMesh(GLMesh &mesh);
void UDestroyMesh(GLMesh &mesh);
bool UCreateTexture(const char* filename, GLuint &textureId);
void UDestroyTexture(GLuint textureId);
void URender();
bool UCreateShaderProgram(const char* vtxShaderSource, const char* fragShaderSource, GLuint &programId);
void UDestroyShaderProgram(GLuint programId);


/* Vertex Shader Source Code*/
const GLchar * vertexShaderSource = GLSL(440,
	layout(location = 0) in vec3 position; // VAP position 0 for vertex position data
	layout(location = 1) in vec3 normal; //VAP position 1 for normal 
	layout(location = 2) in vec2 textureCoordinate;

	out vec3 vertexFragmentPos; // for outgoing color / pixels to fragment
	out vec3 vertexNormal; // for outgoing normals to fragment shader
	out vec2 vertexTextureCoordinate;

	//Global variables for the  transform matrices
	uniform mat4 model;
	uniform mat4 view;
	uniform mat4 projection;


void main()
{
	vertexFragmentPos = vec3(model * vec4(position, 1.0f)); // Gets fragment / pixel position in world space only ( exclude view and projection)

	vertexNormal = mat3(transpose(inverse(model))) * normal; // get normal vectors in wordl space only and excluded normal translation properties
	vertexTextureCoordinate = textureCoordinate;

	gl_Position = projection * view * model * vec4(position, 1.0f); //transforms vertices into clip cordinates

}
);


/* Fragment Shader Source Code*/
const GLchar * fragmentShaderSource = GLSL(440,

	in vec3 vertexNormal; //for incoming normals
	in vec3 vertexFragmentPos; //for incoming fragment position
	in vec2 vertexTextureCoordinate;

	out vec4 fragmentColor;

	//uniform / Global varaibles for object
	uniform vec3 objectColor;
	uniform vec3 lightColor;
	uniform vec3 lightPos;
	uniform vec3 keyLightColor;
	uniform vec3 keyLightPos;
	uniform vec3 viewPosition;
	uniform vec3 keyViewPosition;
	uniform sampler2D uTexture;

	void main()
	{

		//FIRST LIGHT SHADER
		//Calculate Ambient lighting
		float ambientStrength = 0.5f; // Set ambient or global lighting
		vec3 ambient = ambientStrength * lightColor; // Generate ambient light color

		//Calculate Diffuse lighting
		vec3 norm = normalize(vertexNormal); // Normalize vectors to 1 unit
		vec3 lightDirection = normalize(lightPos - vertexFragmentPos); // Calculate distance (light direction) between light source and fragments/pixels on cube
		float impact = max(dot(norm, lightDirection), 0.0);// Calculate diffuse impact by generating dot product of normal and light
		vec3 diffuse = impact * lightColor; // Generate diffuse light color

		//Calculate Specular lighting
		float specularIntensity = 0.0f; // Set specular light strength
		float highlightSize = 0.0f; // Set specular highlight size
		vec3 viewDir = normalize(viewPosition - vertexFragmentPos); // Calculate view direction
		vec3 reflectDir = reflect(-lightDirection, norm);// Calculate reflection vector
		//Calculate specular component
		float specularComponent = pow(max(dot(viewDir, reflectDir), 0.0), highlightSize);
		vec3 specular = specularIntensity * specularComponent * lightColor;


		//SECOND LIGHT SHADER
		//Calculate Key Ambient lighting
		float keyAmbientStrength = 0.2f; // Set ambient or global lighting
		vec3 keyAmbient = keyAmbientStrength * keyLightColor;; // Generate ambient light color

		//Calculate Key Diffuse lighting
		vec3 keyNorm = normalize(vertexNormal); // Normalize vectors to 1 unit
		vec3 keyLightDirection = normalize(keyLightPos - vertexFragmentPos); // Calculate distance (light direction) between light source and fragments/pixels on cube
		float keyImpact = max(dot(keyNorm, keyLightDirection), 0.0f);// Calculate diffuse impact by generating dot product of normal and light
		vec3 keyDiffuse = keyImpact * keyLightColor;; // Generate diffuse light color

		//Calculate Key Specular lighting
		float keySpecularIntensity = 0.7; // Set specular light strength
		float keyHighlightSize = 150.0f; // Set specular highlight size
		vec3 keyViewDir = normalize(keyViewPosition - vertexFragmentPos); // Calculate view direction
		vec3 keyReflectDir = reflect(-keyLightDirection, keyNorm);// Calculate reflection vector

		//Calculate key specular component
		float keySpecularComponent = pow(max(dot(keyViewDir, keyReflectDir), 0.0f), keyHighlightSize);
		vec3 keySpecular = keySpecularIntensity * keySpecularComponent * keyLightColor;;

		// Texture holds the color to be used for all three components
		vec3 textureColor = texture(uTexture, vertexTextureCoordinate).xyz;
		vec3 fillResult = (ambient + diffuse + specular);
		vec3 keyResult = (keyAmbient + keyDiffuse + keySpecular);
		// Calculate phong result
		vec3 result = fillResult + keyResult;
		vec3 phong = (result)* textureColor.xyz;

		fragmentColor = vec4(phong, 1.0); // Send lighting results to GPU
	}
);

/* Lamp Shader Source Code*/
const GLchar * lampVertexShaderSource = GLSL(440,

	layout(location = 0) in vec3 position; // VAP position 0 for vertex position data

		//Uniform / Global variables for the  transform matrices
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
	gl_Position = projection * view * model * vec4(position, 1.0f); // Transforms vertices into clip coordinates
}
);

/* Fragment Shader Source Code*/
const GLchar * lampFragmentShaderSource = GLSL(440,

	out vec4 fragmentColor; // For outgoing lamp color (smaller cube) to the GPU

void main()
{
	fragmentColor = vec4(1.0f); // Set color to white (1.0f,1.0f,1.0f) with alpha 1.0
}
);

// Images are loaded with Y axis going down, but OpenGL's Y axis goes up, so let's flip it
void flipImageVertically(unsigned char *image, int width, int height, int channels)
{
	for (int j = 0; j < height / 2; ++j)
	{
		int index1 = j * width * channels;
		int index2 = (height - 1 - j) * width * channels;

		for (int i = width * channels; i > 0; --i)
		{
			unsigned char tmp = image[index1];
			image[index1] = image[index2];
			image[index2] = tmp;
			++index1;
			++index2;
		}
	}
}


int main(int argc, char* argv[])
{
	if (!UInitialize(argc, argv, &gWindow))
		return EXIT_FAILURE;

	// Create the mesh
	UCreateMesh(gMesh); // Calls the function to create the Vertex Buffer Object

	// Create the shader program
	if (!UCreateShaderProgram(vertexShaderSource, fragmentShaderSource, gProgramId))
		return EXIT_FAILURE;

	// Create the shader program
	if (!UCreateShaderProgram(lampVertexShaderSource, lampFragmentShaderSource, gLampProgramId))
		return EXIT_FAILURE;


	// Load textures
	const char * texFilename = "Unobox.png";
	if (!UCreateTexture(texFilename, gTextureIdBox))
	{
		cout << "Failed to load texture " << texFilename << endl;
		return EXIT_FAILURE;
	}
	texFilename = "mineral.jpg";
	if (!UCreateTexture(texFilename, gTextureIdMineral))
	{
		cout << "Failed to load texture " << texFilename << endl;
		return EXIT_FAILURE;
	}
	texFilename = "table.jpg";
	if (!UCreateTexture(texFilename, gTextureIdTable))
	{
		cout << "Failed to load texture " << texFilename << endl;
		return EXIT_FAILURE;
	}

	texFilename = "gray.jpg";
	if (!UCreateTexture(texFilename, gTextureIdCoin))
	{
		cout << "Failed to load texture " << texFilename << endl;
		return EXIT_FAILURE;
	}

	texFilename = "coinwrap.jpg";
	if (!UCreateTexture(texFilename, gTextureIdCoinWrap))
	{
		cout << "Failed to load texture " << texFilename << endl;
		return EXIT_FAILURE;
	}

	texFilename = "rings.png";
	if (!UCreateTexture(texFilename, gTextureIdRing))
	{
		cout << "Failed to load texture " << texFilename << endl;
		return EXIT_FAILURE;
	}
	// tell opengl for each sampler to which texture unit it belongs to (only has to be done once)
	glUseProgram(gProgramId);
	// We set the texture as texture unit 0
	glUniform1i(glGetUniformLocation(gProgramId, "uTexture"), 0);
	
	// Sets the background color of the window to black (it will be implicitely used by glClear)
	glClearColor(0.0f, 0.0f, 0.5f, 1.0f);

	// render loop
	// -----------
	while (!glfwWindowShouldClose(gWindow))
	{
		// per-frame timing
		// --------------------
		float currentFrame = glfwGetTime();
		gDeltaTime = currentFrame - gLastFrame;
		gLastFrame = currentFrame;

		// input
		// -----
		UProcessInput(gWindow);

		// Render this frame
		URender();

		glfwPollEvents();
	}

	// Release mesh data
	UDestroyMesh(gMesh);

	// Release texture
	UDestroyTexture(gTextureIdBox);
	UDestroyTexture(gTextureIdMineral);
	UDestroyTexture(gTextureIdTable);
	UDestroyTexture(gTextureIdCoin);
	UDestroyTexture(gTextureIdCoinWrap);
	UDestroyTexture(gTextureIdRing);
	// Release shader program
	UDestroyShaderProgram(gProgramId);
	UDestroyShaderProgram(gLampProgramId);

	exit(EXIT_SUCCESS); // Terminates the program successfully
}


// Initialize GLFW, GLEW, and create a window
bool UInitialize(int argc, char* argv[], GLFWwindow** window)
{
	// GLFW: initialize and configure
	// ------------------------------
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 4);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

	// GLFW: window creation
	// ---------------------
	*window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, WINDOW_TITLE, NULL, NULL);
	if (*window == NULL)
	{
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return false;
	}
	glfwMakeContextCurrent(*window);
	glfwSetFramebufferSizeCallback(*window, UResizeWindow);
	glfwSetCursorPosCallback(*window, UMousePositionCallback);
	glfwSetScrollCallback(*window, UMouseScrollCallback);
	glfwSetMouseButtonCallback(*window, UMouseButtonCallback);

	// tell GLFW to capture our mouse
	glfwSetInputMode(*window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	// GLEW: initialize
	// ----------------
	// Note: if using GLEW version 1.13 or earlier
	glewExperimental = GL_TRUE;
	GLenum GlewInitResult = glewInit();

	if (GLEW_OK != GlewInitResult)
	{
		std::cerr << glewGetErrorString(GlewInitResult) << std::endl;
		return false;
	}

	// Displays GPU OpenGL version
	cout << "INFO: OpenGL Version: " << glGetString(GL_VERSION) << endl;

	return true;
}


// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
void UProcessInput(GLFWwindow* window)
{
	static const float cameraSpeed = 2.5f;
	int i = 0;

	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) //key to escape the code for running
		glfwSetWindowShouldClose(window, true);

	if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) // key to go upwards on the y axis
		gCamera.ProcessKeyboard(UPWARD, gDeltaTime);

	if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) // key to go downward on the y axis
		gCamera.ProcessKeyboard(DOWNWARD, gDeltaTime);

	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) { // key to go foward in the z axis
		gCamera.ProcessKeyboard(FORWARD, gDeltaTime);
		if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) { // key shift to go forward quicker while pressing the key w in the z axis
			gCamera.ProcessKeyboard(SUPERFORWARD, gDeltaTime);
		}
	}

	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {// key to go backwards in the z axis
		gCamera.ProcessKeyboard(BACKWARD, gDeltaTime);
		if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) {// key shift to go backwards quicker while pressing the key s in the z axis
			gCamera.ProcessKeyboard(SUPERBACKWARD, gDeltaTime);
		}
	}

	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) // key to allow go left in the x axis
		gCamera.ProcessKeyboard(LEFT, gDeltaTime);


	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) // key to allo go right in the x axis
		gCamera.ProcessKeyboard(RIGHT, gDeltaTime);

	if (glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS) { // key allow change in projection from 3d to 2d
		gCamera.ProcessKeyboard(PROJECTION, gDeltaTime);
		Sleep(150); // delays the p key in order to  decrease input-smoother transitioning
	}

	//FIRST LIGHT
	if (glfwGetKey(window, GLFW_KEY_I) == GLFW_PRESS) { //key to up y axis
		pointLightPosition[0].y += 0.1f * 0.1f;
	}

	if (glfwGetKey(window, GLFW_KEY_K) == GLFW_PRESS) { //key to go down y axis
		pointLightPosition[0].y -= 0.1f *0.1f;
	}

	if (glfwGetKey(window, GLFW_KEY_L) == GLFW_PRESS) { //key to go right x axis
		pointLightPosition[0].x += 0.1f * 0.1f;
	}

	if (glfwGetKey(window, GLFW_KEY_J) == GLFW_PRESS) { //key to go left x axis
		pointLightPosition[0].x -= 0.1f * 0.1f;
	}

	if (glfwGetKey(window, GLFW_KEY_U) == GLFW_PRESS) { //key to go front z axis
		pointLightPosition[0].z += 0.1f * 0.1f;
	}

	if (glfwGetKey(window, GLFW_KEY_O) == GLFW_PRESS) { //key to go back z axis
		pointLightPosition[0].z -= 0.1f * 0.1f;
	}

	//SECOND LIGHT
	if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) { //key to up y axis
		pointLightPosition[1].y += 0.1f * 0.1f;
	}

	if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) { //key to go down y axis
		pointLightPosition[1].y -= 0.1f *0.1f;
	}

	if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) { //key to go right x axis
		pointLightPosition[1].x += 0.1f * 0.1f;
	}

	if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) { //key to go left x axis
		pointLightPosition[1].x -= 0.1f * 0.1f;
	}

	if (glfwGetKey(window, GLFW_KEY_RIGHT_CONTROL) == GLFW_PRESS) { //key to go front z axis
		pointLightPosition[1].z += 0.1f * 0.1f;
	}

	if (glfwGetKey(window, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS) { //key to go back z axis
		pointLightPosition[1].z -= 0.1f * 0.1f;
	}
	
}


// glfw: whenever the window size changed (by OS or user resize) this callback function executes
void UResizeWindow(GLFWwindow* window, int width, int height)
{
	glViewport(0, 0, width, height);

}

// glfw: whenever the mouse moves, this callback is called
void UMousePositionCallback(GLFWwindow* window, double xpos, double ypos)
{
	if (gFirstMouse)
	{
		gLastX = xpos;
		gLastY = ypos;
		gFirstMouse = false;
	}

	float xoffset = xpos - gLastX;
	float yoffset = gLastY - ypos; // reversed since y-coordinates go from bottom to top

	gLastX = xpos;
	gLastY = ypos;

	gCamera.ProcessMouseMovement(xoffset, yoffset);
}

// glfw: whenever the mouse scroll wheel scrolls, this callback is called
void UMouseScrollCallback(GLFWwindow* window, double xoffset, double yoffset)
{
	gCamera.ProcessMouseScroll(yoffset);
}

// glfw: handle mouse button events
void UMouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
	switch (button)
	{
	case GLFW_MOUSE_BUTTON_LEFT:
	{
		if (action == GLFW_PRESS)
			cout << "Left mouse button pressed" << endl;
		else
			cout << "Left mouse button released" << endl;
	}
	break;

	case GLFW_MOUSE_BUTTON_MIDDLE:
	{
		if (action == GLFW_PRESS)
			cout << "Middle mouse button pressed" << endl;
		else
			cout << "Middle mouse button released" << endl;
	}
	break;

	case GLFW_MOUSE_BUTTON_RIGHT:
	{
		if (action == GLFW_PRESS)
			cout << "Right mouse button pressed" << endl;
		else
			cout << "Right mouse button released" << endl;
	}
	break;

	default:
		cout << "Unhandled mouse button event" << endl;
		break;
	}
}



// Functioned called to render a frame
void URender()
{
	
	// Enable z-depth
	glEnable(GL_DEPTH_TEST);

	// Clear the frame and z buffers
	glClearColor(0.0f, 0.0f, 0.15f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// camera/view transformation
	glm::mat4 view = gCamera.GetViewMatrix();

	// Creates a projection with class camera
	glm::mat4 projection = gCamera.GetProjection();
	
	// Transformations are applied right-to-left order
	//glm::mat4 transformation = translation * rotation * scale;
	glm::mat4 transformation(1.0f);

	// Sends transform information to the Vertex shader
	GLuint transformLocation = glGetUniformLocation(gProgramId, "shaderTransform");
	glUniformMatrix4fv(transformLocation, 1, GL_FALSE, glm::value_ptr(transformation));


	// Set the shader to be used
	glUseProgram(gProgramId);

	//LAMP 1 SHADERS
	// Reference matrix uniforms from the Cube Shader program for the cub color, light color, light position, and camera position
	GLint objectColorLoc = glGetUniformLocation(gProgramId, "objectColor");
	GLint lightColorLoc = glGetUniformLocation(gProgramId, "lightColor");
	GLint lightPositionLoc = glGetUniformLocation(gProgramId, "lightPos");
	GLint viewPositionLoc = glGetUniformLocation(gProgramId, "viewPosition");

	// Pass color, light, and camera data to the Cube Shader program's corresponding uniforms
	glUniform3f(objectColorLoc, gObjectColor[0].r, gObjectColor[0].g, gObjectColor[0].b);
	glUniform3f(lightColorLoc, gLightColor[0].r, gLightColor[0].g, gLightColor[0].b);
	glUniform3f(lightPositionLoc, pointLightPosition[0].x, pointLightPosition[0].y, pointLightPosition[0].z);
	const glm::vec3 cameraPosition = gCamera.Position;
	glUniform3f(viewPositionLoc, cameraPosition.x, cameraPosition.y, cameraPosition.z);

	//LAMP 2 SHADERS
	GLint objectColorLoc2 = glGetUniformLocation(gProgramId, "keyObjectColor");
	GLint lightColorLoc2 = glGetUniformLocation(gProgramId, "keyLightColor");
	GLint lightPositionLoc2 = glGetUniformLocation(gProgramId, "keyLightPos");
	GLint viewPositionLoc2 = glGetUniformLocation(gProgramId, "keyViewPosition");

	// Pass color, light, and camera data to the Cube Shader program's corresponding uniforms
	glUniform3f(objectColorLoc2, gObjectColor[1].r, gObjectColor[1].g, gObjectColor[1].b);
	glUniform3f(lightColorLoc2, gLightColor[1].r, gLightColor[1].g, gLightColor[1].b);
	glUniform3f(lightPositionLoc2, pointLightPosition[1].x, pointLightPosition[1].y, pointLightPosition[1].z);
	glUniform3f(viewPositionLoc2, cameraPosition.x, cameraPosition.y, cameraPosition.z);


	GLint modelLoc = glGetUniformLocation(gProgramId, "model");
	GLint viewLoc = glGetUniformLocation(gProgramId, "view");
	GLint projLoc = glGetUniformLocation(gProgramId, "projection");

	glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
	glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

	// 1. Scales the object by 2
	glm::mat4 scale = glm::scale(glm::vec3(2.0f, 2.0f, 2.0f));
	// 2. Rotates shape by 15 degrees in the x axis
	glm::mat4 rotation = glm::rotate(glm::radians(0.0f), glm::vec3(1.0, 1.0f, 0.0f));

	glm::vec3 location = glm::vec3(0.0f, 0.0f, 0.0f);
	// 3. Place object at the origin
	glm::mat4 translation = glm::translate(location);
	// Model matrix: transformations are applied right-to-left order
	glm::mat4 model = translation * rotation * scale;
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

	// Retrieves and passes transform matrices to the Shader program\
	//PLANE
	// Activate the VBOs contained within the mesh's VAO
	glBindVertexArray(gMesh.vaoPlane);

	// bind textures on corresponding texture units
	glBindTexture(GL_TEXTURE_2D, gTextureIdTable);
	glDrawArrays(GL_TRIANGLES, 0, gMesh.planeIndices); // Draws the triangle for Plane


	//CUBE
	// Activate the VBOs contained within the mesh's VAO
	glBindVertexArray(gMesh.vaoCube);

	// bind textures on corresponding texture units
	glBindTexture(GL_TEXTURE_2D, gTextureIdBox);
	glDrawArrays(GL_TRIANGLES, 0, gMesh.cubeIndices); // Draws the Triangle for cube



	//PYRAMID
	// Activate the VBOs contained within the mesh's VAO
	glBindVertexArray(gMesh.vao);
	
	scale = glm::scale(glm::vec3(1.5f));
	rotation = glm::rotate(glm::radians(60.0f), glm::vec3(0.0, 1.0f, 0.0f));
	location = glm::vec3(0.3f, -0.2f, 0.8f);
	translation = glm::translate(location);
	model = translation * rotation * scale;
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));



	// bind textures on corresponding texture units
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, gTextureIdMineral);
	glDrawArrays(GL_TRIANGLES, 0, gMesh.nIndices); // Draws the triangle for pyramid
	



	//Money CYLINDER SHAPE
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, gTextureIdCoinWrap);
	glBindVertexArray(gMesh.vaoCylinder[0]);
	scale = glm::scale(glm::vec3(1.5f));
	rotation = glm::rotate(glm::radians(90.0f), glm::vec3(1.0, 0.0f, 0.0f));
	location = glm::vec3(0.6f, -0.147f, 0.0f);
	translation = glm::translate(location);
	model = translation * rotation * scale;
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
	static_meshes_3D::Cylinder C1(0.04, 20, 0.3, true, true, true);
	C1.render();

	//Money Inner CYLINDER SHAPE
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, gTextureIdCoin);
	glBindVertexArray(gMesh.vaoCylinder[1]);
	scale = glm::scale(glm::vec3(1.5f));
	rotation = glm::rotate(glm::radians(90.0f), glm::vec3(1.0, 0.0f, 0.0f));
	location = glm::vec3(0.6f, -0.147f, 0.0f);
	translation = glm::translate(location);
	model = translation * rotation * scale;
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
	static_meshes_3D::Cylinder C2(0.03, 20, 0.301, true, true, true);
	C2.render();

	//Ring CYLINDER SHAPE
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, gTextureIdRing);
	glBindVertexArray(gMesh.vaoCylinder[2]);
	scale = glm::scale(glm::vec3(3.0f));
	rotation = glm::rotate(glm::radians(90.0f), glm::vec3(1.0, 0.5f, 0.0f));
	location = glm::vec3(0.6f, -0.1f, 0.4f);
	translation = glm::translate(location);
	model = translation * rotation * scale;
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
	static_meshes_3D::Cylinder C3(0.03, 20, 0.010, true, true, true);
	C3.render();

	//Inner Ring CYLINDER SHAPE
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, gTextureIdTable);
	glBindVertexArray(gMesh.vaoCylinder[3]);
	scale = glm::scale(glm::vec3(3.0f));
	rotation = glm::rotate(glm::radians(90.0f), glm::vec3(1.0, 0.5f, 0.0f));
	location = glm::vec3(0.6f, -0.1f, 0.4f);
	translation = glm::translate(location);
	model = translation * rotation * scale;
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
	static_meshes_3D::Cylinder C4(0.026, 20, 0.011, true, false, true);
	C4.render();





	// LAMP: FIRST LAMP BUILD
	//---------------------------------------------------------------------------------------------------
	//Light cube vaos
	glBindVertexArray(gMesh.vaoLight);


	//LAMP POSITIONING SHADER
	glUseProgram(gLampProgramId);

	//Transforms the Vao2 Cube in order for it to be a lamp
	model = glm::translate(pointLightPosition[0]) * glm::scale(gLightScale[0]);

	// Reference matrix uniforms from the Lamp Shader program
	modelLoc = glGetUniformLocation(gLampProgramId, "model");
	viewLoc = glGetUniformLocation(gLampProgramId, "view");
	projLoc = glGetUniformLocation(gLampProgramId, "projection");

	// Pass matrix data to the Lamp Shader program's matrix uniforms
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
	glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
	glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));


	glDrawArrays(GL_TRIANGLES, 0, gMesh.lightIndices);


	//SECOND LAMP BUILD
	//----------------------------------------------------------------------------------


	//Transform the VAO2 object into a light
	model = glm::translate(pointLightPosition[1]) * glm::scale(gLightScale[1]);

	//LAMP TWO POSITIONING SHADER

	// Reference matrix uniforms from the Lamp Shader program
	modelLoc = glGetUniformLocation(gLampProgramId, "model");
	viewLoc = glGetUniformLocation(gLampProgramId, "view");
	projLoc = glGetUniformLocation(gLampProgramId, "projection");

	// Pass matrix data to the Lamp Shader program's matrix uniforms
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
	glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
	glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));


	glDrawArrays(GL_TRIANGLES, 0, gMesh.lightIndices);
	

	// Deactivate the Vertex Array Object
	glBindVertexArray(0);

	// glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
	glfwSwapBuffers(gWindow);    // Flips the the back buffer with the front buffer every frame.


}



// Implements the UCreateMesh function
void UCreateMesh(GLMesh &mesh)
{

	GLfloat verts[] = {
		    // Vertex Positions    // Colors (r,g,b,a)
			//PYRAMID
			//front
			0.0f,  0.2f,  0.0f,    0.0f, 0.0f, 1.0f,    0.5f,  1.0f, 
			0.1f,  0.0f,  0.1f,    0.0f, 0.0f, 1.0f,   1.0f,  0.0f,
		   -0.1f,  0.0f,  0.1f,    0.0f, 0.0f, 1.0f,    0.0f,  0.0f,

		   //back
		   -0.1f,  0.0f, -0.1f,    0.0f, 0.0f, -1.0f,   0.0f,  0.0f,
			0.1f,  0.0f, -0.1f,    0.0f, 0.0f, -1.0f,   1.0f,  0.0f,
		    0.0f,  0.2f,  0.0f,    0.0f, 0.0f, -1.0f,   0.5f,  1.0f,
			
		   //right
			0.1f,  0.0f,  0.1f,    1.0f, 0.0f, 0.0f,   0.0f,  0.0f,
			0.1f,  0.0f, -0.1f,    1.0f, 0.0f, 0.0f,   1.0f,  0.0f, 
			0.0f,  0.2f,  0.0f,    1.0f, 0.0f, 0.0f,   0.5f,  1.0f,
			
			//left
		   -0.1f,  0.0f,  0.1f,   -1.0f, 0.0f, 0.0f,   1.0f,  0.0f,
		   -0.1f,  0.0f, -0.1f,   -1.0f, 0.0f, 0.0f,   0.0f,  0.0f,
			0.0f,  0.2f,  0.0f,   -1.0f, 0.0f, 0.0f,  0.5f,  1.0f,

			//bottom
			0.1f,  0.0f,  0.1f,    0.0f, -1.0f, 0.0f,  1.0f, 1.0f,
		   -0.1f,  0.0f,  0.1f,    0.0f, -1.0f, 0.0f,  0.0f, 1.0f,
			0.1f,  0.0f, -0.1f,    0.0f, -1.0f, 0.0f,  1.0f, 0.0f, 
			0.1f,  0.0f, -0.1f,    0.0f, -1.0f, 0.0f,  1.0f, 0.0f,
		   -0.1f,  0.0f, -0.1f,    0.0f, -1.0f, 0.0f,  0.0f, 0.0f,
		   -0.1f,  0.0f,  0.1f,    0.0f, -1.0f, 0.0f,  0.0f, 1.0f
	};

	GLfloat vertices1[] = {

			//CUBE
			//back
			-0.2f, -0.1f, -0.4f,  0.0f, 0.0f, -1.0f,   0.0f, 0.7f,
			 0.2f, -0.1f, -0.4f,  0.0f, 0.0f, -1.0f,   0.1f, 0.7f,
			 0.2f,  0.0f, -0.4f,  0.0f, 0.0f, -1.0f,   0.1f, 0.8f,

			 0.2f,  0.0f, -0.4f,  0.0f, 0.0f, -1.0f,   0.1f, 0.8f,
			-0.2f,  0.0f, -0.4f,  0.0f, 0.0f, -1.0f,   0.0f, 0.8f,
			-0.2f, -0.1f, -0.4f,  0.0f, 0.0f, -1.0f,   0.0f, 0.7f,

			//front
			-0.2f, -0.1f,  0.2f,  0.0f, 0.0f, 1.0f,    0.0f, 0.7f,
			 0.2f, -0.1f,  0.2f,  0.0f, 0.0f, 1.0f,    0.1f, 0.7f,
			 0.2f,  0.0f,  0.2f,  0.0f, 0.0f, 1.0f,    0.1f, 0.8f,

			 0.2f,  0.0f,  0.2f,  0.0f, 0.0f,  1.0f,   0.1f, 0.8f,
			-0.2f,  0.0f,  0.2f,  0.0f, 0.0f,  1.0f,   0.0f, 0.8f,
			-0.2f, -0.1f,  0.2f,  0.0f, 0.0f,  1.0f,   0.0f, 0.7f,

			//left
			-0.2f,  0.0f,  0.2f,  -1.0f, 0.0f, 0.0f,   1.0f, 0.0f,
			-0.2f,  0.0f, -0.4f,  -1.0f, 0.0f, 0.0f,   1.0f, 0.8f,
			-0.2f, -0.1f, -0.4f,  -1.0f, 0.0f, 0.0f,   0.0f, 0.8f,

			-0.2f, -0.1f, -0.4f,  -1.0f, 0.0f, 0.0f,   0.0f, 0.8f,
			-0.2f, -0.1f,  0.2f,  -1.0f, 0.0f, 0.0f,   0.0f, 0.0f,
			-0.2f,  0.0f,  0.2f,  -1.0f, 0.0f, 0.0f,   1.0f, 0.0f,

			//right
			 0.2f,  0.0f,  0.2f,  1.0f, 0.0f, 0.0f,    1.0f, 0.0f,
			 0.2f,  0.0f, -0.4f,  1.0f, 0.0f, 0.0f,    1.0f, 0.8f,
			 0.2f, -0.1f, -0.4f,  1.0f, 0.0f, 0.0f,    0.0f, 0.8f,

			 0.2f, -0.1f, -0.4f,  1.0f, 0.0f, 0.0f,   0.0f, 0.8f,
			 0.2f, -0.1f,  0.2f,  1.0f, 0.0f, 0.0f,   0.0f, 0.0f,
			 0.2f,  0.0f,  0.2f,  1.0f, 0.0f, 0.0f,   1.0f, 0.0f,

			 //bottom
			-0.2f, -0.1f, -0.4f,  0.0f, -1.0f, 0.0f,  0.0f, 0.8f,
			 0.2f, -0.1f, -0.4f,  0.0f, -1.0f, 0.0f,  1.0f, 0.8f,
			 0.2f, -0.1f,  0.2f,  0.0f, -1.0f, 0.0f,  1.0f, 0.0f,
			 0.2f, -0.1f,  0.2f,  0.0f, -1.0f, 0.0f,  1.0f, 0.0f,
			-0.2f, -0.1f,  0.2f,  0.0f, -1.0f, 0.0f,  0.0f, 0.0f,
			-0.2f, -0.1f, -0.4f,  0.0f, -1.0f, 0.0f,  0.0f, 0.8f,

			//Top
			-0.2f,  0.0f, -0.4f,  0.0f,  1.0f, 0.0f,  0.0f, 0.8f,
			 0.2f,  0.0f, -0.4f,  0.0f,  1.0f, 0.0f,  1.0f, 0.8f,
			 0.2f,  0.0f,  0.2f,  0.0f,  1.0f, 0.0f,  1.0f, 0.0f,
			 0.2f,  0.0f,  0.2f,  0.0f,  1.0f, 0.0f,  1.0f, 0.0f,
			-0.2f,  0.0f,  0.2f,  0.0f,  1.0f, 0.0f,  0.0f, 0.0f,
			-0.2f,  0.0f, -0.4f,  0.0f,  1.0f, 0.0f,  0.0f, 0.8f,


		 ////CUBE TOP LID
	//back
	   -0.2f, -0.1f,    -0.55f,  0.0f, 0.0f, -1.0f,     0.0f, 0.7f,
		0.2f, -0.1f,    -0.55f,  0.0f, 0.0f, -1.0f,     0.1f, 0.7f,
		0.2f, -0.08f,   -0.55f,  0.0f, 0.0f, -1.0f,     0.1f, 0.8f,

	    0.2f, -0.08f,   -0.55f,  0.0f, 0.0f, -1.0f,     0.1f, 0.8f,
	   -0.2f, -0.08f,   -0.55f,  0.0f, 0.0f, -1.0f,     0.0f, 0.8f,
	   -0.2f, -0.1f,    -0.55f,  0.0f, 0.0f, -1.0f,     0.0f, 0.7f,

	   //front
	   -0.2f, -0.1f,    -0.4f,  0.0f, 0.0f,  1.0f,      0.0f, 0.7f,
	    0.2f, -0.1f,    -0.4f,  0.0f, 0.0f,  1.0f,      0.1f, 0.7f,
	    0.2f, -0.08f,   -0.4f,  0.0f, 0.0f,  1.0f,     0.1f, 0.8f,

	    0.2f,  -0.08f,  -0.4f,  0.0f, 0.0f,  1.0f,     0.1f, 0.8f,
	   -0.2f,  -0.08f,  -0.4f,  0.0f, 0.0f,  1.0f,     0.0f, 0.8f,
	   -0.2f,  -0.1f,   -0.4f,  0.0f, 0.0f,  1.0f,     0.0f, 0.7f,

	   //left
	   -0.2f,  -0.08f,  -0.4f,   -1.0f, 0.0f, 0.0f,    0.1f, 0.7f,
	   -0.2f,  -0.08f,  -0.55f,  -1.0f, 0.0f, 0.0f,    0.1f, 0.8f,
	   -0.2f,  -0.1f,   -0.55f,  -1.0f, 0.0f, 0.0f,    0.0f, 0.8f,

	   -0.2f, -0.1f,    -0.55f,  -1.0f, 0.0f, 0.0f,    0.0f, 0.8f,
	   -0.2f, -0.1f,    -0.4f,   -1.0f, 0.0f, 0.0f,    0.0f, 0.7f,
	   -0.2f, -0.08f,   -0.4f,   -1.0f, 0.0f, 0.0f,    0.1f, 0.7f,

	   //right
		0.2f,  -0.08f,  -0.4f,   1.0f, 0.0f, 0.0f,    0.1f, 0.7f,
		0.2f,  -0.08f,  -0.55f,  1.0f, 0.0f, 0.0f,    0.1f, 0.8f,
		0.2f,  -0.1f,   -0.55f,  1.0f, 0.0f, 0.0f,    0.0f, 0.8f,

		0.2f,  -0.1f,   -0.55f,  1.0f, 0.0f, 0.0f,     0.0f, 0.8f,
		0.2f,  -0.1f,   -0.4f,   1.0f, 0.0f, 0.0f,     0.0f, 0.7f,
		0.2f,  -0.08f,  -0.4f,   1.0f, 0.0f, 0.0f,     0.1f, 0.7f,
		
		//Bottom
	   -0.2f, -0.1f,    -0.55f,  0.0f, -1.0f, 0.0f,    0.0f, 1.0f,
		0.2f, -0.1f,    -0.55f,  0.0f, -1.0f, 0.0f,    1.0f, 1.0f,
		0.2f, -0.1f,    -0.4f,   0.0f, -1.0f, 0.0f,    1.0f, 0.8f,
		0.2f, -0.1f,    -0.4f,   0.0f, -1.0f, 0.0f,    1.0f, 0.8f,
	   -0.2f, -0.1f,    -0.4f,   0.0f, -1.0f, 0.0f,    0.0f, 0.8f,
	   -0.2f, -0.1f,    -0.55f,  0.0f, -1.0f, 0.0f,    0.0f, 1.0f,

		//Top
	   -0.2f,  -0.08f,  -0.55f,  0.0f, 1.0f, 0.0f,     0.0f, 1.0f,
		0.2f,  -0.08f,  -0.55f,	 0.0f, 1.0f, 0.0f,     1.0f, 1.0f,
		0.2f,  -0.08f,  -0.4f,   0.0f, 1.0f, 0.0f,     1.0f, 0.8f,
		0.2f,  -0.08f,  -0.4f,	 0.0f, 1.0f, 0.0f,     1.0f, 0.8f,
	   -0.2f,  -0.08f,  -0.4f,	 0.0f, 1.0f, 0.0f,     0.0f, 0.8f,
	   -0.2f,  -0.08f,  -0.55f,  0.0f, 1.0f, 0.0f,	   0.0f, 1.0f

	};

	GLfloat verts2[] = {

		//PLANE
		//back
	    -1.0f, -0.2f, -1.0f,   0.0f, 0.0f, -1.0f,	 0.0f, 0.0f,
		 1.0f, -0.2f, -1.0f,   0.0f, 0.0f, -1.0f,	 1.0f, 0.0f,
		 1.0f, -0.1f, -1.0f,   0.0f, 0.0f, -1.0f,	 1.0f, 1.0f,

		 1.0f, -0.1f, -1.0f,   0.0f, 0.0f, -1.0f,	 1.0f, 1.0f,
		-1.0f, -0.1f, -1.0f,   0.0f, 0.0f, -1.0f,	 0.0f, 1.0f,
		-1.0f, -0.2f, -1.0f,   0.0f, 0.0f, -1.0f,	 0.0f, 0.0f,

		//front
		-1.0f, -0.2f,  1.0f,   0.0f, 0.0f,  1.0f,	 0.0f, 0.0f,
		 1.0f, -0.2f,  1.0f,   0.0f, 0.0f,  1.0f,	 1.0f, 0.0f,
		 1.0f, -0.1f,  1.0f,   0.0f, 0.0f,  1.0f,	 1.0f, 1.0f,

		 1.0f, -0.1f,  1.0f,   0.0f, 0.0f,  1.0f,	 1.0f, 1.0f,
		-1.0f, -0.1f,  1.0f,   0.0f, 0.0f,  1.0f,	 0.0f, 1.0f,
		-1.0f, -0.2f,  1.0f,   0.0f, 0.0f,  1.0f,	 0.0f, 0.0f,

		//left
		-1.0f, -0.1f,  1.0f,  -1.0f, 0.0f, 0.0f,	 1.0f, 0.0f,
		-1.0f, -0.1f, -1.0f,  -1.0f, 0.0f, 0.0f,	 1.0f, 1.0f,
		-1.0f, -0.2f, -1.0f,  -1.0f, 0.0f, 0.0f,	 0.0f, 1.0f,

		-1.0f, -0.2f, -1.0f,  -1.0f, 0.0f, 0.0f,	 0.0f, 1.0f,
		-1.0f, -0.2f,  1.0f,  -1.0f, 0.0f, 0.0f,	 0.0f, 0.0f,
		-1.0f, -0.1f,  1.0f,  -1.0f, 0.0f, 0.0f,	 1.0f, 0.0f,

		//right
		 1.0f, -0.1f,  1.0f,  1.0f, 0.0f, 0.0f,		 1.0f, 0.0f,
		 1.0f, -0.1f, -1.0f,  1.0f, 0.0f, 0.0f,		 1.0f, 1.0f,
		 1.0f, -0.2f, -1.0f,  1.0f, 0.0f, 0.0f,		 0.0f, 1.0f,

		 1.0f, -0.2f, -1.0f,  1.0f, 0.0f, 0.0f,		 0.0f, 1.0f,
		 1.0f, -0.2f,  1.0f,  1.0f, 0.0f, 0.0f,		 0.0f, 0.0f,
		 1.0f, -0.1f,  1.0f,  1.0f, 0.0f, 0.0f,		 1.0f, 0.0f,

		 //bottom
		-1.0f, -0.2f, -1.0f,  0.0f, -1.0f, 0.0f,	 0.0f, 1.0f,
		 1.0f, -0.2f, -1.0f,  0.0f, -1.0f, 0.0f,	 1.0f, 1.0f,
		 1.0f, -0.2f,  1.0f,  0.0f, -1.0f, 0.0f,	 1.0f, 0.0f,
		 1.0f, -0.2f,  1.0f,  0.0f, -1.0f, 0.0f,	 1.0f, 0.0f,
		-1.0f, -0.2f,  1.0f,  0.0f, -1.0f, 0.0f,	 0.0f, 0.0f,
		-1.0f, -0.2f, -1.0f,  0.0f, -1.0f, 0.0f,	 0.0f, 1.0f,

		//top
		-1.0f, -0.1f, -1.0f,  0.0f,  1.0f, 0.0f,	 0.0f, 1.0f,
		 1.0f, -0.1f, -1.0f,  0.0f,  1.0f, 0.0f,	 1.0f, 1.0f,
		 1.0f, -0.1f,  1.0f,  0.0f,  1.0f, 0.0f,	 1.0f, 0.0f,
		 1.0f, -0.1f,  1.0f,  0.0f,  1.0f, 0.0f,	 1.0f, 0.0f,
		-1.0f, -0.1f,  1.0f,  0.0f,  1.0f, 0.0f,	 0.0f, 0.0f,
		-1.0f, -0.1f, -1.0f,  0.0f,  1.0f, 0.0f,	 0.0f, 1.0f
	};

	//LIGHTCUBES
	GLfloat lightvert[] = {
		// Vertex Positions                        //texture position
		//front
		//Back Face          //Negative Z Normal    Texture Coords.
	  -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,   0.0f, 0.0f,
	   0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,   1.0f, 0.0f,
	   0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,	1.0f, 1.0f,
	   0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,   1.0f, 1.0f,
	  -0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,   0.0f, 1.0f,
	  -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,   0.0f, 0.0f,

			//Front Face         //Positive Z Normal
	  -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f, 0.0f,
	   0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f, 0.0f,
	   0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f, 1.0f,
	   0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f, 1.0f,
	  -0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f, 1.0f,
	  -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f, 0.0f,

	 //Left Face          //Negative X Normal
	  -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f, 0.0f,
	  -0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  1.0f, 1.0f,
	  -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
	  -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
	  -0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 0.0f,
	  -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f, 0.0f,

	   //Right Face         //Positive X Normal
	   0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f,
	   0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f,
	   0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
	   0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
	   0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 0.0f,
	   0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f,

	   //Bottom Face        //Negative Y Normal
	  -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 1.0f,
	   0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 1.0f,
	   0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 0.0f,
	   0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 0.0f,
	  -0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 0.0f,
	  -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 1.0f,

	  //Top Face           //Positive Y Normal
	  -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 1.0f,
	   0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 1.0f,
	   0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 0.0f,
	   0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 0.0f,
	  -0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 0.0f,
	  -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 1.0f
		};

	


	const GLuint floatsPerVertex = 3;
	const GLuint floatsPerNormal = 3;
	const GLuint floatsPerUV = 2;


	//PYRAMID

	mesh.nIndices = sizeof(verts) / sizeof(verts[0]) * (floatsPerVertex + floatsPerUV);
	glGenVertexArrays(1, &mesh.vao); // we can also generate multiple VAOs or buffers at the same time

	glBindVertexArray(mesh.vao);


	glGenBuffers(1, &mesh.vbos);
	glBindBuffer(GL_ARRAY_BUFFER, mesh.vbos);
	glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);


	// Strides between vertex coordinates 
	GLint stride = sizeof(float) * (floatsPerVertex + floatsPerNormal + floatsPerUV);// The number of floats before each

	// Create Vertex Attribute Pointers
	glVertexAttribPointer(0, floatsPerVertex, GL_FLOAT, GL_FALSE, stride, 0);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, floatsPerNormal, GL_FLOAT, GL_FALSE, stride, (char*)(sizeof(float) * floatsPerVertex));
	glEnableVertexAttribArray(1);

	glVertexAttribPointer(2, floatsPerUV, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float) * (floatsPerVertex + floatsPerNormal)));
	glEnableVertexAttribArray(2);



	//CUBE
	mesh.cubeIndices = sizeof(vertices1) / sizeof(vertices1[0]) * (floatsPerVertex + floatsPerUV);;

	glGenVertexArrays(1, &mesh.vaoCube);
	glBindVertexArray(mesh.vaoCube);

	glGenBuffers(1, &mesh.vbosCube);
	glBindBuffer(GL_ARRAY_BUFFER, mesh.vbosCube);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices1), vertices1, GL_STATIC_DRAW);


	// Create Vertex Attribute Pointers
	glVertexAttribPointer(0, floatsPerVertex, GL_FLOAT, GL_FALSE, stride, 0);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, floatsPerNormal, GL_FLOAT, GL_FALSE, stride, (char*)(sizeof(float) * floatsPerVertex));
	glEnableVertexAttribArray(1);

	glVertexAttribPointer(2, floatsPerUV, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float) * (floatsPerVertex + floatsPerNormal)));
	glEnableVertexAttribArray(2);


	//PLANE
	mesh.planeIndices = sizeof(verts2) / sizeof(verts2[0]);


	glGenVertexArrays(1, &mesh.vaoPlane); // we can also generate multiple VAOs or buffers at the same time
	glBindVertexArray(mesh.vaoPlane);

	glGenBuffers(1, &mesh.vbosPlane);
	glBindBuffer(GL_ARRAY_BUFFER, mesh.vbosPlane);
	glBufferData(GL_ARRAY_BUFFER, sizeof(verts2), verts2, GL_STATIC_DRAW);

	// Create Vertex Attribute Pointers
	glVertexAttribPointer(0, floatsPerVertex, GL_FLOAT, GL_FALSE, stride, 0);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, floatsPerNormal, GL_FLOAT, GL_FALSE, stride, (char*)(sizeof(float) * floatsPerVertex));
	glEnableVertexAttribArray(1);

	glVertexAttribPointer(2, floatsPerUV, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float) * (floatsPerVertex + floatsPerNormal)));
	glEnableVertexAttribArray(2);

	//Lightcube
	mesh.lightIndices = sizeof(lightvert) / (sizeof(lightvert[0]) * (floatsPerVertex + floatsPerUV));

	glGenVertexArrays(1, &mesh.vaoLight); // we can also generate multiple VAOs or buffers at the same time
	glBindVertexArray(mesh.vaoLight);

	// Create 2 buffers: first one for the vertex data; second one for the indices
	glGenBuffers(1, &mesh.vbosLight);
	glBindBuffer(GL_ARRAY_BUFFER, mesh.vbosLight); // Activates the buffer
	glBufferData(GL_ARRAY_BUFFER, sizeof(lightvert), lightvert, GL_STATIC_DRAW); // Sends vertex or coordinate data to the GPU


	// Create Vertex Attribute Pointers
	glVertexAttribPointer(0, floatsPerVertex, GL_FLOAT, GL_FALSE, stride, 0);
	glEnableVertexAttribArray(0);

	glVertexAttribPointer(1, floatsPerNormal, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float) * floatsPerVertex));
	glEnableVertexAttribArray(1);

	glVertexAttribPointer(2, floatsPerUV, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float) * (floatsPerVertex + floatsPerNormal)));
	glEnableVertexAttribArray(2);

	//Cylinder
	glGenVertexArrays(1, &mesh.vaoCylinder[0]);
	glGenBuffers(1, &mesh.vbosCylinder[0]);
	glBindVertexArray(mesh.vaoCylinder[0]);
	glBindBuffer(GL_ARRAY_BUFFER, mesh.vbosCylinder[0]);

	glGenVertexArrays(1, &mesh.vaoCylinder[1]);
	glGenBuffers(1, &mesh.vbosCylinder[1]);
	glBindVertexArray(mesh.vaoCylinder[1]);
	glBindBuffer(GL_ARRAY_BUFFER, mesh.vbosCylinder[1]);
	
	glGenVertexArrays(1, &mesh.vaoCylinder[2]);
	glGenBuffers(1, &mesh.vbosCylinder[2]);
	glBindVertexArray(mesh.vaoCylinder[2]);
	glBindBuffer(GL_ARRAY_BUFFER, mesh.vbosCylinder[2]);

	glGenVertexArrays(1, &mesh.vaoCylinder[3]);
	glGenBuffers(1, &mesh.vbosCylinder[3]);
	glBindVertexArray(mesh.vaoCylinder[3]);
	glBindBuffer(GL_ARRAY_BUFFER, mesh.vbosCylinder[3]);
};

void UDestroyMesh(GLMesh &mesh)
{
	glDeleteVertexArrays(1, &mesh.vao);
	glDeleteVertexArrays(1, &mesh.vaoCube);
	glDeleteVertexArrays(1, &mesh.vaoLight);
	glDeleteVertexArrays(1, &mesh.vaoPlane);
	glDeleteBuffers(1, &mesh.vbos);
	glDeleteBuffers(1, &mesh.vbosCube);
	glDeleteBuffers(1, &mesh.vbosLight);
	glDeleteBuffers(1, &mesh.vbosPlane);
}
/*Generate and load the texture*/
bool UCreateTexture(const char* filename, GLuint &textureId)
{
	int width, height, channels;
	unsigned char *image = stbi_load(filename, &width, &height, &channels, 0);
	if (image)
	{
		flipImageVertically(image, width, height, channels);

		glGenTextures(1, &textureId);
		glBindTexture(GL_TEXTURE_2D, textureId);

		// set the texture wrapping parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		// set texture filtering parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		if (channels == 3)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
		else if (channels == 4)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
		else
		{
			cout << "Not implemented to handle image with " << channels << " channels" << endl;
			return false;
		}

		glGenerateMipmap(GL_TEXTURE_2D);

		stbi_image_free(image);
		glBindTexture(GL_TEXTURE_2D, 0); // Unbind the texture

		return true;
	}

	// Error loading the image
	return false;
}


void UDestroyTexture(GLuint textureId)
{
	glGenTextures(1, &textureId);
}


// Implements the UCreateShaders function
bool UCreateShaderProgram(const char* vtxShaderSource, const char* fragShaderSource, GLuint &programId)
{
	// Compilation and linkage error reporting
	int success = 0;
	char infoLog[512];

	// Create a Shader program object.
	programId = glCreateProgram();

	// Create the vertex and fragment shader objects
	GLuint vertexShaderId = glCreateShader(GL_VERTEX_SHADER);
	GLuint fragmentShaderId = glCreateShader(GL_FRAGMENT_SHADER);

	// Retrive the shader source
	glShaderSource(vertexShaderId, 1, &vtxShaderSource, NULL);
	glShaderSource(fragmentShaderId, 1, &fragShaderSource, NULL);

	// Compile the vertex shader, and print compilation errors (if any)
	glCompileShader(vertexShaderId); // compile the vertex shader
	// check for shader compile errors
	glGetShaderiv(vertexShaderId, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(vertexShaderId, 512, NULL, infoLog);
		std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;

		return false;
	}

	glCompileShader(fragmentShaderId); // compile the fragment shader
	// check for shader compile errors
	glGetShaderiv(fragmentShaderId, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(fragmentShaderId, sizeof(infoLog), NULL, infoLog);
		std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;

		return false;
	}

	// Attached compiled shaders to the shader program
	glAttachShader(programId, vertexShaderId);
	glAttachShader(programId, fragmentShaderId);

	glLinkProgram(programId);   // links the shader program
	// check for linking errors
	glGetProgramiv(programId, GL_LINK_STATUS, &success);
	if (!success)
	{
		glGetProgramInfoLog(programId, sizeof(infoLog), NULL, infoLog);
		std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;

		return false;
	}

	glUseProgram(programId);    // Uses the shader program

	return true;
}


void UDestroyShaderProgram(GLuint programId)
{
	glDeleteProgram(programId);
}