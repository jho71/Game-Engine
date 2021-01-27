/////////////////////////////////////////////////////////////////////////////////////
//
// This code is used to teach the course "game engine foundations" in Seneca college
// Developed by Alireza Moghaddam on Sep. 2020
//
////////////////////////////////////////////////////////////////////////////////////

using namespace std;

#include "vgl.h"
#include "LoadShaders.h"
#include "glm\glm.hpp"
#include "glm\gtc\matrix_transform.hpp"
#include "glm\gtx\rotate_vector.hpp"
#include "..\SOIL\src\SOIL.h"
#include <iostream>
#include <vector>

enum VAO_IDs { Triangles, NumVAOs };
enum Buffer_IDs { ArrayBuffer };
enum Attrib_IDs { vPosition = 0 };

const GLint NumBuffers = 2;
GLuint VAOs[NumVAOs];
GLuint Buffers[NumBuffers];
GLuint location;
GLuint cam_mat_location;
GLuint proj_mat_location;
GLuint texture[2];    //Array of pointers to textrure data in VRAM. We use two textures in this example.

GLfloat alpha = 0.0f;

const GLuint NumVertices = 28;

//Height of camera (player) from the level
float height = 0.8f;

//Player motion speed for movement and pitch/yaw
float travel_speed = 300.0f;        //Motion speed
float mouse_sensitivity = 0.01f;    //Pitch/Yaw speed

//Used for tracking mouse cursor position on screen
int x0 = 0;
int y_0 = 0;

//Transformation matrices and camera vectors
glm::mat4 model_view;
glm::vec3 unit_z_vector = glm::vec3(0, 0, 1);    //Assigning a meaningful name to (0,0,1) :-)
glm::vec3 cam_pos = glm::vec3(0.0f, 0.0f, height);
glm::vec3 forward_vector = glm::vec3(1, 1, 0);    //Forward vector is parallel to the level at all times (No pitch)

//The direction which the camera is looking, at any instance
glm::vec3 looking_dir_vector = glm::vec3(1, 1, 0);
glm::vec3 up_vector = unit_z_vector;
glm::vec3 side_vector = glm::cross(up_vector, forward_vector);


//Used to measure time between two frames
int oldTimeSinceStart = 0;
int deltaTime;

//Creating and rendering bunch of objects on the scene to interact with
const int Num_Obstacles = 10;
float obstacle_data[Num_Obstacles][3];

class GameObject {
public:
    char type;
    glm::vec3 location;
    glm::vec3 direction;


    GameObject() {
        type = 'a';
        location = glm::vec3(0.0f, 0.0f, height);
        direction = glm::vec3(0.0f, 0.0f, 0.0f);
    }

    GameObject(char s_type, glm::vec3 s_location, glm::vec3 s_direction) {
        type = s_type;
        location = s_location;
        direction = s_direction;
    }
};

class wheel {
public:
    glm::vec3 location;
    glm::vec3 direction;
    float x;
    float y;
    float scene;
    float speed;
    bool display_flag;

    wheel() {
        location[2] = height;
        scene = 1;
        speed = 0.01f;
        display_flag = true;
    }

    wheel(float w_x, float w_y, float w_s, float w_speed, glm::vec3 temp_dir) {
        x = w_x;
        y = w_y;
        location[0] = x;
        location[1] = y;
        location[2] = height;
        scene = w_s;
        speed = w_speed;
        display_flag = true;
        //        direction = glm::normalize(cam_pos - location);
        direction = temp_dir;
    }

};

class enemyObject {
public:
    glm::vec3 location;
    glm::vec3 direction;
    float x;
    float y;
    float scene;
    float speed;
    bool display_flag;

    wheel* wheel_1;
    wheel* wheel_2;
    wheel* wheel_3;
    wheel* wheel_4;

    enemyObject() {
        location[0] = x;
        location[1] = y;
        location[2] = height;
        scene = 1;
        speed = 0.01f;
        display_flag = false;
    }

    enemyObject(float t_x, float t_y, float t_s, float t_speed, glm::vec3 temp_dir) {
        x = t_x;
        y = t_y;
        location[0] = x;
        location[1] = y;
        location[2] = height;
        scene = t_s;
        speed = t_speed;
        display_flag = true;
        //        direction = glm::normalize(cam_pos - location);
        direction = temp_dir;
        wheel_1 = new wheel(x + 2, y + 2, 1, speed, temp_dir);
        wheel_2 = new wheel(x - 2, y + 2, 1, speed, temp_dir);
        wheel_3 = new wheel(x - 2, y - 2, 1, speed, temp_dir);
        wheel_4 = new wheel(x + 2, y - 2, 1, speed, temp_dir);
    }
};

vector <GameObject> gameScene;

vector <enemyObject> enemyScene;

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

    //Randomizing the position and scale of obstacles push it into the vector
    for (int i = 0; i < Num_Obstacles; i++)
    {
        float xx = randomFloat(-50, 50); //X
        float yy = randomFloat(-50, 50); //Y
        float ss = 4; //Scale
        float s_speed = randomFloat(0.0005, 0.001); //enemy speed

        float d_x = randomFloat(-50, 50);
        float d_y = randomFloat(-50, 50);
        glm::vec3 temp_dir = glm::vec3(d_x, d_y, 0.0f);
        enemyObject addEnemy(xx, yy, ss, s_speed, temp_dir);
        enemyScene.push_back(addEnemy);
    }

    ShaderInfo shaders[] = {
        { GL_VERTEX_SHADER, "triangles.vert" },
        { GL_FRAGMENT_SHADER, "triangles.frag" },
        { GL_NONE, NULL }
    };

    GLuint program = LoadShaders(shaders);
    glUseProgram(program);    //My Pipeline is set up


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
        { -0.45, -0.45 ,0.50}
        /*
                { -0.45, -0.45 ,0.01 }, // bottom face
                { 0.45, -0.45 ,0.01 },
                { 0.45, 0.45 ,0.01 },
                { -0.45, 0.45 ,0.01 },

                { -0.45, -0.45 ,0.9 }, //top face
                { 0.45, -0.45 ,0.9 },
                { 0.45, 0.45 ,0.9 },
                { -0.45, 0.45 ,0.9 },

                { 0.45, -0.45 , 0.01 }, //left face
                { 0.45, 0.45 , 0.01 },
                { 0.45, 0.45 ,0.9 },
                { 0.45, -0.45 ,0.9 },

                { -0.45, -0.45, 0.01 }, //right face
                { -0.45, 0.45 , 0.01 },
                { -0.45, 0.45 ,0.9 },
                { -0.45, -0.45 ,0.9 },

                { -0.45, 0.45 , 0.01 }, //front face
                { 0.45, 0.45 , 0.01 },
                { 0.45, 0.45 ,0.9 },
                { -0.45, 0.45 ,0.9 },

                { -0.45, -0.45 , 0.01 }, //back face
                { 0.45, -0.45 , 0.01 },
                { 0.45, -0.45 ,0.9 },
                { -0.45, -0.45 ,0.9 },
                */
    };

    //These are the texture coordinates for the second texture
    GLfloat textureCoordinates[28][2] = {
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
    };




    //Creating our texture:
    //This texture is loaded from file. To do this, we use the SOIL (Simple OpenGL Imaging Library) library.
    //When using the SOIL_load_image() function, make sure the you are using correct patrameters, or else, your image will NOT be loaded properly, or will not be loaded at all.
    GLint width1, height1;
    unsigned char* textureData1 = SOIL_load_image("grass.png", &width1, &height1, 0, SOIL_LOAD_RGB);

    GLint width2, height2;
    unsigned char* textureData2 = SOIL_load_image("ammo.png", &width2, &height2, 0, SOIL_LOAD_RGB);

    glGenBuffers(2, Buffers);
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



    location = glGetUniformLocation(program, "model_matrix");
    cam_mat_location = glGetUniformLocation(program, "camera_matrix");
    proj_mat_location = glGetUniformLocation(program, "projection_matrix");

    ///////////////////////TEXTURE SET UP////////////////////////

    //Allocating two buffers in VRAM
    glGenTextures(2, texture);

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
}

//Helper function to draw a cube
void drawCube(float scale)
{
    model_view = glm::scale(model_view, glm::vec3(scale, scale, scale));
    glUniformMatrix4fv(location, 1, GL_FALSE, &model_view[0][0]);

    //Select the second texture (apple.png) when drawing the second geometry (cube)
    glBindTexture(GL_TEXTURE_2D, texture[1]);
    glDrawArrays(GL_QUADS, 4, 24);
}

//-----------------------------------------------------------------------------------
//-----------------------------------Enemy--------------------------------------------

void updateEnemyScene() {
    for (int i = 0; i < enemyScene.size(); i++) {
        float check_y = enemyScene[i].location.y - cam_pos.y;
        float check_x = enemyScene[i].location.x - cam_pos.x;
        float check_length_distance = sqrt(check_x * check_x + check_y * check_y);
        //       enemyScene[i].direction = glm::normalize(cam_pos - enemyScene[i].location);
        enemyScene[i].location = enemyScene[i].location + enemyScene[i].speed * enemyScene[i].direction;
        if (check_length_distance <= 1) {
            enemyScene[i].display_flag = false;
            break;
        }
        glUniformMatrix4fv(location, 1, GL_FALSE, &model_view[0][0]);
        model_view = glm::mat4(1.0);
    }
}

//Renders level and enemy
void draw_level()
{
    //Select the first texture (grass.png) when drawing the first geometry (floor)
    glBindTexture(GL_TEXTURE_2D, texture[0]);
    glDrawArrays(GL_QUADS, 0, 4);

    //Rendering obstacles obstacles  draw enemy object
    for (int i = 0; i < Num_Obstacles; i++)
    {
        if (enemyScene[i].display_flag) {
            model_view = glm::translate(model_view, glm::vec3(enemyScene[i].location[0], enemyScene[i].location[1], 0.0));
            glUniformMatrix4fv(location, 1, GL_FALSE, &model_view[0][0]);
            drawCube(enemyScene[i].scene);
            model_view = glm::mat4(1.0);
        }
    }
}

//------------------------------------------------------------------------------------------------------
//-------------------------------------Bullet------------------------------------------------------------

// create a box like a bullet
void drawBulletObjects() {
    for (int i = 0; i < gameScene.size(); i++) {
        model_view = glm::translate(model_view, glm::vec3(gameScene[i].location[0], gameScene[i].location[1], 0.8));
        //model_view = glm::translate(model_view, gameScene.back().location);
        glUniformMatrix4fv(location, 1, GL_FALSE, &model_view[0][0]);
        drawCube(0.1);
        model_view = glm::mat4(1.0);
    }
}

// moving the box bullet to the looking direction
void updateBulletScene() {
    for (int i = 0; i < gameScene.size(); i++) {
        gameScene[i].location = gameScene[i].location + 0.01f * gameScene[i].direction;
        glUniformMatrix4fv(location, 1, GL_FALSE, &model_view[0][0]);
        model_view = glm::mat4(1.0);
    }
}

//------------------------------------------------------------------------------------------------------
//-------------------------------------Wheel__1----------------------------------------------------------


void updateWheel_1_Scene() {
    for (int i = 0; i < enemyScene.size(); i++) {
        float check_y = enemyScene[i].wheel_1->location.y - cam_pos.y;
        float check_x = enemyScene[i].wheel_1->location.x - cam_pos.x;
        float check_length_distance = sqrt(check_x * check_x + check_y * check_y);
        //     enemyScene[i].wheel_1->direction = glm::normalize(cam_pos - enemyScene[i].wheel_1->location);
        enemyScene[i].wheel_1->location = enemyScene[i].location + enemyScene[i].wheel_1->speed * enemyScene[i].wheel_1->direction;
        enemyScene[i].wheel_1->location.x = enemyScene[i].wheel_1->location.x + 2;
        enemyScene[i].wheel_1->location.y = enemyScene[i].wheel_1->location.y + 2;
        if (check_length_distance <= 0.5) {
            enemyScene[i].wheel_1->display_flag = false;
            break;
        }
        glUniformMatrix4fv(location, 1, GL_FALSE, &model_view[0][0]);
        model_view = glm::mat4(1.0);
    }
}



// create a box like a wheel
void drawWheel_1() {
    for (int i = 0; i < Num_Obstacles; i++) {
        if (enemyScene[i].display_flag) {
            model_view = glm::translate(model_view, glm::vec3(enemyScene[i].wheel_1->location[0], enemyScene[i].wheel_1->location[1], 0.1));
            model_view = glm::rotate(model_view, alpha, glm::vec3(0, 1, 0));
            glUniformMatrix4fv(location, 1, GL_FALSE, &model_view[0][0]);
            drawCube(enemyScene[i].wheel_1->scene);
            model_view = glm::mat4(1.0);
        }
    }
}
//-------------------------------------Wheel__2----------------------------------------------------------

void updateWheel_2_Scene() {
    for (int i = 0; i < enemyScene.size(); i++) {
        float check_y = enemyScene[i].wheel_2->location.y - cam_pos.y;
        float check_x = enemyScene[i].wheel_2->location.x - cam_pos.x;
        float check_length_distance = sqrt(check_x * check_x + check_y * check_y);
        //      enemyScene[i].wheel_2->direction = glm::normalize(cam_pos - enemyScene[i].wheel_2->location);
        enemyScene[i].wheel_2->location = enemyScene[i].location + enemyScene[i].wheel_2->speed * enemyScene[i].wheel_2->direction;
        enemyScene[i].wheel_2->location.x = enemyScene[i].wheel_2->location.x - 2;
        enemyScene[i].wheel_2->location.y = enemyScene[i].wheel_2->location.y + 2;
        if (check_length_distance <= 0.5) {
            enemyScene[i].wheel_2->display_flag = false;
            break;
        }
        glUniformMatrix4fv(location, 1, GL_FALSE, &model_view[0][0]);
        model_view = glm::mat4(1.0);
    }
}



// create a box like a wheel
void drawWheel_2() {
    for (int i = 0; i < Num_Obstacles; i++) {
        if (enemyScene[i].display_flag) {
            model_view = glm::translate(model_view, glm::vec3(enemyScene[i].wheel_2->location[0], enemyScene[i].wheel_2->location[1], 0.1));
            model_view = glm::rotate(model_view, alpha, glm::vec3(0, 1, 0));
            glUniformMatrix4fv(location, 1, GL_FALSE, &model_view[0][0]);
            drawCube(enemyScene[i].wheel_2->scene);
            model_view = glm::mat4(1.0);
        }
    }
}

//-------------------------------------Wheel__3----------------------------------------------------------

void updateWheel_3_Scene() {
    for (int i = 0; i < enemyScene.size(); i++) {
        float check_y = enemyScene[i].wheel_3->location.y - cam_pos.y;
        float check_x = enemyScene[i].wheel_3->location.x - cam_pos.x;
        float check_length_distance = sqrt(check_x * check_x + check_y * check_y);
        //      enemyScene[i].wheel_3->direction = glm::normalize(cam_pos - enemyScene[i].wheel_3->location);
        enemyScene[i].wheel_3->location = enemyScene[i].location + enemyScene[i].wheel_3->speed * enemyScene[i].wheel_3->direction;
        enemyScene[i].wheel_3->location.x = enemyScene[i].wheel_3->location.x - 2;
        enemyScene[i].wheel_3->location.y = enemyScene[i].wheel_3->location.y - 2;
        if (check_length_distance <= 0.5) {
            enemyScene[i].wheel_3->display_flag = false;
            break;
        }
        glUniformMatrix4fv(location, 1, GL_FALSE, &model_view[0][0]);
        model_view = glm::mat4(1.0);
    }
}



// create a box like a wheel
void drawWheel_3() {
    for (int i = 0; i < Num_Obstacles; i++) {
        if (enemyScene[i].display_flag) {
            model_view = glm::translate(model_view, glm::vec3(enemyScene[i].wheel_3->location[0], enemyScene[i].wheel_3->location[1], 0.1));
            model_view = glm::rotate(model_view, alpha, glm::vec3(0, 1, 0));
            glUniformMatrix4fv(location, 1, GL_FALSE, &model_view[0][0]);
            drawCube(enemyScene[i].wheel_3->scene);
            model_view = glm::mat4(1.0);
        }
    }
}

//-------------------------------------Wheel__4----------------------------------------------------------

void updateWheel_4_Scene() {
    for (int i = 0; i < enemyScene.size(); i++) {
        float check_y = enemyScene[i].wheel_4->location.y - cam_pos.y;
        float check_x = enemyScene[i].wheel_4->location.x - cam_pos.x;
        float check_length_distance = sqrt(check_x * check_x + check_y * check_y);
        //      enemyScene[i].wheel_4->direction = glm::normalize(cam_pos - enemyScene[i].wheel_4->location);
        enemyScene[i].wheel_4->location = enemyScene[i].location + enemyScene[i].wheel_4->speed * enemyScene[i].wheel_4->direction;
        enemyScene[i].wheel_4->location.x = enemyScene[i].wheel_4->location.x + 2;
        enemyScene[i].wheel_4->location.y = enemyScene[i].wheel_4->location.y - 2;
        if (check_length_distance <= 0.5) {
            enemyScene[i].wheel_4->display_flag = false;
            break;
        }
        glUniformMatrix4fv(location, 1, GL_FALSE, &model_view[0][0]);
        model_view = glm::mat4(1.0);
    }
}



// create a box like a wheel
void drawWheel_4() {
    for (int i = 0; i < Num_Obstacles; i++) {
        if (enemyScene[i].display_flag) {
            model_view = glm::translate(model_view, glm::vec3(enemyScene[i].wheel_4->location[0], enemyScene[i].wheel_4->location[1], 0.1));
            model_view = glm::rotate(model_view, alpha, glm::vec3(0, 1, 0));
            glUniformMatrix4fv(location, 1, GL_FALSE, &model_view[0][0]);
            drawCube(enemyScene[i].wheel_4->scene);
            model_view = glm::mat4(1.0);
        }
    }
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

    alpha += 0.5;
    draw_level();
    drawBulletObjects();
    drawWheel_1();
    drawWheel_2();
    drawWheel_3();
    drawWheel_4();

    updateEnemyScene();
    updateBulletScene();
    updateWheel_1_Scene();
    updateWheel_2_Scene();
    updateWheel_3_Scene();
    updateWheel_4_Scene();
    glFlush();
}


void keyboard(unsigned char key, int x, int y)
{
    if (key == 'a')
    {
        //Moving camera along opposit direction of side vector
        cam_pos += side_vector * travel_speed * ((float)deltaTime) / 1000.0f;
    }
    if (key == 'd')
    {
        //Moving camera along side vector
        cam_pos -= side_vector * travel_speed * ((float)deltaTime) / 1000.0f;
    }
    if (key == 'w')
    {
        //Moving camera along forward vector. To be more realistic, we use X=V.T equation in physics
        cam_pos += forward_vector * travel_speed * ((float)deltaTime) / 1000.0f;
    }
    if (key == 's')
    {
        //Moving camera along backward (negative forward) vector. To be more realistic, we use X=V.T equation in physics
        cam_pos -= forward_vector * travel_speed * ((float)deltaTime) / 1000.0f;
    }
    if (key == 'o')
    {
        glm::vec3 temp_cam_pos = cam_pos;
        glm::vec3 shooting_direction = looking_dir_vector;
        GameObject addObject('a', glm::vec3(temp_cam_pos), shooting_direction);
        gameScene.push_back(addObject);
    }

}

//Controlling Pitch with vertical mouse movement
void mouse(int x, int y)
{
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

void idle()
{
    //Calculating the delta time between two frames
    //We will use this delta time when moving forward (in keyboard function)
    int timeSinceStart = glutGet(GLUT_ELAPSED_TIME);
    deltaTime = timeSinceStart - oldTimeSinceStart;
    oldTimeSinceStart = timeSinceStart;
    //cout << timeSinceStart << " " << oldTimeSinceStart << " " << deltaTime << endl;
    glutPostRedisplay();
}

//---------------------------------------------------------------------
//
// main
//
int main(int argc, char** argv)
{
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_RGBA);
    glutInitWindowSize(1024, 1024);
    glutCreateWindow("Camera and Projection");

    glewInit();    //Initializes the glew and prepares the drawing pipeline.

    init();

    glutDisplayFunc(display);

    glutKeyboardFunc(keyboard);

    glutIdleFunc(idle);

    glutPassiveMotionFunc(mouse);

    glutMainLoop();

    for (int i = 0; i < Num_Obstacles; i++) {
        delete  enemyScene[i].wheel_1;
        delete  enemyScene[i].wheel_2;
        delete  enemyScene[i].wheel_3;
        delete  enemyScene[i].wheel_4;
    }

}