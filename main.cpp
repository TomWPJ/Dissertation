#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <iostream>
#include <filesystem>
#include <vector>
#include "MCtables.h"
#include <string>
#include <fstream>
#include <sstream>

std::string readShaderSource(const char* vertexPath);
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow *window);
void initPrimitivesBuffer();
void movePrimitive(glm::vec3 moveVal);
bool doSimulation = true;
bool simReset = true;
bool drawLines = false;
int genType = 0;
const unsigned int SCR_WIDTH = 1280;
const unsigned int SCR_HEIGHT = 720;
const unsigned int GRID_SIZE = 80;
int SPHERE_RADIUS = GRID_SIZE / 8;
const unsigned int numVals = GRID_SIZE * GRID_SIZE * GRID_SIZE;
bool doRotate = false;
glm::vec3 viewVal(0.0f, 0.0f, 0.0f);
glm::vec3 viewRotate(0.0f, 0.0f, 0.0f);
int presetView = 0;
double TIME_STEP = 0.0;
double CURRENT_TIME = 0.0;
double PREVIOUS_TIME = 0.0;
glm::vec3 WIND_VELOCITY(0.0f, 0.0f, 0.0f);
bool writeVals = true;
typedef struct {
	float x, y, z;
} Point;

typedef struct {
	Point fbl,fbr,ftl,ftr,rbl,rbr,rtl,rtr;
} Cuboid;

typedef struct {
	float one, two;
} Overflow;

Cuboid collisionPrimitives[1] = {};
GLfloat rigidVertexData[24];

glm::vec4 rigidColor(0.5f, 0.5f, 0.5f, -1.0f);
glm::vec4 particleColor(155.0f/ 255.0f, 118.0f/ 255.0f,83.0f/ 255.0f, 1.0f);

unsigned int rigidIndices[] = {  
	0, 1, 3,
	2, 1, 3
	//4, 5, 7,
	//6, 5, 7,
	//0, 4, 3,
	//7, 4, 3,
	//3, 2, 7,
	//6, 2, 7,
	//2, 1, 6,
	//5, 1, 6,
	//1, 0, 5,
	//4, 0, 5
};

int main()
{
	// INIT GLFW STUFF START
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Dissertation", NULL, NULL);
	if (window == NULL)
	{
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		std::cout << "Failed to initialize GLAD" << std::endl;
		return -1;
	}
	// INIT GLFW STUFF END

	// TEXTURE INIT START
	GLuint densityTex[2];
	glGenTextures(2, densityTex);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_3D, densityTex[0]);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexImage3D(GL_TEXTURE_3D, 0, GL_R32F, GRID_SIZE, GRID_SIZE, GRID_SIZE, 0, GL_RED, GL_FLOAT, NULL);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_3D, densityTex[1]);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexImage3D(GL_TEXTURE_3D, 0, GL_R32F, GRID_SIZE, GRID_SIZE, GRID_SIZE, 0, GL_RED, GL_FLOAT, NULL);

	GLuint velocityTex[2];
	glGenTextures(2, velocityTex);
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_3D, velocityTex[0]);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexImage3D(GL_TEXTURE_3D, 0, GL_RGBA32F, GRID_SIZE, GRID_SIZE, GRID_SIZE, 0, GL_RGBA, GL_FLOAT, NULL);
	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_3D, velocityTex[1]);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexImage3D(GL_TEXTURE_3D, 0, GL_RGBA32F, GRID_SIZE, GRID_SIZE, GRID_SIZE, 0, GL_RGBA, GL_FLOAT, NULL);

	GLuint overflowTex[2];
	glGenTextures(2, overflowTex);
	glActiveTexture(GL_TEXTURE4);
	glBindTexture(GL_TEXTURE_3D, overflowTex[0]);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexImage3D(GL_TEXTURE_3D, 0, GL_R32F, GRID_SIZE, GRID_SIZE, GRID_SIZE, 0, GL_RED, GL_FLOAT, NULL);
	glActiveTexture(GL_TEXTURE5);
	glBindTexture(GL_TEXTURE_3D, overflowTex[1]);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexImage3D(GL_TEXTURE_3D, 0, GL_R32F, GRID_SIZE, GRID_SIZE, GRID_SIZE, 0, GL_RED, GL_FLOAT, NULL);

	// TEXTURE INIT END

	// BUFFERS INIT START
	GLuint TrisSSBO;
	glGenBuffers(1, &TrisSSBO);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, TrisSSBO);
	glBufferData(GL_SHADER_STORAGE_BUFFER, numVals * 3 * 3 * 5 * sizeof(GLfloat), NULL, GL_DYNAMIC_DRAW);

	GLuint mcTablesBuffs[2];
	glGenBuffers(2, mcTablesBuffs);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, mcTablesBuffs[0]);
	glBufferData(GL_SHADER_STORAGE_BUFFER, 256 * 16 * sizeof(int), &triTable, GL_DYNAMIC_DRAW);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, mcTablesBuffs[1]);
	glBufferData(GL_SHADER_STORAGE_BUFFER, 256 * sizeof(int), &edgeTable, GL_DYNAMIC_DRAW);
	// BUFFERS INIT END
	initPrimitivesBuffer();
	// SOIL GEN SHADER
	GLuint soilGen = glCreateProgram();
	GLuint soilGenShader = glCreateShader(GL_COMPUTE_SHADER);
	std::string sourceReturn = readShaderSource("SoilGenerationShader.txt");
	const char *soilGenSource = sourceReturn.c_str();
	int rvalue;
	glShaderSource(soilGenShader, 1, &soilGenSource, NULL);
	glCompileShader(soilGenShader);
	glGetShaderiv(soilGenShader, GL_COMPILE_STATUS, &rvalue);
	if (!rvalue) {
		fprintf(stderr, "Error in compiling the compute shader\n");
		GLchar log[10240];
		GLsizei length;
		glGetShaderInfoLog(soilGenShader, 10239, &length, log);
		fprintf(stderr, "Compiler log:\n%s\n", log);
		exit(40);
	}
	glAttachShader(soilGen, soilGenShader);
	glLinkProgram(soilGen);
	glGetProgramiv(soilGen, GL_LINK_STATUS, &rvalue);
	if (!rvalue) {
		fprintf(stderr, "Error in linking compute shader program\n");
		GLchar log[10240];
		GLsizei length;
		glGetProgramInfoLog(soilGen, 10239, &length, log);
		fprintf(stderr, "Linker log:\n%s\n", log);
		exit(41);
	}
	glUseProgram(soilGen);
	glUniform1i(glGetUniformLocation(soilGen, "GRID_SIZE"), GRID_SIZE);

	// SOIL ADVECTION SHADER
	GLuint soilAdvection = glCreateProgram();
	GLuint soilAdvectionShader = glCreateShader(GL_COMPUTE_SHADER);
	sourceReturn = readShaderSource("SoilAdvectionShader.txt");
	const char *soilAdvectionSource = sourceReturn.c_str();
	glShaderSource(soilAdvectionShader, 1, &soilAdvectionSource, NULL);
	glCompileShader(soilAdvectionShader);
	glGetShaderiv(soilAdvectionShader, GL_COMPILE_STATUS, &rvalue);
	if (!rvalue) {
		fprintf(stderr, "Error in compiling the compute shader\n");
		GLchar log[10240];
		GLsizei length;
		glGetShaderInfoLog(soilAdvectionShader, 10239, &length, log);
		fprintf(stderr, "Compiler log:\n%s\n", log);
		exit(40);
	}
	glAttachShader(soilAdvection, soilAdvectionShader);
	glLinkProgram(soilAdvection);
	glGetProgramiv(soilAdvection, GL_LINK_STATUS, &rvalue);
	if (!rvalue) {
		fprintf(stderr, "Error in linking compute shader program\n");
		GLchar log[10240];
		GLsizei length;
		glGetProgramInfoLog(soilAdvection, 10239, &length, log);
		fprintf(stderr, "Linker log:\n%s\n", log);
		exit(41);
	}
	glUseProgram(soilAdvection);
	glUniform1i(glGetUniformLocation(soilAdvection, "GRID_SIZE"), GRID_SIZE);
	glUniform1f(glGetUniformLocation(soilAdvection, "voxelLim"), 1.0f);

	// MARCHING CUBES SHADER
	GLuint marchingCubes = glCreateProgram();
	GLuint marchingCubesShader = glCreateShader(GL_COMPUTE_SHADER);
	sourceReturn = readShaderSource("MarchingCubesShader.txt");
	const char *marchingCubesSource = sourceReturn.c_str();
	glShaderSource(marchingCubesShader, 1, &marchingCubesSource, NULL);
	glCompileShader(marchingCubesShader);
	glGetShaderiv(marchingCubesShader, GL_COMPILE_STATUS, &rvalue);
	if (!rvalue) {
		fprintf(stderr, "Error in compiling the compute shader\n");
		GLchar log[10240];
		GLsizei length;
		glGetShaderInfoLog(marchingCubesShader, 10239, &length, log);
		fprintf(stderr, "Compiler log:\n%s\n", log);
		exit(40);
	}
	glAttachShader(marchingCubes, marchingCubesShader);
	glLinkProgram(marchingCubes);
	glGetProgramiv(marchingCubes, GL_LINK_STATUS, &rvalue);
	if (!rvalue) {
		fprintf(stderr, "Error in linking compute shader program\n");
		GLchar log[10240];
		GLsizei length;
		glGetProgramInfoLog(marchingCubes, 10239, &length, log);
		fprintf(stderr, "Linker log:\n%s\n", log);
		exit(41);
	}
	glUseProgram(marchingCubes);
	glUniform1i(glGetUniformLocation(marchingCubes, "GRID_SIZE"), GRID_SIZE);

	// OVERFLOW RESOLUTION SHADER
	GLuint overflowResolution = glCreateProgram();
	GLuint overflowResolutionShader = glCreateShader(GL_COMPUTE_SHADER);
	sourceReturn = readShaderSource("OverflowResolutionShader.txt");
	const char *overflowResolutionSource = sourceReturn.c_str();
	glShaderSource(overflowResolutionShader, 1, &overflowResolutionSource, NULL);
	glCompileShader(overflowResolutionShader);
	glGetShaderiv(overflowResolutionShader, GL_COMPILE_STATUS, &rvalue);
	if (!rvalue) {
		fprintf(stderr, "Error in compiling the compute shader\n");
		GLchar log[10240];
		GLsizei length;
		glGetShaderInfoLog(overflowResolutionShader, 10239, &length, log);
		fprintf(stderr, "Compiler log:\n%s\n", log);
		exit(40);
	}
	glAttachShader(overflowResolution, overflowResolutionShader);
	glLinkProgram(overflowResolution);
	glGetProgramiv(overflowResolution, GL_LINK_STATUS, &rvalue);
	if (!rvalue) {
		fprintf(stderr, "Error in linking compute shader program\n");
		GLchar log[10240];
		GLsizei length;
		glGetProgramInfoLog(overflowResolution, 10239, &length, log);
		fprintf(stderr, "Linker log:\n%s\n", log);
		exit(41);
	}
	glUseProgram(overflowResolution);
	glUniform1i(glGetUniformLocation(overflowResolution, "GRID_SIZE"), GRID_SIZE);

	// VERTEX SHADER
	sourceReturn = readShaderSource("VertexShader.txt");
	const char *vertexShaderSource = sourceReturn.c_str();
	unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
	glCompileShader(vertexShader);
	glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &rvalue);
	if (!rvalue) {
		fprintf(stderr, "Error in compiling the shader\n");
		GLchar log[10240];
		GLsizei length;
		glGetShaderInfoLog(vertexShader, 10239, &length, log);
		fprintf(stderr, "Compiler log:\n%s\n", log);
		exit(40);
	}

	// FRAGMENT SHADER
	sourceReturn = readShaderSource("FragmentShader.txt");
	const char *fragmentShaderSource = sourceReturn.c_str();
	unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
	glCompileShader(fragmentShader);
	glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &rvalue);
	if (!rvalue) {
		fprintf(stderr, "Error in compiling the shader\n");
		GLchar log[10240];
		GLsizei length;
		glGetShaderInfoLog(fragmentShader, 10239, &length, log);
		fprintf(stderr, "Compiler log:\n%s\n", log);
		exit(40);
	}

	unsigned int shaderProgram = glCreateProgram();
	glAttachShader(shaderProgram, vertexShader);
	glAttachShader(shaderProgram, fragmentShader);
	glLinkProgram(shaderProgram);
	glGetProgramiv(shaderProgram, GL_LINK_STATUS, &rvalue);
	if (!rvalue) {
		fprintf(stderr, "Error in linking compute shader program\n");
		GLchar log[10240];
		GLsizei length;
		glGetProgramInfoLog(shaderProgram, 10239, &length, log);
		fprintf(stderr, "Linker log:\n%s\n", log);
		exit(41);
	}
	glUseProgram(shaderProgram);
	glUniform1i(glGetUniformLocation(shaderProgram, "GRID_SIZE"), GRID_SIZE);


	unsigned int VAO;
	glGenVertexArrays(1, &VAO);
	unsigned int EBO;
	glGenBuffers(1, &EBO);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(rigidIndices), rigidIndices, GL_STATIC_DRAW);

	unsigned int VAO2;
	glGenVertexArrays(1, &VAO2);
	unsigned int VAO3;
	glGenVertexArrays(1, &VAO3);
	unsigned int VBO;
	glGenBuffers(1, &VBO);
	glBindVertexArray(VAO2);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(rigidVertexData), rigidVertexData, GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(rigidIndices), rigidIndices, GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);



	// render loop
	// -----------
	bool switchVal = false;
	double prevTime = glfwGetTime();
	double curTime;


	PREVIOUS_TIME = glfwGetTime();
	glm::vec3 rigidMoveVal(0.0f,0.0f,0.0f);
	writeVals = false;
	glBindImageTexture(0, densityTex[0], 0, GL_TRUE, 0, GL_READ_WRITE, GL_R32F);
	glBindImageTexture(1, densityTex[1], 0, GL_TRUE, 0, GL_READ_WRITE, GL_R32F);
	glBindImageTexture(2, velocityTex[0], 0, GL_TRUE, 0, GL_READ_WRITE, GL_RGBA32F);
	glBindImageTexture(3, velocityTex[1], 0, GL_TRUE, 0, GL_READ_WRITE, GL_RGBA32F);
	glBindImageTexture(4, overflowTex[0], 0, GL_TRUE, 0, GL_READ_WRITE, GL_R32F);
	glBindImageTexture(5, overflowTex[1], 0, GL_TRUE, 0, GL_READ_WRITE, GL_R32F);
	GLint dIS = 1, dOS = 0, vIS = 3, vOS = 2, oIS = 5, oOS = 4;
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);
	int counter = 0;
	double totalTime = 0.0;
	int soilGenOut = 1;
	int soilAdvectOut = 0;
	int soilAdvectIn = 0;
	int holdVal = 0;
	while (!glfwWindowShouldClose(window))
	{
		processInput(window);
		glClearColor(0.8f, 0.8f, 1.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		
		if (simReset) {
			dIS = 1; dOS = 0; vIS = 3; vOS = 2; oIS = 5; oOS = 4;
			glUseProgram(soilGen);
			glUniform1i(glGetUniformLocation(soilGen, "densityTexOut"), soilGenOut);
			glUniform1i(glGetUniformLocation(soilGen, "velocityTexOut"), 2 + soilGenOut);
			glUniform1i(glGetUniformLocation(soilGen, "SPHERE_RAD"), SPHERE_RADIUS);
			glUniform1i(glGetUniformLocation(soilGen, "genType"), genType);
			glDispatchCompute(GRID_SIZE / 4, GRID_SIZE / 4, GRID_SIZE / 4);
			doSimulation = false;
			soilGenOut = 1;
			soilAdvectOut = 0;
			soilAdvectIn = 1;
		}
		
		if (doSimulation) {
		

			CURRENT_TIME = glfwGetTime();
			TIME_STEP = CURRENT_TIME - PREVIOUS_TIME;
			PREVIOUS_TIME = CURRENT_TIME;

		
			glUseProgram(soilAdvection);
			glUniform1i(glGetUniformLocation(soilAdvection, "densityTexIn"), soilAdvectIn);
			glUniform1i(glGetUniformLocation(soilAdvection, "densityTexOut"), soilAdvectOut);
			glUniform1i(glGetUniformLocation(soilAdvection, "velocityTexIn"), 2 + soilAdvectIn);
			glUniform1i(glGetUniformLocation(soilAdvection, "velocityTexOut"), 2 + soilAdvectOut);
			glUniform1f(glGetUniformLocation(soilAdvection, "timeStep"), (float)TIME_STEP);
			glUniform3f(glGetUniformLocation(soilAdvection, "windVelocity"), WIND_VELOCITY.x, WIND_VELOCITY.y, WIND_VELOCITY.z);
			glDispatchCompute(GRID_SIZE/4, GRID_SIZE/4, GRID_SIZE/4);
			glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

			glUseProgram(overflowResolution);
			glUniform1i(glGetUniformLocation(overflowResolution, "densityTexIn"), soilAdvectOut);
			glUniform1i(glGetUniformLocation(overflowResolution, "densityTexOut"), soilAdvectIn);
			glUniform1i(glGetUniformLocation(overflowResolution, "overflowTexIn"), 4 + soilAdvectOut);
			glUniform1i(glGetUniformLocation(overflowResolution, "overflowTexOut"), 4 + soilAdvectIn);
			glUniform1i(glGetUniformLocation(overflowResolution, "readOverflow"), 0);
			glDispatchCompute(GRID_SIZE/4, GRID_SIZE/4, GRID_SIZE/4);
			glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
			glUniform1i(glGetUniformLocation(overflowResolution, "readOverflow"), 1);
			for (int i = 0; i < 2; i++) {
				glUniform1i(glGetUniformLocation(overflowResolution, "densityTexIn"), soilAdvectIn);
				glUniform1i(glGetUniformLocation(overflowResolution, "densityTexOut"), soilAdvectOut);
				glUniform1i(glGetUniformLocation(overflowResolution, "overflowTexIn"), 4 + soilAdvectIn);
				glUniform1i(glGetUniformLocation(overflowResolution, "overflowTexOut"), 4 + soilAdvectOut);
				glDispatchCompute(GRID_SIZE / 4, GRID_SIZE / 4, GRID_SIZE / 4);
				glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
				glUniform1i(glGetUniformLocation(overflowResolution, "densityTexIn"), soilAdvectOut);
				glUniform1i(glGetUniformLocation(overflowResolution, "densityTexOut"), soilAdvectIn);
				glUniform1i(glGetUniformLocation(overflowResolution, "overflowTexIn"), 4 + soilAdvectOut);
				glUniform1i(glGetUniformLocation(overflowResolution, "overflowTexOut"), 4 + soilAdvectIn);
				glDispatchCompute(GRID_SIZE / 4, GRID_SIZE / 4, GRID_SIZE / 4);
				glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
			}
			glUniform1i(glGetUniformLocation(overflowResolution, "densityTexIn"), soilAdvectIn);
			glUniform1i(glGetUniformLocation(overflowResolution, "densityTexOut"), soilAdvectOut);
			glUniform1i(glGetUniformLocation(overflowResolution, "overflowTexIn"), 4 + soilAdvectIn);
			glUniform1i(glGetUniformLocation(overflowResolution, "overflowTexOut"), 4 + soilAdvectOut);
			glUniform1i(glGetUniformLocation(overflowResolution, "readOverflow"), 2);
			glDispatchCompute(GRID_SIZE / 4, GRID_SIZE / 4, GRID_SIZE / 4);
			glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

			glUseProgram(marchingCubes);
			glUniform1i(glGetUniformLocation(marchingCubes, "densityTexIn"), soilAdvectOut);
			glDispatchCompute(GRID_SIZE / 4, GRID_SIZE / 4, GRID_SIZE / 4);
			glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
			holdVal = soilAdvectIn;
			soilAdvectIn = soilAdvectOut;
			soilAdvectOut = holdVal;
		}
		else {
			glUseProgram(marchingCubes);
			glUniform1i(glGetUniformLocation(marchingCubes, "densityTexIn"), soilGenOut);
			glDispatchCompute(GRID_SIZE / 4, GRID_SIZE / 4, GRID_SIZE / 4);
			glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
		}
		

		glUseProgram(shaderProgram);
		glm::mat4 model;
		glm::mat4 view;
		glm::mat4 projection;
		if (doRotate) model = glm::rotate(model, (float)(glfwGetTime()) * glm::radians(-55.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		else model = glm::rotate(model, glm::radians(0.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		if (presetView == 0) view = glm::rotate(glm::rotate(glm::translate(view, glm::vec3(0.0f, 0.0f, -3.0f)+viewVal), glm::radians(1.0f)*viewRotate.x, glm::vec3(1.0f, 0.0f, 0.0f)), glm::radians(1.0f)*viewRotate.y, glm::vec3(0.0f, 1.0f, 0.0f));
		else if (presetView == 1) view = glm::rotate(glm::translate(view, glm::vec3(0.0f, 0.0f, 0.0f) + viewVal), glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		projection = glm::perspective(glm::radians(45.0f), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);

		glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
		glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, &view[0][0]);
		glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, &projection[0][0]);

		glBindVertexArray(VAO);
		glBindBuffer(GL_ARRAY_BUFFER, TrisSSBO);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
		glEnableVertexAttribArray(0);
		glUniform4f(glGetUniformLocation(shaderProgram, "materialColor"), particleColor.r,particleColor.g,particleColor.b,particleColor.a);
		if (drawLines) glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		glDrawArrays(GL_TRIANGLES, 0, numVals*15);

		glBindVertexArray(VAO2);
		glBindBuffer(GL_ARRAY_BUFFER, VBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(rigidVertexData), rigidVertexData, GL_STATIC_DRAW);
		glUniform4f(glGetUniformLocation(shaderProgram, "materialColor"), rigidColor.r, rigidColor.g, rigidColor.b, rigidColor.a);
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
		glBindVertexArray(0);


		glfwSwapBuffers(window);
		glfwPollEvents();
		curTime = glfwGetTime();
		
		totalTime += (curTime - prevTime);
		prevTime = curTime;
		counter++;
		if (counter % 20 == 0) std::cout << "(" << WIND_VELOCITY.x << "," << WIND_VELOCITY.y << "," << WIND_VELOCITY.z << ")\n";
		if (counter == 100) {
			
			std::cout << "FPS : " << 100 / totalTime << "\n";
			counter = 0;
			totalTime = 0.0;
		}
	}

	glfwTerminate();
	return 0;
}

void processInput(GLFWwindow *window)
{
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(window, true);
	if (glfwGetKey(window, GLFW_KEY_L) == GLFW_PRESS)
		drawLines = true;
	if (glfwGetKey(window, GLFW_KEY_O) == GLFW_PRESS)
		drawLines = false;
	if (glfwGetKey(window, GLFW_KEY_B) == GLFW_PRESS) {
		simReset = true;
		WIND_VELOCITY = glm::vec3(0.0f, 0.0f, 0.0f);
	}
	if (glfwGetKey(window, GLFW_KEY_V) == GLFW_PRESS) {
		viewRotate = glm::vec3(0.0f, 0.0f, 0.0f);
		viewVal = glm::vec3(0.0f, 0.0f, 0.0f);
	}
	if (glfwGetKey(window, GLFW_KEY_Z) == GLFW_PRESS) {
		viewVal.y += 0.05f;
	}
	if (glfwGetKey(window, GLFW_KEY_C) == GLFW_PRESS) {
		viewVal.y -= 0.05f;
	}
	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
		viewVal.z += 0.05f;
	}
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
		viewVal.z -= 0.05f;
	}
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
		viewVal.x += 0.05f;
	}
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
		viewVal.x -= 0.05f;
	}
	if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) {
		viewRotate.y += 1.0f;
	}
	if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) {
		viewRotate.y -= 1.0f;
	}
	if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS) {
		viewRotate.x += 1.0f;
	}
	if (glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS) {
		viewRotate.x -= 1.0f;
	}
	if (glfwGetKey(window, GLFW_KEY_EQUAL) == GLFW_PRESS) {
		if (SPHERE_RADIUS < GRID_SIZE/2-1) SPHERE_RADIUS += 1;
	}
	if (glfwGetKey(window, GLFW_KEY_MINUS) == GLFW_PRESS) {
		if (SPHERE_RADIUS > 1) SPHERE_RADIUS -= 1;
	}
	if (glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS) {
		if (genType == 2) SPHERE_RADIUS = GRID_SIZE / 8;
		genType = 1;
	}
	if (glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS) {
		genType = 2;
		SPHERE_RADIUS = (GRID_SIZE / 2 + 1)-8;
	}
	if (glfwGetKey(window, GLFW_KEY_0) == GLFW_PRESS) {
		if (genType == 2) SPHERE_RADIUS = GRID_SIZE / 8;
		genType = 0;
	}
	if (glfwGetKey(window, GLFW_KEY_M) == GLFW_PRESS) {
		doSimulation = false;
	}
	if (glfwGetKey(window, GLFW_KEY_N) == GLFW_PRESS) {
		doSimulation = true;
		PREVIOUS_TIME = glfwGetTime();
		simReset = false;
	}

	if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)
		WIND_VELOCITY.z += 0.2f;
	if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)
		WIND_VELOCITY.z -= 0.2f;
	if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS)
		WIND_VELOCITY.x += 0.2f;
	if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)
		WIND_VELOCITY.x -= 0.2f;
	if (glfwGetKey(window, GLFW_KEY_K) == GLFW_PRESS)
		WIND_VELOCITY = glm::vec3(0.0f, 0.0f, 0.0f);
	if (glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS)
		writeVals = true;
}


void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
	glViewport(0, 0, width, height);
}

std::string readShaderSource(const char* sourcePath) {
	std::string sourceCode;
	std::ifstream sourceFile;
	sourceFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
	try
	{
		sourceFile.open(sourcePath);
		std::stringstream sourceStream;
		sourceStream << sourceFile.rdbuf();
		sourceFile.close();
		sourceCode = sourceStream.str();
	}
	catch (std::ifstream::failure e)
	{
		std::cout << "ERROR::SHADER::FILE_NOT_SUCCESFULLY_READ" << std::endl;
	}
	return sourceCode;
}

void initPrimitivesBuffer() {
	collisionPrimitives[0].fbl.x = -1.0f; collisionPrimitives[0].fbl.y = -1.0f; collisionPrimitives[0].fbl.z = -1.0f;
	collisionPrimitives[0].rbl.x = -1.0f; collisionPrimitives[0].rbl.y = -1.0f; collisionPrimitives[0].rbl.z = 64.0f;
	collisionPrimitives[0].rbr.x = 64.0f; collisionPrimitives[0].rbr.y = -1.0f; collisionPrimitives[0].rbr.z = 64.0f;
	collisionPrimitives[0].fbr.x = 64.0f; collisionPrimitives[0].fbr.y = -1.0f; collisionPrimitives[0].fbr.z = -1.0f;
	collisionPrimitives[0].ftl.x = -1.0f; collisionPrimitives[0].ftl.y = 4.0f; collisionPrimitives[0].ftl.z = -1.0f;
	collisionPrimitives[0].rtl.x = -1.0f; collisionPrimitives[0].rtl.y = 4.0f; collisionPrimitives[0].rtl.z = 64.0f;
	collisionPrimitives[0].rtr.x = 64.0f; collisionPrimitives[0].rtr.y = 4.0f; collisionPrimitives[0].rtr.z = 64.0f;
	collisionPrimitives[0].ftr.x = 64.0f; collisionPrimitives[0].ftr.y = 4.0f; collisionPrimitives[0].ftr.z = -1.0f;

	float newVal = (float)GRID_SIZE + 16.0f;

	rigidVertexData[0] = -16.0f; rigidVertexData[3] = -16.0f;  rigidVertexData[6] = newVal; rigidVertexData[9] = newVal; //rigidVertexData[12] = -1.0f; rigidVertexData[15] = -1.0f;  rigidVertexData[18] = 64.0f; rigidVertexData[21] = 64.0f;
	rigidVertexData[1] = 0.0f; rigidVertexData[4] = 0.0f;  rigidVertexData[7] = 0.0f;  rigidVertexData[10] = 0.0f; //rigidVertexData[13] = 4.0f; rigidVertexData[16] = 4.0f;  rigidVertexData[19] = 4.0f;  rigidVertexData[22] = 4.0f;
	rigidVertexData[2] = -16.0f; rigidVertexData[5] = newVal; rigidVertexData[8] = newVal; rigidVertexData[11] = -16.0f; //rigidVertexData[14] = -1.0f; rigidVertexData[17] = 64.0f; rigidVertexData[20] = 64.0f; rigidVertexData[23] = -1.0f;
}

void movePrimitive(glm::vec3 moveVal) {
	collisionPrimitives[0].fbl.x += moveVal.x; collisionPrimitives[0].rbl.x += moveVal.x; collisionPrimitives[0].rbr.x += moveVal.x; collisionPrimitives[0].fbr.x += moveVal.x; collisionPrimitives[0].ftl.x += moveVal.x; collisionPrimitives[0].rtl.x += moveVal.x; collisionPrimitives[0].rtr.x += moveVal.x; collisionPrimitives[0].ftr.x += moveVal.x;
	collisionPrimitives[0].fbl.y += moveVal.y; collisionPrimitives[0].rbl.y += moveVal.y; collisionPrimitives[0].rbr.y += moveVal.y; collisionPrimitives[0].fbr.y += moveVal.y; collisionPrimitives[0].ftl.y += moveVal.y; collisionPrimitives[0].rtl.y += moveVal.y; collisionPrimitives[0].rtr.y += moveVal.y; collisionPrimitives[0].ftr.y += moveVal.y;
	collisionPrimitives[0].fbl.z += moveVal.z; collisionPrimitives[0].rbl.z += moveVal.z; collisionPrimitives[0].rbr.z += moveVal.z; collisionPrimitives[0].fbr.z += moveVal.z; collisionPrimitives[0].ftl.z += moveVal.z; collisionPrimitives[0].rtl.z += moveVal.z; collisionPrimitives[0].rtr.z += moveVal.z; collisionPrimitives[0].ftr.z += moveVal.z;

	rigidVertexData[0] += moveVal.x; rigidVertexData[3] += moveVal.x;  rigidVertexData[6] += moveVal.x; rigidVertexData[9] += moveVal.x; //rigidVertexData[12] += moveVal.x; rigidVertexData[15] += moveVal.x;  rigidVertexData[18] += moveVal.x; rigidVertexData[21] += moveVal.x;
	rigidVertexData[1] += moveVal.y; rigidVertexData[4] += moveVal.y;  rigidVertexData[7] += moveVal.y;  rigidVertexData[10] += moveVal.y; //rigidVertexData[13] += moveVal.y; rigidVertexData[16] += moveVal.y;  rigidVertexData[19] += moveVal.y;  rigidVertexData[22] += moveVal.y;
	rigidVertexData[2] += moveVal.z; rigidVertexData[5] += moveVal.z; rigidVertexData[8] += moveVal.z; rigidVertexData[11] += moveVal.z; //rigidVertexData[14] += moveVal.z; rigidVertexData[17] += moveVal.z; rigidVertexData[20] += moveVal.z; rigidVertexData[23] += moveVal.z;
}