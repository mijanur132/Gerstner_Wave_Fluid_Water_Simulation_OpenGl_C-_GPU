#include <stdio.h>
#include <vector>
#include <iostream>
#include "GLEW\glew.h"
#include "GLFW\glfw3.h"
#include "glm\glm.hpp"
#include "glm\gtc\matrix_transform.hpp"
#include <random>
#include "util.h"



#define INDEX(x, z, x_width) (x+z*x_width)
#define MAX_WAVE_NUMBER 30
#define PI 3.1415926535897932384626433832795
#define G 9.80665

using namespace glm;
using namespace std;


typedef unsigned long DWORD;
extern "C" {	// Force using Nvidia GPU. Turn 0 if don't want to.
	_declspec(dllexport) DWORD NvOptimusEnablement = 0x00000000;
}

GLFWwindow* window;
float window_width = 1200;
float window_height = 1000;
vec3 BGColor(226 / 255.0, 143 / 255.0, 41 / 255.0);


mat4 ViewMatrix;
mat4 ProjectionMatrix;
vec3 position;

void keyCallback(GLFWwindow* window, int key, int sc, int action, int mode);
void mouseButtonCallback(GLFWwindow* window, int button, int action, int mode);

mat4 getViewMatrix()
{
	return ViewMatrix;
}
mat4 getProjectionMatrix()
{
	return ProjectionMatrix;
}
vec3 getEyePos()
{
	return position;
}


float horizontalAngle = pi<float>();
float verticalAngle = -pi<float>() / 5;
float initialFoV = 90;

float speed = 10;
float mouseSpeed = 0.005f;

void getModelViewProj()
{
	static double lastTime = glfwGetTime();
	double currentTime = glfwGetTime();
	float deltaTime = float(currentTime - lastTime);
	
	float FoV = initialFoV;

	ProjectionMatrix = perspective(
		FoV
		, window_width / window_height
		, 0.1f
		, 10000.0f
	);

	ViewMatrix = glm::lookAt(
		position
		, vec3(10, 0, 10)
		, vec3(0,1,0)
	);

	lastTime = currentTime;
}

class ObjctModify
{
private:
	vector<GLfloat> vertices;
	vector<unsigned int> indices;

	void vectorOp(vector<GLfloat> data, vector<GLfloat> &targetData, GLuint &targetBuffer);
	void vectorOpVec3(vector<vec3> data, vector<GLfloat> &targetData, GLuint &targetBuffer);

public:
	GLuint vertices_buffer;
	GLuint indices_buffer;
	void SetVertex(vector<vec3> vertices);
	void SetIndices(vector<unsigned int> indices);

};

void ObjctModify::vectorOp(vector<GLfloat> data, vector<GLfloat> &targetData, GLuint &targetBuffer)
{
	targetData = data;
	glGenBuffers(1, &targetBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, targetBuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * targetData.size(), &targetData[0], GL_STATIC_DRAW);
}

void ObjctModify::vectorOpVec3(vector<vec3> data, vector<GLfloat> &targetData, GLuint &targetBuffer)
{
	for (int i = 0; i < data.size(); i++)
	{
		targetData.push_back(data[i].x);
		targetData.push_back(data[i].y);
		targetData.push_back(data[i].z);
	}
	vectorOp(targetData, targetData, targetBuffer);
}


void ObjctModify::SetVertex(vector<vec3> vertices)
{
	vectorOpVec3(vertices, this->vertices, vertices_buffer);
}



void ObjctModify::SetIndices(vector<unsigned int> indices)
{
	this->indices = indices;
	glGenBuffers(1, &indices_buffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indices_buffer);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * this->indices.size(), &indices[0], GL_STATIC_DRAW);
}


class Area
{
private:
	int x_width;
	int z_width;
	float spacing;
	int x_instance;
	int z_instance;

	vector<vec3> vertices;
	vector<unsigned int> indices;
	vector<vec2> UVs;

public:

	vector<vec3> GetVertices()
	{
		return vertices;
	}
	vector<unsigned int> GetIndices()
	{
		return indices;
	}
	vector<vec2> GetUVs()
	{
		return UVs;
	}
	
	Area(int width, int length, GLfloat spacing, int x_instance, int z_instance);

};



Area::Area(int x_width, int z_width, GLfloat spacing, int x_instance, int z_instance)
{

	this->x_width = x_width;
	this->z_width = z_width;
	this->spacing = spacing;
	this->x_instance = x_instance;
	this->z_instance = z_instance;

	for (int z = 0; z < z_width; z++)
	{
		for (int x = 0; x < x_width; x++)
		{
			vertices.push_back(vec3(x * spacing, 0, z * spacing));

			UVs.push_back(vec2(z / (float)z_width, x / (float)x_width));
			if (z > 0 && x > 0)
			{
				indices.push_back(INDEX((x - 1), (z - 1), x_width));
				indices.push_back(INDEX((x - 1), (z - 0), x_width));
				indices.push_back(INDEX((x - 0), (z - 0), x_width));
				indices.push_back(INDEX((x - 1), (z - 1), x_width));
				indices.push_back(INDEX((x - 0), (z - 0), x_width));
				indices.push_back(INDEX((x - 0), (z - 1), x_width));
			}
		}
	}

	float x_space = (x_width - 1) * spacing;
	float z_space = (z_width - 1) * spacing;


}


struct WaveVar
{
	vec2 WaveDir;
	float WaveLength;
	float Speed;

	float KAmpOverLen;
	float Phase;
	float padding1;
	float padding2;
};
class AllWave : public Area
{
private:
	float mainPhase;
	default_random_engine generator;
	normal_distribution<float> length_distribution;
	normal_distribution<float> amplitude_distribution;
	normal_distribution<float> direction_radiance_distribution;
	normal_distribution<float> phase_distribution;


public:
	float Amplitude;
	vec2 WaveDir;
	float WaveLength;
	float GlobalSteepness;
	float WaveNumber;

	WaveVar waves[MAX_WAVE_NUMBER];
	AllWave(int width, int length, GLfloat spacing, int x_instance, int z_instance, float Amplitude, vec2 WaveDir, float WaveLength, float GlobalSteepness, float WaveNumber):Area(width, length, spacing, x_instance, z_instance) {
	
		this->Amplitude = Amplitude;
		this->WaveDir = normalize(WaveDir);
		this->WaveLength = WaveLength;
		this->GlobalSteepness = GlobalSteepness;
		this->WaveNumber = WaveNumber;
		length_distribution = normal_distribution<float>(WaveLength, 2);
		amplitude_distribution = normal_distribution<float>(Amplitude / WaveNumber, .3);

		float radiance = atan2(WaveDir.y, WaveDir.x) * 180 / PI;
		direction_radiance_distribution = normal_distribution<float>(radiance, 20);

		float mainSpeed = sqrt(2 * G * PI / WaveLength);
		float mainOmega = 2 * PI / WaveLength;
		mainPhase = mainSpeed * mainOmega;
		phase_distribution = normal_distribution<float>(mainPhase, 1);

		for (int i = 0; i < WaveNumber; i++)
		{
			
			float direction_radiance_rand = direction_radiance_distribution(generator);
			float theta = direction_radiance_rand * PI / 180;
			waves[i].WaveDir.x = cos(theta);
			waves[i].WaveDir.y = sin(theta);

			waves[i].WaveLength = length_distribution(generator);
			waves[i].KAmpOverLen = amplitude_distribution(generator) / waves[i].WaveLength;
			waves[i].Speed = sqrt(G * 2 * PI / waves[i].WaveLength);
			waves[i].Phase = phase_distribution(generator);
			int x = 5;
		}
	
	
	}
};

GLuint shaderProgramID;
AllWave AllWaveObj(256, 256, .25, 5, 5
	, 10, vec2(1, 0), 9, 0.1, MAX_WAVE_NUMBER);
GLuint globalSteepnessID;
GLuint waveNumberID;
GLuint EyePositionID;
vec3 eyePos;
GLuint LightPosition_worldspaceID;
float lightPositionx, lightPositiony, lightPositionz;
int InitProgram();
void SendUniformMVP();
GLuint shaders;
void SendUniformWaveVars(AllWave AllWaveObj)
{
	glBufferData(GL_UNIFORM_BUFFER, sizeof(WaveVar) * MAX_WAVE_NUMBER, &AllWaveObj.waves[0], GL_STATIC_DRAW);
}

int main()
{
	if (InitProgram() != 0)
		return -1;

	ShaderGenerator shaderProgram;
	shaderProgram.AddShader("wave_vertx_shad.glsl", GL_VERTEX_SHADER);
	//shaderProgram.AddShader("wave_normal_shad.glsl", GL_FRAGMENT_SHADER);
	//shaderProgram.AddShader("f_ambient_diffuse_specular.glsl", GL_FRAGMENT_SHADER);

	shaderProgramID = shaderProgram.LinkProgram();
	//std::vector<GLuint> shaders;
	//shaders.push_back(compileShader(GL_VERTEX_SHADER, "wave_vertx_shad.glsl"));	
	//shaderProgramID = linkProgram(shaders);	
	//assert(glGetError() == GL_NO_ERROR);

	GLuint VertexArrayID;
	glGenVertexArrays(1, &VertexArrayID);
	glBindVertexArray(VertexArrayID);

	cout <<"***************************"<< endl;
	
	cout << "Press ESC to close; UP and Down to increase or decrease steepness parameter of Garstner wave; 1 and 2 to change eye position; 3 and 4 To increase and decrease wavenumber (Max wave number is 30 which is the system starts with); ///5 and 6 to change wave direction; 7 and 8 to increae or decrease wavelength; 9 and 0 to increase and decrease wave speed; Left and Right to change light position; " << endl;
	cout << "Also you can left click mouse to change your eye position to look the wave any time" << endl;
	cout << "***************************" << endl;

	ObjctModify AllWaveObjBuffer;
	AllWaveObjBuffer.SetVertex(AllWaveObj.GetVertices());
	AllWaveObjBuffer.SetIndices(AllWaveObj.GetIndices());	

	GLuint mvp_uniform_block;
	glGenBuffers(1, &mvp_uniform_block);
	glBindBuffer(GL_UNIFORM_BUFFER, mvp_uniform_block);
	glBufferData(GL_UNIFORM_BUFFER, sizeof(mat4) * 3, NULL, GL_STATIC_DRAW);
	glBindBufferBase(GL_UNIFORM_BUFFER, 0, mvp_uniform_block);

	GLuint wave_uniform_block;
	glGenBuffers(1, &wave_uniform_block);
	glBindBuffer(GL_UNIFORM_BUFFER, wave_uniform_block);
	glBufferData(GL_UNIFORM_BUFFER, sizeof(int) + sizeof(float) * 4 + sizeof(vec2), NULL, GL_STATIC_DRAW);
	glBindBufferBase(GL_UNIFORM_BUFFER, 1, wave_uniform_block);

	GLuint timeID = glGetUniformLocation(shaderProgramID, "time");
	waveNumberID = glGetUniformLocation(shaderProgramID, "WaveNumber");
	globalSteepnessID = glGetUniformLocation(shaderProgramID, "GlobalSteepness");
	LightPosition_worldspaceID = glGetUniformLocation(shaderProgramID, "LightPosition_worldspace");
	EyePositionID = glGetUniformLocation(shaderProgramID, "EyePosition");
	GLuint DirectionalLight_direction_worldspaceID = glGetUniformLocation(shaderProgramID, "DirectionalLight_direction_worldspace");
	
	glfwSetTime(0);
	while (!glfwWindowShouldClose(window))
	{
		glfwPollEvents();
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glUseProgram(shaderProgramID);
		glUniform1f(timeID, glfwGetTime());
		glUniform1i(waveNumberID, AllWaveObj.WaveNumber);
		glUniform1f(globalSteepnessID, AllWaveObj.GlobalSteepness);
		lightPositionx = 128 * .25 / 2; 
		lightPositiony = 10;
		lightPositionz = 128 * .25 / 2;
		glUniform3f(LightPosition_worldspaceID, lightPositionx, lightPositiony, lightPositionz);
		eyePos = getEyePos();
		glUniform3f(EyePositionID, eyePos.x, eyePos.y, eyePos.z);
		glUniform3f(DirectionalLight_direction_worldspaceID, -1, -1, -1);

		glBindBuffer(GL_UNIFORM_BUFFER, mvp_uniform_block);
		SendUniformMVP();
		glBindBuffer(GL_UNIFORM_BUFFER, wave_uniform_block);
		SendUniformWaveVars(AllWaveObj);
		
		glEnableVertexAttribArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, AllWaveObjBuffer.vertices_buffer);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*) 0);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, AllWaveObjBuffer.indices_buffer);
		//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		glDrawElementsInstanced(GL_TRIANGLES, AllWaveObj.GetIndices().size(), GL_UNSIGNED_INT, (void*) 0,1);  //1 here gives only one segment. 

		glfwSwapBuffers(window);
		
	} 


	glDeleteProgram(shaderProgramID);
	glfwTerminate();
	return 0;
}

int InitProgram()
{
	
	position = vec3(10, 15, 0);
	if (!glfwInit())
	{
		fprintf(stderr, "Failed to init glfw\n");
		return -1;
	}
	glfwWindowHint(GLFW_SAMPLES, 16);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);

	window = glfwCreateWindow(window_width, window_height, "Wave Simulation", NULL, NULL);

	if (window == NULL)
	{
		fprintf(stderr, "Failed to open glfw window");
		glfwTerminate();
		return -1;
	}
	glfwSetKeyCallback(window, keyCallback);
	glfwSetMouseButtonCallback(window, mouseButtonCallback);
	glfwMakeContextCurrent(window);

	glewExperimental = true;
	if (glewInit() != GLEW_OK)
	{
		fprintf(stderr, "Failed to init GLEW");
		return -1;
	}
	glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);
	glfwSetCursorPos(window, window_width / 2, window_height / 2);

	//glClearColor(0.8, 0.8, 0.8, 0);
	glClearColor(BGColor.x, BGColor.y, BGColor.z, 1);

	//Z-Buffer
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);
	glEnable(GL_CULL_FACE);
	glEnable(GL_PROGRAM_POINT_SIZE);

	return 0;
}


void SendUniformMVP()
{
	getModelViewProj();
	mat4 projection = getProjectionMatrix();
	mat4 view = getViewMatrix();
	mat4 model = mat4(1);

	glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(mat4), &model[0][0]);
	glBufferSubData(GL_UNIFORM_BUFFER, sizeof(mat4), sizeof(mat4), &view[0][0]);
	glBufferSubData(GL_UNIFORM_BUFFER, sizeof(mat4) * 2, sizeof(mat4), &projection[0][0]);
}

void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mode) {
	if (key == GLFW_KEY_ESCAPE && action == GLFW_RELEASE) {
		glfwSetWindowShouldClose(window, true);
	}
	if (key == GLFW_KEY_1 && action == GLFW_RELEASE) {
		cout << "position Eye +" << endl;
		position += vec3(-1, 0, 1);
		return;
			
	}
	if (key == GLFW_KEY_2 && action == GLFW_RELEASE) {
		cout << "Position Eye -" << endl;
		position -= vec3(-1, 0, 1);
		return;

	}
	if (key == GLFW_KEY_RIGHT && action == GLFW_RELEASE) {
		cout << "R pressed" << endl;
		lightPositionx = lightPositionx + 5;
		lightPositiony = lightPositiony -2;
		lightPositionz = lightPositionz + 5;
		glUniform3f(LightPosition_worldspaceID, lightPositionx, lightPositiony, lightPositionz);
		return;

	}
	if (key == GLFW_KEY_LEFT && action == GLFW_RELEASE) {
		cout << "L pressed" << endl;
		lightPositionx = lightPositionx - 5;
		lightPositiony = lightPositiony + 2;
		lightPositionz = lightPositionz - 5;
		glUniform3f(LightPosition_worldspaceID, lightPositionx, lightPositiony, lightPositionz);
		return;

	}

	if (key == GLFW_KEY_UP && action == GLFW_RELEASE) {
		cout << "UP pressed" << endl;
		cout << "current steepness:" << " " << AllWaveObj.GlobalSteepness << endl;
		AllWaveObj.GlobalSteepness = AllWaveObj.GlobalSteepness + 0.2;
		cout << "updated steepness:" << " " << AllWaveObj.GlobalSteepness << endl;
		glUniform1f(globalSteepnessID, AllWaveObj.GlobalSteepness);
		return;

	}	
	if (key == GLFW_KEY_DOWN && action == GLFW_RELEASE) {
		cout << "UP pressed" << endl;
		cout << "current steepness:" << " " << AllWaveObj.GlobalSteepness << endl;
		AllWaveObj.GlobalSteepness = AllWaveObj.GlobalSteepness - 0.2;
		cout << "updated steepness:" << " " << AllWaveObj.GlobalSteepness << endl;
		glUniform1f(globalSteepnessID, AllWaveObj.GlobalSteepness);
		return;

	}
	if (key == GLFW_KEY_3 && action == GLFW_RELEASE) {
		cout << "current wave number:" << " " << AllWaveObj.WaveNumber << endl;
		AllWaveObj.WaveNumber = AllWaveObj.WaveNumber + 2;
		if (AllWaveObj.WaveNumber > MAX_WAVE_NUMBER) {
			cout << "Wave Number is maximum; Sorry gotta do nothing. Reduce it noww.........." << endl;
			return;
		}
		glUniform1i(waveNumberID, AllWaveObj.WaveNumber);
		cout << "updated steepness:" << " " << AllWaveObj.WaveNumber << endl;
		return;

	}
	if (key == GLFW_KEY_4 && action == GLFW_RELEASE) {
		cout << "current wavenuber:" << " " << AllWaveObj.WaveNumber << endl;
		AllWaveObj.WaveNumber = AllWaveObj.WaveNumber - 2;
		if (AllWaveObj.WaveNumber <=0) {
			cout << "Wave Number is minimum; Sorry gotta do nothing. Increase it now........." << endl;
			return;
		}
		glUniform1i(waveNumberID, AllWaveObj.WaveNumber);
		cout << "updated wavenumber:" << " " << AllWaveObj.WaveNumber << endl;
		return;

	}

	if (key == GLFW_KEY_5 && action == GLFW_RELEASE) {
		for (int i = 0; i < AllWaveObj.WaveNumber; i++)
		{
			AllWaveObj.waves[i].WaveDir = AllWaveObj.waves[i].WaveDir+vec2(-0.1,0.10);
			cout <<"5: x- y "<< AllWaveObj.waves[i].WaveDir.x <<" "<< AllWaveObj.waves[i].WaveDir.y<< endl;
			SendUniformWaveVars(AllWaveObj);
		}		

	}
	   	
	if (key == GLFW_KEY_6 && action == GLFW_RELEASE) {
		for (int i = 0; i < AllWaveObj.WaveNumber; i++)
		{
			AllWaveObj.waves[i].WaveDir = AllWaveObj.waves[i].WaveDir - vec2(-0.1, 0.10);
			cout << "6: x y reduce- " << AllWaveObj.waves[i].WaveDir.x <<" "<< AllWaveObj.waves[i].WaveDir.y<< endl;
			SendUniformWaveVars(AllWaveObj);
		}

	}

	if (key == GLFW_KEY_7 && action == GLFW_RELEASE) {
		for (int i = 0; i < AllWaveObj.WaveNumber; i++)
		{
			AllWaveObj.waves[i].WaveLength = AllWaveObj.waves[i].WaveLength +1;
			cout << "7 8: wavelen " << AllWaveObj.waves[i].WaveLength<< endl;
			SendUniformWaveVars(AllWaveObj);
		}

	}

	if (key == GLFW_KEY_8 && action == GLFW_RELEASE) {
		for (int i = 0; i < AllWaveObj.WaveNumber; i++)
		{
			AllWaveObj.waves[i].WaveLength = AllWaveObj.waves[i].WaveLength - 1;
			cout << "8: wvelen reduce- " << AllWaveObj.waves[i].WaveLength<< endl;
			SendUniformWaveVars(AllWaveObj);
		}

	}

	if (key == GLFW_KEY_9 && action == GLFW_RELEASE) {
		for (int i = 0; i < AllWaveObj.WaveNumber; i++)
		{
			AllWaveObj.waves[i].Speed = AllWaveObj.waves[i].Speed *2;
			cout << "9 0: speed " << AllWaveObj.waves[i].Speed << endl;
			SendUniformWaveVars(AllWaveObj);
		}

	}

	if (key == GLFW_KEY_0 && action == GLFW_RELEASE) {
		for (int i = 0; i < AllWaveObj.WaveNumber; i++)
		{
			AllWaveObj.waves[i].Speed = AllWaveObj.waves[i].Speed /2;
			cout << "0: speed reduce- " << AllWaveObj.waves[i].Speed << endl;
			SendUniformWaveVars(AllWaveObj);
		}

	}
	
}


void mouseButtonCallback(GLFWwindow* window, int button, int action, int mode) {
	if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
		// Activate rotation mode
		cout << "Mouse Button Pressed" << endl;
		
		static double lastTime = glfwGetTime();
		double currentTime = glfwGetTime();
		float deltaTime = float(currentTime - lastTime);

		double xpos, ypos;
		glfwGetCursorPos(window, &xpos, &ypos);
		//xpos = xpos - 1000 / 2;
		//ypos = ypos - 1000 / 2;
		cout << xpos << " xy pos: " << ypos << endl;

		position=vec3(xpos/100, position.y, ypos/100);

	} if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE) {
		
	}
}

