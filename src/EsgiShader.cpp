// ---------------------------------------------------------------------------
//
// OpenGL Framework
// Malek Bengougam, 2012							malek.bengougam@gmail.com
//
// ---------------------------------------------------------------------------

// --- Includes --------------------------------------------------------------

#include "EsgiShader.h"
#define GLEW_STATIC
#include "GL/glew.h"

#include <cstdio>
#include <cstdlib>
#include <cstdarg>

#ifdef _WIN32
#include <windows.h>
#endif

void GL_PRINT(const char *format, ...)
{
	char buffer[512];

	// Formatage du message dans une chaine de caractere
	va_list vaParams;
	va_start(vaParams, format);
	vsnprintf(buffer, 512, format, vaParams);
	va_end(vaParams);

#ifdef _MSC_VER
	//::OutputDebugStringA(buffer);
#endif
	printf("%s", buffer);
}

// --- Fonctions -------------------------------------------------------------

static char* FileToString(const char *sourceFile)
{
	char* text = NULL;

	if (sourceFile != NULL)
	{
		FILE *file = fopen(sourceFile, "r");

		if (file != NULL)
		{
			fseek(file, 0, SEEK_END);
			int count = ftell(file);
			rewind(file);

			if (count > 0)
			{
				text = (char*)malloc(count + 1);
				count = fread(text, sizeof(char), count, file);
				text[count] = '\0';
			}

			fclose(file);
		}
	}

	return text;
}

///
// Cree un shader object, charge le code source du shader et le compile
//
static GLuint LoadShader(GLenum type, const char *sourceFile)
{
	// Preload le fichier de shader
	char *shaderSrc = FileToString(sourceFile);
	if (shaderSrc == NULL) {
		return false;
	}

	// Cree le shader object
	GLuint shader = glCreateShader(type);
	if (shader == 0) {
		return 0;
	}

	// Load the shader source
	glShaderSource(shader, 1, (const char **)&shaderSrc, NULL);

	// Compile le shader
	glCompileShader(shader);

	// on n'a plus besoin du fichier texte
	free(shaderSrc);

	// verifie le status de la compilation
	GLint compiled;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);

	if (!compiled)
	{
		GLint infoLen = 0;

		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLen);

		if (infoLen > 1)
		{
			char* infoLog = (char *)malloc(1 + infoLen);

			glGetShaderInfoLog(shader, infoLen, NULL, infoLog);
			GL_PRINT("Error compiling shader:\n%s\n", infoLog);

			free(infoLog);
		}

		// on supprime le shader object car il est inutilisable
		glDeleteShader(shader);

		return 0;
	}

	return shader;
}

bool EsgiShader::LoadVertexShader(const char *sourceFile)
{
	m_VertexShader = LoadShader(GL_VERTEX_SHADER, sourceFile);
	return (m_VertexShader != 0);
}

#ifdef GL_GEOMETRY_SHADER
bool EsgiShader::LoadGeometryShader(const char *sourceFile)
{
	m_GeometryShader = LoadShader(GL_GEOMETRY_SHADER, sourceFile);
	return (m_GeometryShader != 0);
}
#endif

bool EsgiShader::LoadFragmentShader(const char *sourceFile)
{
	m_FragmentShader = LoadShader(GL_FRAGMENT_SHADER, sourceFile);
	return (m_FragmentShader != 0);
}

#ifdef GL_COMPUTE_SHADER
bool EsgiShader::LoadComputeShader(const char *sourceFile)
{
	m_ComputeShader = LoadShader(GL_COMPUTE_SHADER, sourceFile);
	return (m_ComputeShader != 0);
}
#endif

//
// Initialise les shader & program object
//
bool EsgiShader::Create()
{
	// Cree le program object
	m_ProgramObject = glCreateProgram();

	if (m_ProgramObject == 0) {
		return false;
	}

	if (m_VertexShader) {
		glAttachShader(m_ProgramObject, m_VertexShader);
	}
	if (m_FragmentShader) {
		glAttachShader(m_ProgramObject, m_FragmentShader);
	}
#ifdef GL_GEOMETRY_SHADER
	if (m_GeometryShader) {
		glAttachShader(m_ProgramObject, m_GeometryShader);
	}
#endif

#ifdef GL_COMPUTE_SHADER
	if (m_ComputeShader) {
		glAttachShader(m_ProgramObject, m_ComputeShader);
	}
#endif

	// callback permettant d'effectuer des operations avant le linkage
	if (m_PreLinkCallback) {
		m_PreLinkCallback(m_ProgramObject);
	}

	// Liage des shaders dans le programme
	glLinkProgram(m_ProgramObject);

	GLint linked = 0;
	GLint infoLen = 0;

	// verification du statut du linkage
	glGetProgramiv(m_ProgramObject, GL_LINK_STATUS, &linked);

	if (!linked)
	{
		glGetProgramiv(m_ProgramObject, GL_INFO_LOG_LENGTH, &infoLen);

		if (infoLen > 1)
		{
			char* infoLog = (char *)malloc(infoLen + 1);

			glGetProgramInfoLog(m_ProgramObject, infoLen, NULL, infoLog);
			GL_PRINT("Erreur de lien du programme:\n%s\n", infoLog);

			free(infoLog);
		}

		glDeleteProgram(m_ProgramObject);

		return false;
	}

#if defined(_DEBUG)	|| defined(DEBUG)
	// ne pas utiliser glValidateProgram() au runtime.
	// techniquement il faudrait appeler glValidateProgram() dans le contexte
	// d'utilisation du shader et non a sa creation pour verifier que toutes les 
	// conditions d'execution sont bien remplies
	glValidateProgram(m_ProgramObject);
	glGetProgramiv(m_ProgramObject, GL_INFO_LOG_LENGTH, &infoLen);
	if (infoLen > 1)
	{
		char* infoLog = (char *)malloc(sizeof(char) * infoLen);

		glGetProgramInfoLog(m_ProgramObject, infoLen, NULL, infoLog);
		GL_PRINT("Resultat de la validation du programme:\n%s\n", infoLog);

		free(infoLog);
	}
#endif

	return true;
}

//
// Libere la memoire occupee par le program et le shader object
//
void EsgiShader::Destroy()
{
	if (m_VertexShader) {
		glDetachShader(m_ProgramObject, m_VertexShader);
		glDeleteShader(m_VertexShader);
	}

	if (m_FragmentShader) {
		glDetachShader(m_ProgramObject, m_FragmentShader);
		glDeleteShader(m_FragmentShader);
	}

#ifdef GL_GEOMETRY_SHADER
	if (m_GeometryShader) {
		glDetachShader(m_ProgramObject, m_GeometryShader);
		glDeleteShader(m_GeometryShader);
	}
#endif

#ifdef GL_COMPUTE_SHADER
	if (m_ComputeShader) {
		glDetachShader(m_ProgramObject, m_ComputeShader);
		glDeleteShader(m_ComputeShader);
	}
#endif

	if (m_ProgramObject) {
		glDeleteProgram(m_ProgramObject);
	}
}

//
//
//
GLuint EsgiShader::Bind() {
	glUseProgram(m_ProgramObject);
	return m_ProgramObject;
}

void EsgiShader::Unbind() {
	glUseProgram(0);
}