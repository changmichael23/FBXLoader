#include <iostream>

#define GLEW_STATIC

#include <GL/glew.h>

#include <GLFW/glfw3.h>

#include <glm/vec3.hpp> // glm::vec3
#include <glm/vec4.hpp> // glm::vec4
#include <glm/mat4x4.hpp> // glm::mat4
#include <glm/gtc/matrix_transform.hpp> // glm::translate, glm::rotate, glm::scale, glm::perspective
#include <glm/gtc/type_ptr.hpp> // glm::value_ptr
#include "EsgiShader.h"

#include "FBXLoader.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

FBXLoader* testFBX;

const GLint WIDTH = 1920, HEIGHT = 1080;

EsgiShader g_BasicShader;
EsgiShader g_ComputeShader;
GLFWwindow *window;

// --------------
#define XMIN -1
#define XMAX 1

#define YMIN 5
#define YMAX 8

#define ZMIN -1
#define ZMAX 1

#define VXMIN -0.0001
#define VXMAX 0.0001

#define VYMIN -0.0001
#define VYMAX 0.0001

#define VZMIN -0.0001
#define VZMAX 0.0001


#define NUM_PARTICLES 1024 * 1024
// total number of particles to move
#define WORK_GROUP_SIZE128
// # work-items per work-group
struct pos
{
	float x, y, z, w;
	// positions
};
struct vel
{
	float vx, vy, vz, vw;    // velocities
};
struct color
{
	float r, g, b, a;
	// colors
};
// need to do the following for both position, velocity, and colors of the particles :
GLuint posSSbo;
GLuint velSSbo;
GLuint colSSbo;





struct ViewProj
{
	glm::mat4 viewMatrix;
	glm::mat4 projectionMatrix;
} g_Camera;

struct Object
{
	glm::mat4 worldMatrix;
} g_Objet;

GLuint VBO_position, VBO_normal, VBO_texture, VAO, IBO;
GLuint tex0,tex1,tex2;

#define TOP	2147483647.		// 2^31 - 1	

float
Ranf(float low, float high)
{
	long random();		// returns integer 0 - TOP

	float r = (float)rand();
	return(low + r * (high - low) / (float)RAND_MAX);
}

bool LoadAndCreateTextureRGBA(const char *filename, GLuint &texID, bool linear = false)
{
	glGenTextures(1, &texID);
	glBindTexture(GL_TEXTURE_2D, texID);

	// il est obligatoire de specifier une valeur pour GL_TEXTURE_MIN_FILTER
	// autrement le Texture Object est considere comme invalide
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	int w, h;
	uint8_t *data = stbi_load(filename, &w, &h, nullptr, STBI_rgb_alpha);
	if (data) {
		glTexImage2D(GL_TEXTURE_2D, 0, linear ? GL_RGBA8 :GL_SRGB8_ALPHA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);

		stbi_image_free(data);
	}
	return (data != nullptr);
}
struct pos *points;
bool Initialise()
{
	testFBX = new FBXLoader(VBO_position,IBO,VAO);
	testFBX->LoadFBX();

	g_BasicShader.LoadVertexShader("../src/shaders/basicParticle.vs");
	g_BasicShader.LoadFragmentShader("../src/shaders/basicParticle.fs");
	g_ComputeShader.LoadComputeShader("../src/shaders/basicParticle.cs");
	g_BasicShader.Create();
	g_ComputeShader.Create();

	
	// load texture

	LoadAndCreateTextureRGBA("../samples/ironman/ironman.fbm/ironman.dff.png", tex0);
	LoadAndCreateTextureRGBA("../samples/ironman/ironman.fbm/ironman.spec.png", tex1);
	LoadAndCreateTextureRGBA("../samples/ironman/ironman.fbm/ironman.norm.png", tex2, true);

	static const float triangle[] = {
		-0.5f, -0.5f,
		0.5f, -0.5f,
		0.0f, 0.5f
	};

	glGenVertexArrays(1, &VAO);
	/*glGenBuffers(1, &VBO_position);
	glGenBuffers(1, &VBO_normal);
	glGenBuffers(1, &VBO_texture);*/
	
	//glBindVertexArray(VAO);

	//glBindBuffer(GL_ARRAY_BUFFER, VBO_position);
	//glBufferData(GL_ARRAY_BUFFER, testFBX->nb_vertices * 4 * sizeof(float), testFBX->vertex_pos, GL_STATIC_DRAW);
	//glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, (GLvoid *)0);
	//glEnableVertexAttribArray(0);

	//glBindBuffer(GL_ARRAY_BUFFER, VBO_normal);
	//glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * testFBX->nb_normals * 3, testFBX->vertex_nor, GL_STATIC_DRAW);
	//glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, (GLvoid *)0);
	//glEnableVertexAttribArray(1);

	//glBindBuffer(GL_ARRAY_BUFFER, VBO_texture);
	//glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * testFBX->nb_uvs * 2, testFBX->vertex_uv, GL_STATIC_DRAW);
	//glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, (GLvoid *)0);
	//glEnableVertexAttribArray(2);

	//--
	
	glGenBuffers(1, &posSSbo);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, posSSbo);
	glBufferData(GL_SHADER_STORAGE_BUFFER, NUM_PARTICLES * sizeof(
			struct pos), NULL, GL_STATIC_DRAW);
	GLint bufMask = GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT;
	// the invalidate makes a big difference when re-writing
	points = (struct pos *) glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, NUM_PARTICLES * sizeof(struct pos), bufMask);
	for (int i = 0; i < NUM_PARTICLES; i++)
	{
		points[i].x = Ranf(XMIN, XMAX);
		points[i].y = Ranf(YMIN, YMAX);
		points[i].z = Ranf(ZMIN, ZMAX);
		points[i].w = 1.;
	}
	glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, posSSbo);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

	glGenBuffers(1, &velSSbo);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, velSSbo);
	glBufferData(GL_SHADER_STORAGE_BUFFER, NUM_PARTICLES * sizeof(
			struct vel), NULL, GL_STATIC_DRAW);
	struct vel *vels = (struct vel *) glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, NUM_PARTICLES * sizeof(struct vel), bufMask);
	for (int i = 0; i < NUM_PARTICLES; i++)
	{
		vels[i].vx = Ranf(VXMIN, VXMAX);
		vels[i].vy = Ranf(VYMIN, VYMAX);
		vels[i].vz = Ranf(VZMIN, VZMAX);
		vels[i].vw = 0.;
	}
	glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, velSSbo);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

	glGenBuffers(1, &colSSbo);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, colSSbo);
	glBufferData(GL_SHADER_STORAGE_BUFFER, NUM_PARTICLES * sizeof(
		struct color), NULL, GL_STATIC_DRAW);
	struct color *colors = (struct color *) glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, NUM_PARTICLES * sizeof(struct color), bufMask);
	for (int i = 0; i < NUM_PARTICLES; i++)
	{
		colors[i].r = Ranf(.3f, 1.);
		colors[i].g = Ranf(.3f, 1.);
		colors[i].b = Ranf(.3f, 1.);
		colors[i].a = 1.;
	}
	glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 6, colSSbo);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

	glBindVertexArray(VAO);
	glBindBuffer(GL_ARRAY_BUFFER, posSSbo);
	glBufferData(GL_ARRAY_BUFFER, NUM_PARTICLES * 4 * sizeof(float), points, GL_STATIC_DRAW);
	glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, (GLvoid *)0);
	glEnableVertexAttribArray(0);

	glBindBuffer(GL_ARRAY_BUFFER, colSSbo);
	glBufferData(GL_ARRAY_BUFFER, NUM_PARTICLES * 4 * sizeof(float), colors, GL_STATIC_DRAW);
	glVertexAttribPointer(6, 4, GL_FLOAT, GL_FALSE, 0, (GLvoid *)0);
	glEnableVertexAttribArray(6);

	glBindVertexArray(0);
	

	return true;
}

void Terminate() {
	g_BasicShader.Destroy();
	g_ComputeShader.Destroy();
	glDeleteBuffers(1, &VBO_position);
	glDeleteBuffers(1, &VBO_normal);
	glDeleteBuffers(1, &VBO_texture);
	glDeleteBuffers(1, &tex0);
	glDeleteBuffers(1, &tex1);
	glDeleteBuffers(1, &tex2);
}

void Resize(GLint width, GLint height) {
	glViewport(0, 0, width, height);
}

void Render()
{
	glfwPollEvents();
	// indique que couleur sont en lineaire (Gamma = 1.0)
	glEnable(GL_DEPTH_TEST);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glEnable(GL_FRAMEBUFFER_SRGB);
	glClearColor(0.5f, 0.5f, 0.5f, 1.f);

	
	glMemoryBarrier(GL_ALL_BARRIER_BITS);
	// alternativement on peut utiliser la nouvelle fonction glClearBufferfv()
	auto computeProgram = g_ComputeShader.GetProgram();
	glUseProgram(computeProgram);

	glDispatchCompute(NUM_PARTICLES /256, 1, 1);
	glMemoryBarrier(GL_ALL_BARRIER_BITS);

	auto basicProgram = g_BasicShader.GetProgram();
	glUseProgram(basicProgram);
	g_Camera.projectionMatrix = glm::perspectiveFov(45.f, (float)WIDTH, (float)HEIGHT, 0.1f, 1000.f);
	glm::vec4 position = glm::vec4(0.0f, -0.0f, 4.0f, 1.0f);
	g_Camera.viewMatrix = glm::lookAt(glm::vec3(position), glm::vec3(0.f), glm::vec3(0.f, 1.f, 0.f));

	auto projLocation = glGetUniformLocation(basicProgram, "u_projectionMatrix");
	glUniformMatrix4fv(projLocation, 1, GL_FALSE, glm::value_ptr(g_Camera.projectionMatrix));

	auto viewLocation = glGetUniformLocation(basicProgram, "u_viewMatrix");
	glUniformMatrix4fv(viewLocation, 1, GL_FALSE, glm::value_ptr(g_Camera.viewMatrix));

	auto worldLocation = glGetUniformLocation(basicProgram, "u_worldMatrix");
	glUniformMatrix4fv(worldLocation, 1, GL_FALSE, glm::value_ptr(g_Objet.worldMatrix));

	//glActiveTexture(GL_TEXTURE0);
	//glBindTexture(GL_TEXTURE_2D, tex0);
	//auto diffuseMapLocation = glGetUniformLocation(basicProgram, "u_SamplerDiffuse");
	//glUniform1i(diffuseMapLocation, 0);

	//glActiveTexture(GL_TEXTURE1);
	//glBindTexture(GL_TEXTURE_2D, tex1);
	//auto specularMapLocation = glGetUniformLocation(basicProgram, "u_SamplerSpec");
	//glUniform1i(specularMapLocation, 1);

	//glActiveTexture(GL_TEXTURE2);
	//glBindTexture(GL_TEXTURE_2D, tex2);
	//auto normalMapLocation = glGetUniformLocation(basicProgram, "u_SamplerNorm");
	//glUniform1i(normalMapLocation, 2);

	

	// zero correspond ici a la valeur de layout(location=0) dans le shader basic.vs
	//glBindVertexArray(VAO);
	//glDrawArrays(GL_TRIANGLES, 0, testFBX->nb_vertices);
	
	//glBindVertexArray(VAO);
	//glBindBuffer(GL_ARRAY_BUFFER, posSSbo);
	//glVertexPointer(4, GL_FLOAT, 0, (void *)0);
	//glEnableClientState(GL_VERTEX_ARRAY);


	/*glBindBuffer(GL_ARRAY_BUFFER, colSSbo);
	glColorPointer(4, GL_FLOAT, 0, (void *)0);
	glEnableClientState(GL_COLOR_ARRAY);
	glColor3f(1, 0, 0);*/


	//glPointSize(10);
	//glDrawArrays(GL_POINTS, 0, NUM_PARTICLES);
	//// glDisableClientState(GL_VERTEX_ARRAY);
	
	glBindVertexArray(VAO);

	//glBindBuffer(GL_ARRAY_BUFFER, posSSbo);
	//glEnableClientState(GL_VERTEX_ARRAY);
	/*GLfloat * tmp = new GLfloat[NUM_PARTICLES * 4];
	int cpt = 0;
	for (int i = 0; i < NUM_PARTICLES; i++)
	{
		tmp[cpt] = points[i].x;
		tmp[cpt+1] = points[i].y;
		tmp[cpt+2] = points[i].z;
		tmp[cpt+3] = points[i].w;
		cpt += 4;
	}

	glBufferData(GL_ARRAY_BUFFER, NUM_PARTICLES * 4 * sizeof(float), tmp, GL_STATIC_DRAW);
	glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, (GLvoid *)0);
	glEnableVertexAttribArray(0);*/

	//glBindBuffer(GL_ARRAY_BUFFER, colSSbo);
	//glColorPointer(4, GL_FLOAT, 0, (void *)0);
	//glEnableClientState(GL_COLOR_ARRAY);
	//glColor3f(1, 0, 0);



	glPointSize(3.);
	glDrawArrays(GL_POINTS, 0, NUM_PARTICLES);
	
	glfwSwapBuffers(window);
}

int main()
{
    glfwInit();

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);

    window = glfwCreateWindow(WIDTH, HEIGHT, "OpenGL Test", nullptr, nullptr);

    int screenWidth, screenHeight;
    glfwGetFramebufferSize(window, &screenWidth, &screenHeight);

    if (nullptr == window)
    {
        std::cout << "Failed to create GLFW Window" << std::endl;
        glfwTerminate();
        return EXIT_FAILURE;
    }

    glfwMakeContextCurrent(window);

    glewExperimental = GL_TRUE;

    if (GLEW_OK != glewInit())
    {
        std::cout << "Failed to initialize GLEW" << std::endl;
        return EXIT_FAILURE;
    }
	Resize(WIDTH, HEIGHT);
	Initialise();

	while (!glfwWindowShouldClose(window))
	{
		Render();
	}

    glfwTerminate();
	Terminate();
    return EXIT_SUCCESS;
}