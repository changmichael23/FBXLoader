// ---------------------------------------------------------------------------
//
// OpenGL Shader Framework
// Malek Bengougam, 2012							malek.bengougam@gmail.com
//
// ---------------------------------------------------------------------------

#ifndef ESGI_SHADER_H
#define ESGI_SHADER_H

// --- Includes --------------------------------------------------------------

#ifdef _MSC_VER
#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif
#endif

// --- Classes ---------------------------------------------------------------

class EsgiShader
{
public:
	EsgiShader() : m_ProgramObject(0), m_VertexShader(0)
		, m_FragmentShader(0), m_GeometryShader(0)
	{
	}
	~EsgiShader()
	{
	}

	bool LoadVertexShader(const char *source);
	bool LoadGeometryShader(const char *source);
	bool LoadFragmentShader(const char *source);

	bool LoadComputeShader(const char *source);

	bool Create();
	void Destroy();

	unsigned int Bind();
	void Unbind();

	inline unsigned int GetProgram() const { return m_ProgramObject; }

	void SetPreLinkCallback(void(*callback)(unsigned int)) { m_PreLinkCallback = callback; }

private:
	// handle du program object
	unsigned int m_ProgramObject;
	// handles des shaders
	unsigned int m_VertexShader;
	unsigned int m_FragmentShader;
	unsigned int m_GeometryShader;
	unsigned int m_ComputeShader;
	// callback appelee avant glLinkProgram()
	void(*m_PreLinkCallback)(unsigned int);
};

#endif // ESGI_SHADER_H