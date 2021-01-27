using namespace std;

#include "vgl.h"
#include "LoadShaders.h"
#include "glm\glm.hpp"
#include "glm\gtc\matrix_transform.hpp"
#include "glm\gtx\rotate_vector.hpp"
#include "..\SOIL\src\SOIL.h"
#include <iostream>
#include <vector>
#include <time.h>


enum VAO_IDs { Triangles, NumVAOs };
enum Buffer_IDs { ArrayBuffer };
enum Attrib_IDs { vPosition = 0 };

const GLint NumBuffers = 3;
GLuint VAOs[NumVAOs];
GLuint Buffers[NumBuffers];
GLuint location;
GLuint cam_mat_location;
GLuint proj_mat_location;
GLuint texture[3];	//Array of pointers to textrure data in VRAM. We use two textures in this example.


const GLuint NumVertices = 76;

//Height of camera (player) from the level
float height = 0.8f;

//Player motion speed for movement and pitch/yaw
float travel_speed = 300.0f;		//Motion speed
float mouse_sensitivity = 0.01f;	//Pitch/Yaw speed

//Used for tracking mouse cursor position on screen
int x0 = 0;
int y_0 = 0;

//Transformation matrices and camera vectors
glm::mat4 model_view;
glm::vec3 unit_z_vector = glm::vec3(0, 0, 1);	//Assigning a meaningful name to (0,0,1) :-)
glm::vec3 cam_pos = glm::vec3(0.0f, 0.0f, height);
glm::vec3 forward_vector = glm::vec3(1, 1, 0);	//Forward vector is parallel to the level at all times (No pitch)

//The direction which the camera is looking, at any instance
glm::vec3 looking_dir_vector = glm::vec3(1, 1, 0);
glm::vec3 up_vector = unit_z_vector;
glm::vec3 side_vector = glm::cross(up_vector, forward_vector);


//Used to measure time between two frames
int oldTimeSinceStart = 0;
int oldTime = 0;
int deltaTime;
GLfloat alpha = 0;
clock_t begin_time = clock();
//Creating and rendering bunch of objects on the scene to interact with
const int Num_Obstacles = 50;
float obstacle_data[Num_Obstacles][3];

//Count num od tanks hits
int tanksHit = 0;



//Helper function to generate a random float number within a range
float randomFloat(float a, float b)
{
	float random = ((float)rand()) / (float)RAND_MAX;
	float diff = b - a;
	float r = random * diff;
	return a + r;
}

// inititializing buffers, coordinates, setting up pipeline, etc.
void init(void)
{
	glEnable(GL_DEPTH_TEST);

	//Normalizing all vectors
	up_vector = glm::normalize(up_vector);
	forward_vector = glm::normalize(forward_vector);
	looking_dir_vector = glm::normalize(looking_dir_vector);
	side_vector = glm::normalize(side_vector);

	//Randomizing the position and scale of obstacles
	for (size_t i = 0; i < 28; i++)
	{
		obstacle_data[i][0] = randomFloat(-50, 50); //X
		obstacle_data[i][1] = randomFloat(-50, 50); //Y
		obstacle_data[i][2] = randomFloat(0.1f, 10.0f); //Scale
		float s_speed = randomFloat(0.001f, 0.005f); // Enemy speed

		/*glm::vec3 tmpVec = glm::vec3(obstacle_data[i][0], obstacle_data[i][1], 0);
		SceneNode tmpSceneNode = SceneNode(cam_pos, tmpVec, tmpVec, "cube", obstacle_data[i][2], 0);
		gameScene.push_back(tmpSceneNode);

		glm::vec3 tmpVec2 = glm::vec3(obstacle_data[0][0], obstacle_data[0][1], 0);
		SceneNode tmpSceneNode2 = SceneNode(cam_pos, tmpVec2, tmpVec2, "tank", 5.0f, s_speed);
		gameScene.push_back(tmpSceneNode2);

		glm::vec3 tmpVec1 = glm::vec3(obstacle_data[i][0], obstacle_data[i][1], 0);
		SceneNode tmpSceneNode1 = SceneNode(cam_pos, tmpVec1, tmpVec1, "enemy", randomFloat(0.1f, 5.0f), s_speed);
		gameScene.push_back(tmpSceneNode1);

		*/
	}



	ShaderInfo shaders[] = {
		{ GL_VERTEX_SHADER, "triangles.vert" },
		{ GL_FRAGMENT_SHADER, "triangles.frag" },
		{ GL_NONE, NULL }
	};

	GLuint program = LoadShaders(shaders);
	glUseProgram(program);	//My Pipeline is set up


	//Since we use texture mapping, to simplify the task of texture mapping, 
	//and to clarify the demonstration of texture mapping, we consider 4 vertices per face.
	//Overall, we will have 24 vertices and we have 4 vertices to render the sky (a large square).
	//Therefore, we'll have 28 vertices in total.
	GLfloat vertices[NumVertices][3] = {

		{ -100.0, -100.0, 0.0 }, //Plane to walk on and a sky
		{ 100.0, -100.0, 0.0 },
		{ 100.0, 100.0, 0.0 },
		{ -100.0, 100.0, 0.0 },

		{ -0.45, -0.45 ,-0.50}, // bottom face
		{ 0.45, -0.45 ,-0.50},
		{ 0.45, 0.45 ,-0.50},
		{ -0.45, 0.45 ,-0.50},

		{ -0.45, -0.45 ,0.50}, //top face
		{ 0.45, -0.45 ,0.50},
		{ 0.45, 0.45 ,0.50},
		{ -0.45, 0.45 ,0.50},

		{ 0.45, -0.45 , -0.50}, //left face
		{ 0.45, 0.45 , -0.50},
		{ 0.45, 0.45 ,0.50},
		{ 0.45, -0.45 ,0.50},

		{ -0.45, -0.45, -0.50}, //right face
		{ -0.45, 0.45 , -0.50},
		{ -0.45, 0.45 ,0.50},
		{ -0.45, -0.45 ,0.50},

		{ -0.45, 0.45 , -0.50}, //front face
		{ 0.45, 0.45 , -0.50},
		{ 0.45, 0.45 ,0.50},
		{ -0.45, 0.45 ,0.50},

		{ -0.45, -0.45 , -0.50}, //back face
		{ 0.45, -0.45 , -0.50},
		{ 0.45, -0.45 ,0.50},
		{ -0.45, -0.45 ,0.50},


		//hexagon

		{ 1.00, 1.00, -1.5}, // back face
		{ -1.00, 1.00, -1.5},
		{ -2.00, 1.00, 0},
		{ -1.00, 1.00, 1.5},

		{ 1.00, 1.00, 1.5},
		{ 2.00, 1.00, 0},
		{ -2.00, 1.00, 0},
		{ -1.00, 1.00,1.5},

		{ 2.00, 1.00, 0},
		{ 1.00, 1.00, -1.5},
		{ -1.00, 1.00, -1.5},
		{ -2.00, 1.00, 0},

		{  1 , -1 , -1.5}, //front face
		{ -1 , -1 , -1.5},
		{ -2 , -1 , 0},
		{ -1 , -1 , 1.5},

		{ 2.00, -1.00, 0},
		{ 1.00, -1.00, -1.5},
		{ -1.00, -1.00, -1.5},
		{ -2.00, -1.00, 0},

		{  1 , -1 , 1.5},
		{  2 , -1 , 0},
		{ -2 , -1 , 0},
		{ -1 , -1 , 1.5},

		{ 1 , 1 , -1.5}, //bottom
		{  -1 , 1 , -1.5},
		{ -1 , -1 , -1.5},
		{  1 , -1 , -1.5},

		{1 , 1 , -1.5},
		{1 , -1 , -1.5},
		{2 , -1 , 0},
		{  2 , 1 , 0},

		{2 , 1 , 0},
		{ 2 , -1 , 0},
		{  1 , -1 , 1.5},
		{ 1 , 1 , 1.5},

		{ 1 , 1 , 1.5},
		{  1 , -1 , 1.5},
		{ -1 , -1 , 1.5},
		{ -1 , 1 , 1.5},

		{-1 , 1 , 1.5},
		{  -1 , -1 , 1.5},
		{ -2 , -1 , 0},
		{ -2 , 1 , 0},

		{-2 , 1 , 0},
		{ -2 , -1 , 0},
		{-1 , -1 , -1.5},
		{ -1 , 1 , -1.5}

	};

	//These are the texture coordinates for the second texture
	GLfloat textureCoordinates[NumVertices][2] = {
		0.0f, 0.0f,
		200.0f, 0.0f,
		200.0f, 200.0f,
		0.0f, 200.0f,

		0.0f, 1.0f,
		1.0f, 1.0f,
		1.0f, 0.0f,
		0.0f, 0.0f,

		0.0f, 1.0f,
		1.0f, 1.0f,
		1.0f, 0.0f,
		0.0f, 0.0f,

		0.0f, 1.0f,
		1.0f, 1.0f,
		1.0f, 0.0f,
		0.0f, 0.0f,

		0.0f, 1.0f,
		1.0f, 1.0f,
		1.0f, 0.0f,
		0.0f, 0.0f,

		0.0f, 1.0f,
		1.0f, 1.0f,
		1.0f, 0.0f,
		0.0f, 0.0f,

		0.0f, 1.0f,
		1.0f, 1.0f,
		1.0f, 0.0f,
		0.0f, 0.0f,

		0.0f, 1.0f,
		1.0f, 1.0f,
		1.0f, 0.0f,
		0.0f, 0.0f,

		0.0f, 1.0f,
		1.0f, 1.0f,
		1.0f, 0.0f,
		0.0f, 0.0f,

		0.0f, 1.0f,
		1.0f, 1.0f,
		1.0f, 0.0f,
		0.0f, 0.0f,

		0.0f, 1.0f,
		1.0f, 1.0f,
		1.0f, 0.0f,
		0.0f, 0.0f,

		0.0f, 1.0f,
		1.0f, 1.0f,
		1.0f, 0.0f,
		0.0f, 0.0f,

		0.0f, 1.0f,
		1.0f, 1.0f,
		1.0f, 0.0f,
		0.0f, 0.0f,

		0.0f, 1.0f,
		1.0f, 1.0f,
		1.0f, 0.0f,
		0.0f, 0.0f,

		0.0f, 1.0f,
		1.0f, 1.0f,
		1.0f, 0.0f,
		0.0f, 0.0f,

		0.0f, 1.0f,
		1.0f, 1.0f,
		1.0f, 0.0f,
		0.0f, 0.0f,

		0.0f, 1.0f,
		1.0f, 1.0f,
		1.0f, 0.0f,
		0.0f, 0.0f,

		0.0f, 1.0f,
		1.0f, 1.0f,
		1.0f, 0.0f,
		0.0f, 0.0f,

		0.0f, 1.0f,
		1.0f, 1.0f,
		1.0f, 0.0f,
		0.0f, 0.0f,

	};


	//Creating our texture:
	//This texture is loaded from file. To do this, we use the SOIL (Simple OpenGL Imaging Library) library.
	//When using the SOIL_load_image() function, make sure the you are using correct patrameters, or else, your image will NOT be loaded properly, or will not be loaded at all.
	GLint width1, height1;
	unsigned char* textureData1 = SOIL_load_image("grass.png", &width1, &height1, 0, SOIL_LOAD_RGB);

	GLint width2, height2;
	unsigned char* textureData2 = SOIL_load_image("ammo.png", &width2, &height2, 0, SOIL_LOAD_RGB);

	GLint width3, height3;
	unsigned char* textureData3 = SOIL_load_image("rubber.png", &width3, &height3, 0, SOIL_LOAD_RGB);

	glGenBuffers(3, Buffers);
	glBindBuffer(GL_ARRAY_BUFFER, Buffers[0]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
	glBindAttribLocation(program, 0, "vPosition");
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0));
	glEnableVertexAttribArray(0);

	glBindBuffer(GL_ARRAY_BUFFER, Buffers[1]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(textureCoordinates), textureCoordinates, GL_STATIC_DRAW);
	glBindAttribLocation(program, 1, "vTexCoord");
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0));
	glEnableVertexAttribArray(1);

	glBindBuffer(GL_ARRAY_BUFFER, Buffers[2]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(textureCoordinates), textureCoordinates, GL_STATIC_DRAW);
	glBindAttribLocation(program, 1, "vTexCoord");
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0));
	glEnableVertexAttribArray(1);

	location = glGetUniformLocation(program, "model_matrix");
	cam_mat_location = glGetUniformLocation(program, "camera_matrix");
	proj_mat_location = glGetUniformLocation(program, "projection_matrix");

	///////////////////////TEXTURE SET UP////////////////////////

	//Allocating two buffers in VRAM
	glGenTextures(3, texture);

	//First Texture: 

	//Set the type of the allocated buffer as "TEXTURE_2D"
	glBindTexture(GL_TEXTURE_2D, texture[0]);

	//Loading the second texture into the second allocated buffer:
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width1, height1, 0, GL_RGB, GL_UNSIGNED_BYTE, textureData1);

	//Setting up parameters for the texture that recently pushed into VRAM
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);


	//And now, second texture: 

	//Set the type of the allocated buffer as "TEXTURE_2D"
	glBindTexture(GL_TEXTURE_2D, texture[1]);

	//Loading the second texture into the second allocated buffer:
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width2, height2, 0, GL_RGB, GL_UNSIGNED_BYTE, textureData2);

	//Setting up parameters for the texture that recently pushed into VRAM
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	//////////////////////////////////////////////////////////////
	//And now, third texture: 

	//Set the type of the allocated buffer as "TEXTURE_2D"
	glBindTexture(GL_TEXTURE_2D, texture[2]);

	//Loading the second texture into the second allocated buffer:
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width3, height3, 0, GL_RGB, GL_UNSIGNED_BYTE, textureData3);

	//Setting up parameters for the texture that recently pushed into VRAM
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	//////////////////////////////////////////////////////////////
	//gameScene.push_back(std::make_unique<Tank>());
}

class SceneNode {
public:
	std::string type;
	glm::vec3 location__;
	glm::vec3 direction;
	float scale;
	float speed;
	vector<std::unique_ptr<SceneNode>> children;

	glm::vec3 transf;
	glm::vec3 side_vector;
	glm::vec3 up_vector;
	bool timeStarted;
	bool isAlive;
	clock_t bulletTimer;

	SceneNode() {

		type = "cube";
		scale = 0.0f;
		speed = 0.0f;
		direction = glm::vec3(1, 0, 0);
		up_vector = glm::vec3(0, 0, 1);
		side_vector = glm::cross(up_vector, direction);
		location__ = glm::vec3(0, 0, 0);
		transf = glm::vec3(0, 0, 0);
		isAlive = true;


	}

	SceneNode(glm::vec3 location_, glm::vec3 direction_, std::string type_, float scale_, float speed_) {
		location__ = location_;
		type = type_;
		scale = scale_;
		speed = speed_;
		direction = direction_;
		isAlive = true;
		bulletTimer = 0;
		timeStarted = false;
	}

	virtual void draw() {

		model_view = glm::scale(model_view, glm::vec3(scale, scale, scale));
		model_view = glm::translate(model_view, location__ + transf);

		glUniformMatrix4fv(location, 1, GL_FALSE, &model_view[0][0]);

		glBindTexture(GL_TEXTURE_2D, texture[1]);
		glDrawArrays(GL_QUADS, 4, 24);

		model_view = glm::mat4(1.0);
	}

	void render() {

		glm::mat4 back_up_model_view = model_view;

		draw();

		for (size_t i = 0; i < children.size(); i++) {
			children.at(i)->render();
		}

		model_view = back_up_model_view;
	}
};

class Wheel : public SceneNode {
public:

	Wheel() {
		this->type = "wheel";
		this->scale = 0.3f;
	}

	void draw() {

		side_vector = glm::cross(up_vector, direction);
		glm::mat4 back_up_model_view = model_view;

		model_view = glm::scale(model_view, glm::vec3(scale, scale, scale));
	
		model_view = glm::translate(model_view, (location__ * 6.67f) + this->transf);
		model_view = glm::rotate(model_view, atan(location__.y / location__.x), unit_z_vector);
		model_view = glm::rotate(model_view, alpha / 1000, glm::vec3(0, 1, 0));
		
		glUniformMatrix4fv(location, 1, GL_FALSE, &model_view[0][0]);

		glBindTexture(GL_TEXTURE_2D, texture[2]);
		glDrawArrays(GL_QUADS, 28, 48);

		model_view = back_up_model_view;
		glUniformMatrix4fv(location, 1, GL_FALSE, &model_view[0][0]);
	}

};

class Tank : public SceneNode {
public:

	int numOfWheels = 4;

	Tank() {
		this->scale = 2.0f;
		this->type = "tank";
		this->speed = randomFloat(0.0001f, .01f);
		this->location__ = glm::vec3(randomFloat(-20, 20), randomFloat(-20, 20), 0);
		//this->direction = glm::vec3(randomFloat(-1.0f, 1.0f), randomFloat(-1.0f, 1.0f), 0);
		//this->location__ = glm::vec3(0, 0, 0);
		//this->direction = glm::vec3(0, 0, 0);

		for (size_t i = 0; i < numOfWheels; i++) {
			children.push_back(std::make_unique<Wheel>());// Create four new wheels for the tank body
		}

		children.at(0)->transf = glm::vec3(-3, 3, 1);// Place wheels in the right pos
		children.at(1)->transf = glm::vec3(3, 3, 1);
		children.at(2)->transf = glm::vec3(-3, -3, 1);
		children.at(3)->transf = glm::vec3(3, -3, 1);

		transf = glm::vec3(0, 0, .5); //set tank body to above the ground

	}
	void updateWheels() {

		for (size_t i = 0; i < numOfWheels; i++) {
			children.at(i)->speed = speed;
			children.at(i)->direction = direction;
			children.at(i)->location__ = location__;

		}

	}
	void draw() {

		updateWheels();

		glm::mat4 back_up_model_view = model_view;
		//std::cout << "Tank speed: " << speed << ", Tank location: x: " << location__.x+ transf.x << ", y: " << location__.y+transf.y << ", Tank direction: (x: " << direction.x << " y:)" << direction.y << std::endl;
		model_view = glm::scale(model_view, glm::vec3(scale, scale, scale));
		model_view = glm::translate(model_view, location__ + transf);
		model_view = glm::rotate(model_view, atan(location__.y / location__.x), unit_z_vector);
 
		glUniformMatrix4fv(location, 1, GL_FALSE, &model_view[0][0]);


		glBindTexture(GL_TEXTURE_2D, texture[1]);
		glDrawArrays(GL_QUADS, 4, 24);

		model_view = back_up_model_view;

		glUniformMatrix4fv(location, 1, GL_FALSE, &model_view[0][0]);
	}

};


vector<std::unique_ptr<SceneNode>> gameScene;


//Helper function to draw a cube
void drawCube(float scale, std::string type_)
{
	model_view = glm::scale(model_view, glm::vec3(scale, scale, scale));
	//model_view = glm::rotate(model_view, 1.0f, looking_dir_vector);
	glUniformMatrix4fv(location, 1, GL_FALSE, &model_view[0][0]);

	if (type_ == "enemy") {
		glBindTexture(GL_TEXTURE_2D, texture[2]);
		glDrawArrays(GL_QUADS, 4, 24);
	}
	else if (type_ == "bullet") {
		glBindTexture(GL_TEXTURE_2D, texture[1]);
		glDrawArrays(GL_QUADS, 4, 24);
	}

}

void drawObjects() {

	for (size_t i = 0; i < gameScene.size(); ++i) {


		model_view = glm::translate(model_view, glm::vec3(gameScene.at(i)->location__.x, gameScene.at(i)->location__.y, 0));

		glUniformMatrix4fv(location, 1, GL_FALSE, &model_view[0][0]);
		drawCube(gameScene.at(i)->scale, gameScene.at(i)->type);

		model_view = glm::mat4(1.0);
	}
}

void updateObj() {
	size_t sz = gameScene.size();
	bool breakAllLoops = false;

	for (size_t i = 0; i < sz; ++i) {

		if (gameScene.at(i)->type == "tank") {
			if (gameScene.at(i)->isAlive == false) {
				gameScene.erase(gameScene.begin() + i);
				glUniformMatrix4fv(location, 1, GL_FALSE, &model_view[0][0]);
				model_view = glm::mat4(1.0);
				break;
			}
			else {

				gameScene.at(i)->location__ = gameScene.at(i)->location__ + gameScene.at(i)->speed * gameScene.at(i)->direction;
				glm::normalize(gameScene.at(i)->location__);

				gameScene.at(i)->render();

				gameScene.at(i)->direction = glm::vec3(0, 0, 0) - gameScene.at(i)->location__;
				glm::normalize(gameScene.at(i)->direction);

				float dx = gameScene.at(i)->location__.x - cam_pos.x;
				float dy = gameScene.at(i)->location__.y - cam_pos.y;
				float distance = sqrt(dx * dx + dy * dy);

				float dx1 = gameScene.at(i)->location__.x;
				float dy1 = gameScene.at(i)->location__.y;
				float distance1 = sqrt(dx1 * dx1 + dy1 * dy1);

				if (distance <= 1) { // touched player so end game

					tanksHit = 10;
					glUniformMatrix4fv(location, 1, GL_FALSE, &model_view[0][0]);
					model_view = glm::mat4(1.0);
					break;

				}
				if (distance1 <= 1) { // reached center so destroy tank

					gameScene.erase(gameScene.begin() + i);
					glUniformMatrix4fv(location, 1, GL_FALSE, &model_view[0][0]);
					model_view = glm::mat4(1.0);
					break;

				}
				sz = gameScene.size();
				for (size_t j = 0; j < sz; j++) {

					if (gameScene.at(j)->type == "tank" && i != j) {

						float dx2 = gameScene.at(i)->location__.x - gameScene.at(j)->location__.x;
						float dy2 = gameScene.at(i)->location__.y - gameScene.at(j)->location__.y;

						float distance2 = sqrt(dx2 * dx2 + dy2 * dy2);

						if (distance2 <= 2) { // tank collision with another tank delete both tanks

							gameScene.erase(gameScene.begin() + i);
							gameScene.at(j)->isAlive = false;
							cout << "tank collision with another tank" << endl;
							breakAllLoops = true;
							break;

						}

					}
				}
			}
		}
		else if (gameScene.at(i)->type == "bullet") {

			if ((gameScene.at(i)->isAlive == false || clock() - gameScene.at(i)->bulletTimer >= 2000.0f) && gameScene.at(i)->timeStarted == true) {

				gameScene.erase(gameScene.begin() + i);
				glUniformMatrix4fv(location, 1, GL_FALSE, &model_view[0][0]);
				model_view = glm::mat4(1.0);
				break;
			}
			else {
				if (gameScene.at(i)->timeStarted == false) {
					gameScene.at(i)->bulletTimer = clock();
					gameScene.at(i)->timeStarted = true;
				}
				gameScene.at(i)->location__ = gameScene.at(i)->location__ + gameScene.at(i)->speed * gameScene.at(i)->direction;

				sz = gameScene.size();
				for (size_t j = 0; j < sz; j++) {

					if (gameScene.at(j)->type == "tank" && i != j) {

						float dx = gameScene.at(i)->location__.x - gameScene.at(j)->location__.x;
						float dy = gameScene.at(i)->location__.y - gameScene.at(j)->location__.y;
						float distance = sqrt(dx * dx + dy * dy);

						if (distance <= 1.5) {

							
							gameScene.at(i)->isAlive = false;
							gameScene.erase(gameScene.begin() + j);
							tanksHit++;
							cout << "bullet hits tank" << endl;
							breakAllLoops = true;
							break;

						}
					}
				}
			}
		}
		glUniformMatrix4fv(location, 1, GL_FALSE, &model_view[0][0]);
		model_view = glm::mat4(1.0);

		if (breakAllLoops == true) {
			break;
		}
	}
}
//Renders level
void draw_level()
{
	drawObjects();
	updateObj();
	//Select the first texture (grass.png) when drawing the first geometry (floor)
	glBindTexture(GL_TEXTURE_2D, texture[0]);
	glDrawArrays(GL_QUADS, 0, 4);


	//Rendering obstacles obstacles
	/*for (size_t i = 0; i < gameScene.size(); i++)
	{
		model_view = glm::translate(model_view, glm::vec3(obstacle_data[i][0], obstacle_data[i][1], 0.0));
		glUniformMatrix4fv(location, 1, GL_FALSE, &model_view[0][0]);
		drawCube(gameScene.at(i)->scale, gameScene.at(i)->type);
		model_view = glm::mat4(1.0);

	}
	*/
}

//---------------------------------------------------------------------
//
// display
//
void display(void)
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	model_view = glm::mat4(1.0);
	glUniformMatrix4fv(location, 1, GL_FALSE, &model_view[0][0]);

	//The 3D point in space that the camera is looking
	glm::vec3 look_at = cam_pos + looking_dir_vector;

	glm::mat4 camera_matrix = glm::lookAt(cam_pos, look_at, up_vector);
	glUniformMatrix4fv(cam_mat_location, 1, GL_FALSE, &camera_matrix[0][0]);

	glm::mat4 proj_matrix = glm::frustum(-0.01f, +0.01f, -0.01f, +0.01f, 0.01f, 100.0f);
	glUniformMatrix4fv(proj_mat_location, 1, GL_FALSE, &proj_matrix[0][0]);

	draw_level();

	glFlush();
}


void keyboard(unsigned char key, int x, int y)
{
	if (tanksHit < 10) {
		if (key == 'a')
		{
			//Moving camera along opposit direction of side vector
			cam_pos += side_vector * travel_speed * ((float)deltaTime) / 10000.0f;
		}
		if (key == 'd')
		{
			//Moving camera along side vector
			cam_pos -= side_vector * travel_speed * ((float)deltaTime) / 10000.0f;
		}
		if (key == 'w')
		{
			//Moving camera along forward vector. To be more realistic, we use X=V.T equation in physics
			cam_pos += forward_vector * travel_speed * ((float)deltaTime) / 10000.0f;
		}
		if (key == 's')
		{
			//Moving camera along backward (negative forward) vector. To be more realistic, we use X=V.T equation in physics
			cam_pos -= forward_vector * travel_speed * ((float)deltaTime) / 10000.0f;
		}
		if (key == 'f')
		{
			gameScene.push_back(std::make_unique<SceneNode>(cam_pos, looking_dir_vector, "bullet", 0.1f, 0.1f));

		}
	}
}

//Controlling Pitch with vertical mouse movement
void mouse(int x, int y)
{
	if (tanksHit < 10) {
		//Controlling Yaw with horizontal mouse movement
		int delta_x = x - x0;

		//The following vectors must get updated during a yaw movement
		forward_vector = glm::rotate(forward_vector, -delta_x * mouse_sensitivity, unit_z_vector);
		looking_dir_vector = glm::rotate(looking_dir_vector, -delta_x * mouse_sensitivity, unit_z_vector);
		side_vector = glm::rotate(side_vector, -delta_x * mouse_sensitivity, unit_z_vector);
		up_vector = glm::rotate(up_vector, -delta_x * mouse_sensitivity, unit_z_vector);
		x0 = x;

		//The following vectors must get updated during a pitch movement
		int delta_y = y - y_0;
		glm::vec3 tmp_up_vec = glm::rotate(up_vector, delta_y * mouse_sensitivity, side_vector);
		glm::vec3 tmp_looking_dir = glm::rotate(looking_dir_vector, delta_y * mouse_sensitivity, side_vector);

		//The dot product is used to prevent the user from over-pitch (pitching 360 degrees)
		//The dot product is equal to cos(theta), where theta is the angle between looking_dir and forward vector
		GLfloat dot_product = glm::dot(tmp_looking_dir, forward_vector);

		//If the angle between looking_dir and forward vector is between (-90 and 90) degress 
		if (dot_product > 0)
		{
			up_vector = glm::rotate(up_vector, delta_y * mouse_sensitivity, side_vector);
			looking_dir_vector = glm::rotate(looking_dir_vector, delta_y * mouse_sensitivity, side_vector);
		}
		y_0 = y;
	}
}

void idle()
{
	float omega = 1;//MidTerm code Added

		//Calculating the delta time between two frames
		//We will use this delta time when moving forward (in keyboard function)
	int timeSinceStart = glutGet(GLUT_ELAPSED_TIME);
	alpha = omega * timeSinceStart;//MidTerm code Added
	deltaTime = timeSinceStart - oldTimeSinceStart;
	oldTimeSinceStart = timeSinceStart;

	//cout << timeSinceStart << " " << oldTimeSinceStart << " " << deltaTime << endl;
	glutPostRedisplay();



	if (clock() - begin_time > 2000.0f) {
		//std::cout << float(clock() - begin_time) << endl;
		begin_time = clock();
		gameScene.push_back(std::make_unique<Tank>());
	}
}

//---------------------------------------------------------------------
//
// main
//
int main(int argc, char** argv)
{
	begin_time = clock();


	//std::string tmp  = "targeter";
	//gameScene.push_back(std::make_unique<Tank>(tmp));

	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_RGBA);
	glutInitWindowSize(1024, 1024);
	glutCreateWindow("Camera and Projection");

	glewInit();	//Initializes the glew and prepares the drawing pipeline.

	init();

	glutDisplayFunc(display);

	glutKeyboardFunc(keyboard);

	glutIdleFunc(idle);

	glutPassiveMotionFunc(mouse);

	glutMainLoop();



}
