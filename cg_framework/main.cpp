#ifdef _WIN32
	#include <GL/glew.h>
#else
	#define GLFW_INCLUDE_GLCOREARB
#endif

#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <string>
#include <vector>

#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <FreeImage.h>
#include "objloader.h"

////////////////////////////////////////////////////////////////////////////////////////

// Loaders
void LoadShader (char* Name, GLuint* ID, GLenum Type) {
	FILE* File = fopen(Name, "r");
	if (!File) return;

	int Size = 0;
    while(!feof(File)) { fgetc(File); ++Size; }
	rewind(File);
    
	char *Content = new char[Size];
	memset(Content, 0, Size);
	fread(Content, sizeof(char), Size, File);

	const char* Source = Content; const char **FILE_Source = &Source;
	*ID = glCreateShader(Type);
	glShaderSource(*ID, sizeof(Content)/sizeof(char*), FILE_Source, NULL);
	glCompileShader(*ID);

	fclose(File);
	delete Content; }

GLuint LoadTexture (char* Name) {
	GLuint ID;

	FIBITMAP *Texture = FreeImage_Load(FIF_JPEG, Name);
	if (Texture) {
		glGenTextures(1, &ID);
		glBindTexture(GL_TEXTURE_2D, ID);

		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, FreeImage_GetWidth(Texture), FreeImage_GetHeight(Texture), 0, GL_BGR, GL_UNSIGNED_BYTE, FreeImage_GetBits(Texture));

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);

		glBindTexture(GL_TEXTURE_2D, 0);

		FreeImage_Unload(Texture);

		return ID; }

	return 0; }

////////////////////////////////////////////////////////////////////////////////////////

// OpenGL
bool Inited = false;

GLuint ID_Program;

// Shader
GLuint ID_Vertex; char* FILE_Vertex = "../shaders/vertex_shader.vs";
GLuint ID_Frag; char* FILE_Frag = "../shaders/frag_shader.fs";

////////////////////////////////////////////////////////////////////////////////////////

// Camera
struct MyCamera {
	glm::vec3 Position;
	glm::vec3 View;
	glm::vec3 Up;

	bool Follow;
	glm::vec3 Velocity;

	glm::mat4 Matrix; };

MyCamera NewCamera (glm::vec3 Position, glm::vec3 View, glm::vec3 Up) {
	MyCamera Camera;

	Camera.Position = Position;
	Camera.View = View;
	Camera.Up = Up;

	Camera.Follow = true;
	Camera.Velocity = glm::vec3(0.005);

	return Camera; }

MyCamera MainCamera = NewCamera(glm::vec3(0, 0, 0), glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));

// Light
struct MyLight {
	glm::vec3 Position;

	char* Type;
	glm::vec3 Angle;
	glm::vec3 Velocity;
	glm::vec4 Ambient;
	glm::vec4 Diffuse;
	glm::vec4 Intensity;

	glm::vec4 Source; };

MyLight NewLight (glm::vec3 Position) {
	MyLight Light;

	Light.Position = Position;

	Light.Type = "Fix";
	Light.Angle = glm::vec3(0);
	Light.Velocity = glm::vec3(0.005);
	Light.Ambient = glm::vec4(0.4, 0.4, 0.4, 1);
	Light.Diffuse = glm::vec4(1);
	Light.Intensity = glm::vec4(0.9, 0.9, 0.9, 1);

	return Light; }

MyLight MainLight = NewLight(glm::vec3(0, -5, 0));

// Window
struct MyWindow {
	int Width;
	int Height;

	float Aspect;
	GLFWwindow* Window;

	glm::mat4 Matrix; };

MyWindow NewWindow (int Width, int Height) {
	MyWindow Window;

	Window.Width = Width;
	Window.Height = Height;

	Window.Aspect = (float) Width / (float) Height;
	Window.Window = 0;

	return Window; }

MyWindow MainWindow = NewWindow(640, 480);

	// Funtions
void UpdateWindow () {
	glViewport(0, 0, MainWindow.Width, MainWindow.Height);
	MainWindow.Matrix = glm::perspective(45.0f, MainWindow.Aspect, 1.0f, 1000.0f); }

void WindowSizeCallback (GLFWwindow* Window, int Width, int Height) { UpdateWindow(); }

////////////////////////////////////////////////////////////////////////////////////////

// Object
OBJLoader OBJCube("../models/Cube.obj");
OBJLoader OBJSphere("../models/Sphere.obj");

struct MyObject {
	char* FILE_Texture;
	char* Type;
	glm::vec3 Position;
	glm::vec3 Size;

	bool Jump;
	char* Direction;
	int Timer;
	glm::vec3 Angle;
	glm::vec3 Pivot;
	glm::vec3 Velocity;

	GLuint VAO;

	GLuint texUVId;
	GLuint ID_Texture;

	GLuint ID_Normals;
	GLuint ID_Vertices;
	GLuint ID_Colors;
	GLuint ID_Indices;

	double MaxJump;

	glm::mat4 Matrix; };

MyObject NewObject (char* FILE_Texture, char* Type, glm::vec3 Position, glm::vec3 Size) {
	MyObject Object;

	Object.FILE_Texture = FILE_Texture;
	Object.Type = Type;
	Object.Position = Position;
	Object.Size = Size;

	Object.Jump = false;
	Object.Direction = "Left";
	Object.Timer = 0;
	Object.Angle = glm::vec3(0);
	Object.Pivot = Size / glm::vec3(2);
	Object.Velocity = glm::vec3(0.005);

	return Object; }

std::vector<MyObject> Cubes;
MyObject Sphere;

	// Funtions
void InitObject () {
	// Cube
	char* FILE_Texture = "../models/textures/White.jpg";

	Cubes.push_back(NewObject(FILE_Texture, "Normal", glm::vec3(0, 0, 0), glm::vec3(3, 1, 1)));

	Cubes.push_back(NewObject(FILE_Texture, "Normal", glm::vec3(4, 1, 0), glm::vec3(1, 1, 1)));
	Cubes.push_back(NewObject(FILE_Texture, "Normal", glm::vec3(6, 3, 0), glm::vec3(1, 1, 1)));
	Cubes.push_back(NewObject(FILE_Texture, "Normal", glm::vec3(4, 5, 0), glm::vec3(1, 1, 1)));

	Cubes.push_back(NewObject(FILE_Texture, "Normal", glm::vec3(0, 6, 0), glm::vec3(3, 1, 1)));
	Cubes.push_back(NewObject(FILE_Texture, "Slider", glm::vec3(0, 6, 0), glm::vec3(3, 1, 1))); // 5 - Slider [-10, 0]
	Cubes.push_back(NewObject(FILE_Texture, "Normal", glm::vec3(-10, 6, 0), glm::vec3(3, 1, 1)));

	Cubes.push_back(NewObject(FILE_Texture, "Elevator", glm::vec3(-12, 6, 0), glm::vec3(1, 1, 1))); // 7 - Elevator [0, 12]

	Cubes.push_back(NewObject(FILE_Texture, "Normal", glm::vec3(-10, 12, 0), glm::vec3(3, 1, 1)));
	Cubes.push_back(NewObject(FILE_Texture, "OTO", glm::vec3(-5, 12, 0), glm::vec3(1, 1, 1))); // 9 - OTO
	Cubes.push_back(NewObject(FILE_Texture, "OTO", glm::vec3(-2, 12, 0), glm::vec3(1, 1, 1))); // 10 - OTO
	Cubes.push_back(NewObject(FILE_Texture, "Normal", glm::vec3(1, 12, 0), glm::vec3(3, 1, 1)));

	// Sphere
	Sphere = NewObject("../models/textures/Bricks.jpg", "Sphere", glm::vec3(0, 3, 0), glm::vec3(0.5)); }

void InitObject (MyObject& MyObject, OBJLoader& OBJLoader) {
	FreeImage_Initialise();
	MyObject.ID_Texture = LoadTexture(MyObject.FILE_Texture);

	const float *vertices = OBJLoader.getVerticesArray();
	const float *textureCoords = OBJLoader.getTextureCoordinatesArray();
	const float *normals = OBJLoader.getNormalsArray();
	const unsigned int *indices = OBJLoader.getIndicesArray();

	glGenVertexArrays(1, &MyObject.VAO);
	glBindVertexArray(MyObject.VAO);

	glEnableVertexAttribArray(0);
	glGenBuffers(1, &MyObject.ID_Vertices);
	glBindBuffer(GL_ARRAY_BUFFER, MyObject.ID_Vertices);
	glBufferData(GL_ARRAY_BUFFER, OBJLoader.getNVertices() * 3 * sizeof(float), vertices, GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

	glEnableVertexAttribArray(1);
	glGenBuffers(1, &MyObject.texUVId);
	glBindBuffer(GL_ARRAY_BUFFER, MyObject.texUVId);
	glBufferData(GL_ARRAY_BUFFER, OBJLoader.getNVertices() * 2 * sizeof(float), textureCoords, GL_STATIC_DRAW);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, 0);

	glEnableVertexAttribArray(2);
	glGenBuffers(1, &MyObject.ID_Normals);
	glBindBuffer(GL_ARRAY_BUFFER, MyObject.ID_Normals);
	glBufferData(GL_ARRAY_BUFFER, OBJLoader.getNVertices() * 3 * sizeof(float), normals, GL_STATIC_DRAW);
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, 0);
    
	glGenBuffers(1, &MyObject.ID_Indices);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, MyObject.ID_Indices);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, OBJLoader.getNIndices() * sizeof(unsigned int), indices, GL_STATIC_DRAW);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0); }

void DrawObject (MyObject& MyObject, OBJLoader& OBJLoader) {
	glBindVertexArray(MyObject.VAO);

	glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, MyObject.ID_Texture);

	GLuint loc = glGetUniformLocation(ID_Program, "mMatrix"); glUniformMatrix4fv(loc, 1, GL_FALSE, (GLfloat *)&MyObject.Matrix); glDrawElements(GL_TRIANGLES, OBJLoader.getNIndices(), GL_UNSIGNED_INT, 0); }

	// Collision
bool CollisionW (MyObject& Object) {
	if (Sphere.Position.y > Object.Position.y) return false;

	// Horizontal
	float AL = Sphere.Position.x - Sphere.Size.x / 4;
	float AR = Sphere.Position.x + Sphere.Size.x / 4;

	float BL = Object.Position.x - Sphere.Size.x;
	float BR = Object.Position.x + Object.Size.x + Sphere.Size.x;

	bool Horizontal = AL >= BL && AR <= BR;

	// Vertical
	float AC = Sphere.Position.y + Sphere.Size.y;
	float BC = Object.Position.y - Object.Size.y / 2;

	bool Vertical = AC >= BC;

	//
	return Horizontal && Vertical; }

bool CollisionA (MyObject& Object) {
	if (Sphere.Position.x < Object.Position.x) return false;

	// Horizontal
	float AC = Sphere.Position.x - Sphere.Size.x;
	float BC = Object.Position.x + Object.Size.x;

	bool  Horizontal = AC <= BC;

	// Vertical
	float AT = Sphere.Position.y + Sphere.Size.y;
	double AB = Sphere.Position.y - Sphere.Size.y + 0.1;

	float BT = Object.Position.y + Object.Size.y;
	float BB = Object.Position.y;

	bool Vertical = AT >= BB && AB <= BT;

	//
	return Horizontal && Vertical; }

bool CollisionS (MyObject& Object) {
	if (Sphere.Position.y < Object.Position.y) return false;

	// Horizontal
	float AL = Sphere.Position.x - Sphere.Size.x / 4;
	float AR = Sphere.Position.x + Sphere.Size.x / 4;

	float BL = Object.Position.x - Sphere.Size.x;
	float BR = Object.Position.x + Object.Size.x + Sphere.Size.x;

	bool Horizontal = AL >= BL && AR <= BR;

	// Vertical
	float AC = Sphere.Position.y - Sphere.Size.y;
	float BC = Object.Position.y + Object.Size.y;

	bool Vertical = AC <= BC;

	//
	if (Horizontal && Vertical) {
		if (strcmp (Object.Type, "Elevator") == 0) Sphere.Position.y = Object.Position.y + Sphere.Size.y + Object.Size.y;//printf("%f %f\n", Sphere.Position.y, Object.Position.y);
		if (strcmp (Object.Type, "OTO") == 0) {
			if (Object.Timer >= 300) Object.Position.y = 100;
			Object.Timer++; }}

	//
	return Horizontal && Vertical; }

bool CollisionD (MyObject& Object) {
	if (Sphere.Position.x > Object.Position.x) return false;

	// Horizontal
	float AC = Sphere.Position.x + Sphere.Size.x;
	float BC = Object.Position.x;

	bool  Horizontal = AC >= BC;

	// Vertical
	float AT = Sphere.Position.y + Sphere.Size.y;
	double AB = Sphere.Position.y - Sphere.Size.y + 0.1;

	float BT = Object.Position.y + Object.Size.y;
	float BB = Object.Position.y;

	bool Vertical = AT >= BB && AB <= BT;

	//
	return Horizontal && Vertical; }

bool Collision (std::string Option) {
	for (unsigned int i=0; i < Cubes.size(); i++) {
		if (Option.find("W") != -1 && CollisionW(Cubes[i])) return true;
		if (Option.find("A") != -1 && CollisionA(Cubes[i])) return true;
		if (Option.find("S") != -1 && CollisionS(Cubes[i])) return true;
		if (Option.find("D") != -1 && CollisionD(Cubes[i])) return true; }

	return false; }

	// Rotation
glm::mat4 RotationX (glm::vec3 Pivot, glm::vec3 Angle) {
	glm::mat4 m1 = glm::translate(glm::mat4(1), Pivot);
	glm::mat4 m2 = glm::rotate(glm::mat4(1), Angle.x, glm::vec3(1, 0, 0));
	glm::mat4 m3 = glm::translate(glm::mat4(1), -Pivot);

	return m1 * m2 * m3; }

glm::mat4 RotationY (glm::vec3 Pivot, glm::vec3 Angle) {
	glm::mat4 m1 = glm::translate(glm::mat4(1), Pivot);
	glm::mat4 m2 = glm::rotate(glm::mat4(1), Angle.y, glm::vec3(0, 1, 0));
	glm::mat4 m3 = glm::translate(glm::mat4(1), -Pivot);

	return m1 * m2 * m3; }

glm::mat4 RotationZ (glm::vec3 Pivot, glm::vec3 Angle) {
	glm::mat4 m1 = glm::translate(glm::mat4(1), Pivot);
	glm::mat4 m2 = glm::rotate(glm::mat4(1), Angle.z, glm::vec3(0, 0, 1));
	glm::mat4 m3 = glm::translate(glm::mat4(1), -Pivot);

	return m1 * m2 * m3; }

////////////////////////////////////////////////////////////////////////////////////////

// Main Loop Update
void UpdateBezier () {}

void UpdateCamera () {
	glm::vec3 Distance = glm::vec3(0, 5, 10);

	if (MainCamera.Follow) MainCamera.Position = Sphere.Position + Distance;

	MainCamera.View = MainCamera.Position - Distance;
	MainCamera.Matrix = glm::lookAt(MainCamera.Position, MainCamera.View, MainCamera.Up); }

void UpdateDraw () {
	if (!Inited) {
		Inited = true;

		#ifdef _WIN32
			glewExperimental = GL_TRUE;
			GLenum err = glewInit();
		#endif

		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glEnable(GL_DEPTH_TEST);

		// Object
		InitObject();

			// Cube
		OBJCube.init();
		for (unsigned int i=0; i < Cubes.size(); i++) InitObject(Cubes[i], OBJCube);

			// Sphere
		OBJSphere.init(); InitObject(Sphere, OBJSphere);

		// Loaders
		LoadShader(FILE_Vertex, &ID_Vertex, GL_VERTEX_SHADER);
		LoadShader(FILE_Frag, &ID_Frag, GL_FRAGMENT_SHADER);

		//
		GLuint *ID = &ID_Program;
		*ID = glCreateProgram();
		glAttachShader(*ID, ID_Vertex);
		glAttachShader(*ID, ID_Frag);
		glLinkProgram (*ID);

		UpdateWindow(); }
    
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glUseProgram(ID_Program);

	//
	GLuint loc;

	loc = glGetUniformLocation(ID_Program, "tex"); glUniform1i(loc, 0);
    
	loc = glGetUniformLocation(ID_Program, "pMatrix"); glUniformMatrix4fv(loc, 1, GL_FALSE, (GLfloat *)&MainWindow.Matrix[0]);
	loc = glGetUniformLocation(ID_Program, "vMatrix"); glUniformMatrix4fv(loc, 1, GL_FALSE, (GLfloat *)&MainCamera.Matrix[0]);
	loc = glGetUniformLocation(ID_Program, "nMatrix"); glUniformMatrix3fv(loc, 1, GL_FALSE, (GLfloat *)&glm::mat3(MainCamera.Matrix)[0]);

	// Light
	loc = glGetUniformLocation(ID_Program, "lightDir"); glUniform3fv(loc, 1, (GLfloat *)&MainLight.Source[0]);
	loc = glGetUniformLocation(ID_Program, "lightIntensity"); glUniform4fv(loc, 1, (GLfloat *)&MainLight.Intensity);
	loc = glGetUniformLocation(ID_Program, "ambientIntensity"); glUniform4fv(loc, 1, (GLfloat *)&MainLight.Ambient);
	loc = glGetUniformLocation(ID_Program, "diffuseColor"); glUniform4fv(loc, 1, (GLfloat *)&MainLight.Diffuse);

	// Object
	for (unsigned int i=0; i < Cubes.size(); i++) DrawObject(Cubes[i], OBJCube);
	DrawObject(Sphere, OBJSphere);

	//
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, 0);
	glBindVertexArray(0);
	glUseProgram(0);

    glFlush(); }

void UpdateKeyboard () {
	if (glfwGetKey(MainWindow.Window, GLFW_KEY_R)) Sphere.Position = glm::vec3(0, 3, 0);

	// Camera
	if (glfwGetKey(MainWindow.Window, GLFW_KEY_W)) { MainCamera.Follow = false; MainCamera.Position.y += MainCamera.Velocity.y; }
	if (glfwGetKey(MainWindow.Window, GLFW_KEY_A)) { MainCamera.Follow = false; MainCamera.Position.x -= MainCamera.Velocity.x; }
	if (glfwGetKey(MainWindow.Window, GLFW_KEY_S)) { MainCamera.Follow = false; MainCamera.Position.y -= MainCamera.Velocity.y; }
	if (glfwGetKey(MainWindow.Window, GLFW_KEY_D)) { MainCamera.Follow = false; MainCamera.Position.x += MainCamera.Velocity.x; }

	if (glfwGetKey(MainWindow.Window, GLFW_KEY_SPACE)) MainCamera.Follow = true;

	// Sphere
	if (glfwGetKey(MainWindow.Window, GLFW_KEY_UP) && Collision("S")) { Sphere.Jump = true; Sphere.MaxJump = Sphere.Position.y + 2.5; }

	if (glfwGetKey(MainWindow.Window, GLFW_KEY_LEFT) && !Collision("A")) { Sphere.Angle.z += Sphere.Velocity.z * 30; Sphere.Position.x -= Sphere.Velocity.x / 3; }
	if (glfwGetKey(MainWindow.Window, GLFW_KEY_RIGHT) && !Collision("D")) { Sphere.Angle.z -= Sphere.Velocity.z * 30; Sphere.Position.x += Sphere.Velocity.x / 3; }}

void UpdateLight () { MainLight.Source = MainCamera.Matrix * glm::vec4(MainLight.Position.x, MainLight.Position.y, MainLight.Position.z, 0); }

void UpdateObject () {
	// Cube
		// Elevator
	if (strcmp (Cubes[7].Direction, "Left") == 0) {
		if (Cubes[7].Position.y <= 0) Cubes[7].Direction = "Right";
		Cubes[7].Position.y -= Cubes[7].Velocity.y / 3; }

	else {
		if (Cubes[7].Position.y >= 12) Cubes[7].Direction = "Left";
		Cubes[7].Position.y += Cubes[7].Velocity.y / 3; }

		// Slider
	if (strcmp (Cubes[5].Direction, "Left") == 0) {
		if (Cubes[5].Position.x <= -10) Cubes[5].Direction = "Right";
		Cubes[5].Position.x -= Cubes[5].Velocity.x / 3; }

	else {
		if (Cubes[5].Position.x >= 0) Cubes[5].Direction = "Left";
		Cubes[5].Position.x += Cubes[5].Velocity.x / 3; }

	for (unsigned int i=0; i < Cubes.size(); i++) Cubes[i].Matrix = glm::translate(glm::mat4(1), Cubes[i].Position) * glm::scale(glm::mat4(1), Cubes[i].Size);

	// Sphere
	Sphere.Pivot = glm::vec3(0);

		// Gravity
	if (Sphere.Jump) {
		Sphere.Position.y += Sphere.Velocity.y / 2;

		if (Sphere.Position.y > Sphere.MaxJump || Collision("WS")) Sphere.Jump = false; }

	else { if (!Collision("S")) Sphere.Position.y -= Sphere.Velocity.y / 2; }

		//
	Sphere.Matrix = glm::translate(glm::mat4(1), Sphere.Position) * RotationZ(Sphere.Pivot, Sphere.Angle) * glm::scale(glm::mat4(1), Sphere.Size); }

void UpdateParticle () {}

// Main
int main (int argc, char const *argv[]) {
	glfwInit();

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	MainWindow.Window = glfwCreateWindow(MainWindow.Width, MainWindow.Height, "CG Framework", NULL, NULL);

	if (!MainWindow.Window) { glfwTerminate(); return -1; }

	glfwMakeContextCurrent(MainWindow.Window);
	glfwSetWindowSizeCallback(MainWindow.Window, WindowSizeCallback);

	while (!glfwGetKey(MainWindow.Window, GLFW_KEY_ESCAPE) && !glfwWindowShouldClose(MainWindow.Window)) {
		UpdateBezier();
		UpdateCamera();
		UpdateDraw();
		UpdateKeyboard();
		UpdateLight();
		UpdateObject();
		UpdateParticle();

		glfwSwapBuffers(MainWindow.Window);
		glfwPollEvents(); }

	glfwTerminate();
	exit(EXIT_SUCCESS); }