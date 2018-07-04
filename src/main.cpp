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
#include <vector>
#include "FBXLoader.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

FBXLoader* testFBX;

const GLint WIDTH = 1920, HEIGHT = 1080;

EsgiShader g_BasicShader;
GLFWwindow *window;

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

bool Initialise()
{
	testFBX = new FBXLoader(VBO_position,IBO,VAO);
	testFBX->LoadFBX();
	//testFBX->vertex_data

	//for (int i = 0; i < testFBX->nb_normals; ++i)
	//{
	//	std::cerr<<testFBX->vertex_nor[i]<<std::endl;
	//}
	g_BasicShader.LoadVertexShader("../src/shaders/basic.vs");
	g_BasicShader.LoadFragmentShader("../src/shaders/basic.fs");
	g_BasicShader.Create();

	// load texture

	LoadAndCreateTextureRGBA("../samples/ironman/ironman.fbm/ironman.dff.png", tex0);
	LoadAndCreateTextureRGBA("../samples/ironman/ironman.fbm/ironman.spec.png", tex1);
	LoadAndCreateTextureRGBA("../samples/ironman/ironman.fbm/ironman.norm.png", tex2, true);

	static const float triangle[] = {
		-0.5f, -0.5f,
		0.5f, -0.5f,
		0.0f, 0.5f
	};
	/*glGenBuffers(1, &IBO);
	glBindVertexArray(VAO);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IBO);

	glBufferData(GL_ELEMENT_ARRAY_BUFFER, testFBX->nb_indices * sizeof(GLuint), testFBX->vertex_ind, GL_STATIC_DRAW);
*/

	//glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (GLvoid *)0);
	//glEnableVertexAttribArray(0);

	//glBindVertexArray(0);


	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO_position);
	glGenBuffers(1, &VBO_normal);
	glGenBuffers(1, &VBO_texture);
	glBindVertexArray(VAO);

	glBindBuffer(GL_ARRAY_BUFFER, VBO_position);
	glBufferData(GL_ARRAY_BUFFER, testFBX->nb_vertices*4*sizeof(float), testFBX->vertex_pos, GL_STATIC_DRAW);
	glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, (GLvoid *)0);
	glEnableVertexAttribArray(0);

	glBindBuffer(GL_ARRAY_BUFFER, VBO_normal);
	glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * testFBX->nb_normals * 3, testFBX->vertex_nor, GL_STATIC_DRAW);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, (GLvoid *)0);
	glEnableVertexAttribArray(1);

	glBindBuffer(GL_ARRAY_BUFFER, VBO_texture);
	glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * testFBX->nb_uvs * 2, testFBX->vertex_uv, GL_STATIC_DRAW);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, (GLvoid *)0);
	glEnableVertexAttribArray(2);



	glBindVertexArray(0);

	return true;
}

void Terminate() {
	g_BasicShader.Destroy();
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
	
	// alternativement on peut utiliser la nouvelle fonction glClearBufferfv()

	auto basicProgram = g_BasicShader.GetProgram();
	glUseProgram(basicProgram);

	g_Camera.projectionMatrix = glm::perspectiveFov(45.f, (float)WIDTH, (float)HEIGHT, 0.1f, 1000.f);
	glm::vec4 position = glm::vec4(-1.0f, 2.5f, 12.0f, 1.0f);
	g_Camera.viewMatrix = glm::lookAt(glm::vec3(position), glm::vec3(0.f), glm::vec3(0.f, 1.f, 0.f));

	auto projLocation = glGetUniformLocation(basicProgram, "u_projectionMatrix");
	glUniformMatrix4fv(projLocation, 1, GL_FALSE, glm::value_ptr(g_Camera.projectionMatrix));

	auto viewLocation = glGetUniformLocation(basicProgram, "u_viewMatrix");
	glUniformMatrix4fv(viewLocation, 1, GL_FALSE, glm::value_ptr(g_Camera.viewMatrix));

	auto worldLocation = glGetUniformLocation(basicProgram, "u_worldMatrix");
	glUniformMatrix4fv(worldLocation, 1, GL_FALSE, glm::value_ptr(g_Objet.worldMatrix));

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, tex0);
	auto diffuseMapLocation = glGetUniformLocation(basicProgram, "u_SamplerDiffuse");
	glUniform1i(diffuseMapLocation, 0);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, tex1);
	auto specularMapLocation = glGetUniformLocation(basicProgram, "u_SamplerSpec");
	glUniform1i(specularMapLocation, 1);

	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, tex2);
	auto normalMapLocation = glGetUniformLocation(basicProgram, "u_SamplerNorm");
	glUniform1i(normalMapLocation, 2);

	

	// zero correspond ici a la valeur de layout(location=0) dans le shader basic.vs
	glBindVertexArray(VAO);
	glDrawArrays(GL_TRIANGLES, 0, testFBX->nb_vertices);
	glfwSwapBuffers(window);
}

int main()
{
    glfwInit();

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
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