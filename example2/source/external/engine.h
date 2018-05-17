//**************************************************
// Proto Engine v 0.01 - 2018-05-17
// Copyright (c) 2018 Raúl Rita
// Attribution 4.0 International (CC BY 4.0)
// https://creativecommons.org/licenses/by/4.0/
//**************************************************

#define WIN32_LEAN_AND_MEAN
#include "stb_image.h"
#include <stdbool.h>
#include <math.h>
#include <windows.h>
#include <gl/gl.h>

//**************************************************
// CONFIG
//**************************************************

char APP_NAME[] = "WorkingTitle";
int DISPLAY_WIDTH = 1920;
int DISPLAY_HEIGHT = 1080;
bool FULL_SCREEN = true;
bool PIXEL_ART = false;
bool SHOW_CURSOR = false;
bool DEBUG = false;

//**************************************************
// GLOBALS - can be used - not defined here
//**************************************************

/*
bool quit - if true ends the game
Shader current_shader - shader in use
bool input_keys[256]; // keys pressed
bool released_keys[256]; // keys released
bool key_any; // any key pressed
*/

//**************************************************
// BASIC HEADER - to implement in game
//**************************************************

void game_init();
void game_tick(const float delta);
void game_terminate();

//**************************************************
// CONSTANTS
//**************************************************

const float PI_OVER_360 = 0.0087266f;
const float PI = 3.14159265358979323846f;
const float HALF_PI = 1.57079632679f;

const int FRAMES_PER_SECOND = 60;
const float FRAME_TARGET = 16.67f; //1000.f / (float)FRAMES_PER_SECOND;

//**************************************************
// TYPES
//**************************************************

typedef char* string;
typedef signed char sbyte;
typedef unsigned char byte;
typedef unsigned short word;
typedef unsigned int uint;

typedef struct DataHolder
{
	long length;
	void* data;	
} DataHolder;

typedef struct Vector
{
    float x;
    float y;
} Vector;

const Vector VZero = { 0, 0 };

typedef struct Rect
{
	uint x;
	uint y;
	uint width;
	uint height;
} Rect;

typedef struct Quad
{
	Vector top_left;
	Vector top_right;
	Vector bottom_left;
	Vector bottom_right;
} Quad;

typedef struct
{
    uint id;
    uint width;
    uint height;

    Vector position;
    Vector pivot;
    Rect source;
    byte alpha;
    byte shadow;
    float scale;
    float rotation;
    bool visible;
    bool flip_x;
    bool flip_y;
} Texture;

typedef struct Shader
{
	word id;
	
	// locations on shader
	uint vertex_position;
	uint texture_position;
	
} Shader;

Shader current_shader;
Shader base_shader;

float square_vertices[] = // dummy
{
	-1.0f, -1.0f,
	1.0f, -1.0f,
	1.0f, 1.0f,
	-1.0f, 1.0f,
}; // 4 for square

float texture_vertices[] = // dummy
{
	0.0f, 1.0f,
	1.0f, 1.0f,
	1.0f, 0.0f,
	0.0f, 0.0f
};

const string direct_vs = "#version 100
attribute vec2 vertex_position;
attribute vec2 texture_position;
varying vec2 texture_coordinate;
void main()
{
gl_Position = vec4(vertex_position, 0, 1);
texture_coordinate = texture_position;
}";

const string direct_fs = "#version 100
precision mediump float;
varying vec2 texture_coordinate;
uniform sampler2D texture0;
void main()
{   
gl_FragColor = texture2D(texture0, texture_coordinate);
}";

//**************************************************
// INPUT
//**************************************************

bool input_keys[256];
bool released_keys[256];
bool key_any;

bool key_down(const char key)
{
	return input_keys[key];
}

bool key_up(const char key)
{
	return released_keys[key];
}


//**************************************************
// FUNCTIONS
//**************************************************

void debug(const char* format, ...)
{
	if (! DEBUG)
		return;
	
	char str[1024];

	va_list argptr;
	va_start(argptr, format);
	vsnprintf(str, sizeof(str), format, argptr);
	va_end(argptr);
	
	FILE* file;
	file = fopen("log.txt", "a");
	fputs(str, file);
	fputs("\n", file);
	fclose(file);
}

void debug_clean()
{
	FILE* file;
	file = fopen("log.txt", "w");
	fclose(file);	
}

DataHolder load_file(const string filename)
{
    DataHolder result;

    debug("Opening file %s", filename);
    
    FILE* file = fopen(filename, "rb");
    
    fseek(file, 0, SEEK_END);
    result.length = ftell(file);
    rewind(file);

    result.data = (byte*)malloc(result.length * sizeof(byte));

    fread(result.data, sizeof(byte), result.length, file);
    fclose(file);
    
    return result;
}

char* load_text(const string filename)
{
    DataHolder holder = load_file(filename);

    string result = (char*)malloc(holder.length * sizeof(byte));    
    memcpy(result, holder.data, holder.length);

    free(holder.data);
    
    return result;  
}


float to_degrees(const float radians)
{
    return radians * 180.f / PI;
    //return (radians > 0 ? radians : (2.f * PI + radians)) * 360.f / (2.f * PI);
}

float to_radians(const float degrees)
{
    return degrees * PI / 180.f;
}

float translate_x(const float x)
{
    return x * 2.0f / DISPLAY_WIDTH - 1.f;
}

float translate_y(const float y)
{
    return y * -2.0f / DISPLAY_HEIGHT + 1.f;
}


bool equals(const Vector value1, const Vector value2)
{
    return value1.x == value2.x && value1.y == value2.y;
}

bool vector_in_rect(const Vector point, const Rect rect)
{
    return
        rect.x <= point.x && rect.x + rect.width >= point.x &&
        rect.y <= point.y && rect.y + rect.height >= point.y;
}

void subtract(Vector* v1, const Vector v2)
{
    v1->x -= v2.x;
    v1->y -= v2.y;
}

void add(Vector* v1, const Vector v2)
{
    v1->x += v2.x;
    v1->y += v2.y;
}

void product(Vector* v1, const float v2)
{
    v1->x *= v2;
    v1->y *= v2;
}

float wrap(const float value, const float lower, const float upper)
{
    float rangeZero = upper - lower;

    if (value >= lower && value <= upper)
        return value;

    return fmod(value, rangeZero) + lower;
}

float magnitude(Vector* vector)
{
    return (float)sqrt(vector->x * vector->x + vector->y * vector->y);
}

void normalize(Vector* vector)
{
    float magnitude = (float)sqrt(vector->x * vector->x + vector->y * vector->y);

    vector->x /= magnitude;
    vector->y /= magnitude;
}

float space(const Vector v1, const Vector v2)
{
    return sqrt(pow((v2.x - v1.x), 2) + pow((v2.y - v1.y), 2));
}

void rotate(Vector* point, const Vector pivot, const float angle)
{
    const float x =
        (point->x - pivot.x) * cos(angle) -
        (point->y - pivot.y) * sin(angle) +
        pivot.x;

    const float y =
        (point->x - pivot.x) * sin(angle) +
        (point->y - pivot.y) * cos(angle) +
        pivot.y;

    point->x = x;
    point->y = y;
}

float lerp(const float start, const float target, const float percentage) // percentage [0..1]
{
    return start + percentage * (target - start);
}

float angular_lerp(const float start, const float target, const float percentage)
{
    float startHelper = start;
    float targetHelper = target;

    float difference = abs(targetHelper - startHelper);
    if (difference > 180)
    {
        // We need to add on to one of the values.
        if (targetHelper > startHelper)
        {
            // We'll add it on to start...
            startHelper += 360.f;
        }
        else
        {
            // Add it on to end.
            targetHelper += 360.f;
        }

        // Interpolate it.
        float result = lerp(startHelper, targetHelper, percentage);

        return wrap(result, 0, 360);
    }

    return lerp(startHelper, targetHelper, percentage);
}

bool is_zero(const Vector v1)
{
	return v1.x == 0 && v1.y == 0;
}

Vector center(const Quad quad)
{
	Vector result =
    {
        (float)(quad.top_left.x + quad.top_right.x + quad.bottom_left.x + quad.bottom_right.x) / 4.f,
        (float)(quad.top_left.y + quad.top_right.y + quad.bottom_left.y + quad.bottom_right.y) / 4.f
    };

    return result;
}

Quad calculate_quad(const Texture texture)
{
    float angle = to_radians(texture.rotation);
    Vector position = texture.position;
    Vector pivot = texture.pivot;
    float scale = texture.scale;
    int source_width = texture.source.width;
    int source_height = texture.source.height;

    if (scale != 1.0f)
    {
        source_width *= scale;
        source_height *= scale;

        if (! is_zero(pivot))
        {
            pivot.x *= scale;
            pivot.y *= scale;
        }
    }

    if (! is_zero(pivot))
        subtract(&position, pivot);

    Vector top_left = { position.x, position.y} ;
    Vector top_right = { position.x + source_width, position.y };
    Vector bottom_right = { position.x + source_width, position.y + source_height };
    Vector bottom_left = { position.x, position.y + source_height };

    if (angle != 0)
    {
        if (texture.flip_x)
            pivot.x = source_width - pivot.x;

        if (texture.flip_y)
            pivot.y = source_height - pivot.y;

        add(&pivot, position);

        rotate(&top_left, pivot, angle);
        rotate(&top_right, pivot, angle);
        rotate(&bottom_right, pivot, angle);
        rotate(&bottom_left, pivot, angle);
    }

    if (texture.flip_x)
    {
        Vector aux = top_left;
        top_left = top_right;
        top_right = aux;

        aux = bottom_left;
        bottom_left = bottom_right;
        bottom_right = aux;
    }

    if (texture.flip_y)
    {
        Vector aux = top_left;
        top_left = bottom_left;
        bottom_left = aux;

        aux = top_right;
        top_right = bottom_right;
        bottom_right = aux;
    }

    // not working in orangec
    //Quad result = { top_left, top_right, bottom_right, bottom_left };
    Quad result;
    result.top_left = top_left;
    result.top_right = top_right;
    result.bottom_right = bottom_right;
    result.bottom_left = bottom_left;

    return result;
}

//**************************************************
// OPENGL
//**************************************************

typedef BOOL (WINAPI * PFNWGLCHOOSEPIXELFORMATARBPROC) (HDC hdc, const int *piAttribIList, const FLOAT *pfAttribFList, UINT nMaxFormats, int *piFormats, UINT *nNumFormats);
typedef HGLRC (WINAPI * PFNWGLCREATECONTEXTATTRIBSARBPROC) (HDC hDC, HGLRC hShareContext, const int *attribList);
typedef BOOL (WINAPI * PFNWGLSWAPINTERVALEXTPROC) (int interval);
typedef void (APIENTRY * PFNGLATTACHSHADERPROC) (GLuint program, GLuint shader);
typedef void (APIENTRY * PFNGLBINDBUFFERPROC) (GLenum target, GLuint buffer);
typedef void (APIENTRY * PFNGLBINDVERTEXARRAYPROC) (GLuint array);
typedef void (APIENTRY * PFNGLBUFFERDATAPROC) (GLenum target, ptrdiff_t size, const GLvoid *data, GLenum usage);
typedef void (APIENTRY * PFNGLCOMPILESHADERPROC) (GLuint shader);
typedef GLuint (APIENTRY * PFNGLCREATEPROGRAMPROC) (void);
typedef GLuint (APIENTRY * PFNGLCREATESHADERPROC) (GLenum type);
typedef void (APIENTRY * PFNGLDELETEBUFFERSPROC) (GLsizei n, const GLuint *buffers);
typedef void (APIENTRY * PFNGLDELETEPROGRAMPROC) (GLuint program);
typedef void (APIENTRY * PFNGLDELETESHADERPROC) (GLuint shader);
typedef void (APIENTRY * PFNGLDELETEVERTEXARRAYSPROC) (GLsizei n, const GLuint *arrays);
typedef void (APIENTRY * PFNGLDETACHSHADERPROC) (GLuint program, GLuint shader);
typedef void (APIENTRY * PFNGLENABLEVERTEXATTRIBARRAYPROC) (GLuint index);
typedef void (APIENTRY * PFNGLGENBUFFERSPROC) (GLsizei n, GLuint *buffers);
typedef void (APIENTRY * PFNGLGENVERTEXARRAYSPROC) (GLsizei n, GLuint *arrays);
typedef GLint (APIENTRY * PFNGLGETATTRIBLOCATIONPROC) (GLuint program, const char *name);
typedef void (APIENTRY * PFNGLGETPROGRAMINFOLOGPROC) (GLuint program, GLsizei bufSize, GLsizei *length, char *infoLog);
typedef void (APIENTRY * PFNGLGETPROGRAMIVPROC) (GLuint program, GLenum pname, GLint *params);
typedef void (APIENTRY * PFNGLGETSHADERINFOLOGPROC) (GLuint shader, GLsizei bufSize, GLsizei *length, char *infoLog);
typedef void (APIENTRY * PFNGLGETSHADERIVPROC) (GLuint shader, GLenum pname, GLint *params);
typedef void (APIENTRY * PFNGLLINKPROGRAMPROC) (GLuint program);
typedef void (APIENTRY * PFNGLSHADERSOURCEPROC) (GLuint shader, GLsizei count, const char* *string, const GLint *length);
typedef void (APIENTRY * PFNGLUSEPROGRAMPROC) (GLuint program);
typedef void (APIENTRY * PFNGLVERTEXATTRIBPOINTERPROC) (GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const GLvoid *pointer);
typedef void (APIENTRY * PFNGLBINDATTRIBLOCATIONPROC) (GLuint program, GLuint index, const char *name);
typedef GLint (APIENTRY * PFNGLGETUNIFORMLOCATIONPROC) (GLuint program, const char *name);
typedef void (APIENTRY * PFNGLUNIFORMMATRIX4FVPROC) (GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
typedef void (APIENTRY * PFNGLACTIVETEXTUREPROC) (GLenum texture);
typedef void (APIENTRY * PFNGLUNIFORM1IPROC) (GLint location, GLint v0);
typedef void (APIENTRY * PFNGLGENERATEMIPMAPPROC) (GLenum target);
typedef void (APIENTRY * PFNGLDISABLEVERTEXATTRIBARRAYPROC) (GLuint index);
typedef void (APIENTRY * PFNGLUNIFORM1FPROC) (GLint location, GLfloat v0);
typedef void (APIENTRY * PFNGLUNIFORM2FPROC) (GLint location, GLfloat v0);
typedef void (APIENTRY * PFNGLUNIFORM3FVPROC) (GLint location, GLsizei count, const GLfloat *value);
typedef void (APIENTRY * PFNGLUNIFORM4FVPROC) (GLint location, GLsizei count, const GLfloat *value);
typedef void (APIENTRY * PFNGLVEXTEXATTRIB3FPROC) (GLuint index, GLfloat v0, GLfloat v1, GLfloat v2);
typedef void (APIENTRY * PFNGLUNIFORM4FPROC) (GLuint index, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3);

#define WGL_DRAW_TO_WINDOW_ARB         0x2001
#define WGL_ACCELERATION_ARB           0x2003
#define WGL_SWAP_METHOD_ARB            0x2007
#define WGL_SUPPORT_OPENGL_ARB         0x2010
#define WGL_DOUBLE_BUFFER_ARB          0x2011
#define WGL_PIXEL_TYPE_ARB             0x2013
#define WGL_COLOR_BITS_ARB             0x2014
#define WGL_DEPTH_BITS_ARB             0x2022
#define WGL_STENCIL_BITS_ARB           0x2023
#define WGL_FULL_ACCELERATION_ARB      0x2027
#define WGL_SWAP_EXCHANGE_ARB          0x2028
#define WGL_TYPE_RGBA_ARB              0x202B
#define WGL_CONTEXT_MAJOR_VERSION_ARB  0x2091
#define WGL_CONTEXT_MINOR_VERSION_ARB  0x2092

#define GL_ARRAY_BUFFER                   0x8892
#define GL_STATIC_DRAW                    0x88E4
#define GL_FRAGMENT_SHADER                0x8B30
#define GL_VERTEX_SHADER                  0x8B31
#define GL_COMPILE_STATUS                 0x8B81
#define GL_LINK_STATUS                    0x8B82
#define GL_INFO_LOG_LENGTH                0x8B84
#define GL_TEXTURE0                       0x84C0
#define GL_BGRA                           0x80E1
#define GL_ELEMENT_ARRAY_BUFFER           0x8893
#define GL_CLAMP_TO_EDGE                  0x812F
#define GL_GENERATE_MIPMAP_HINT           0x8192

PFNGLUSEPROGRAMPROC glUseProgram;
PFNGLATTACHSHADERPROC glAttachShader;
PFNGLBINDBUFFERPROC glBindBuffer;
PFNGLBINDVERTEXARRAYPROC glBindVertexArray;
PFNGLBUFFERDATAPROC glBufferData;
PFNGLCOMPILESHADERPROC glCompileShader;
PFNGLCREATEPROGRAMPROC glCreateProgram;
PFNGLCREATESHADERPROC glCreateShader;
PFNGLDELETEBUFFERSPROC glDeleteBuffers;
PFNGLDELETEPROGRAMPROC glDeleteProgram;
PFNGLDELETESHADERPROC glDeleteShader;
PFNGLDELETEVERTEXARRAYSPROC glDeleteVertexArrays;
PFNGLDETACHSHADERPROC glDetachShader;
PFNGLENABLEVERTEXATTRIBARRAYPROC glEnableVertexAttribArray;
PFNGLGENBUFFERSPROC glGenBuffers;
PFNGLGENVERTEXARRAYSPROC glGenVertexArrays;
PFNGLGETATTRIBLOCATIONPROC glGetAttribLocation;
PFNGLGETPROGRAMINFOLOGPROC glGetProgramInfoLog;
PFNGLGETPROGRAMIVPROC glGetProgramiv;
PFNGLGETSHADERINFOLOGPROC glGetShaderInfoLog;
PFNGLGETSHADERIVPROC glGetShaderiv;
PFNGLLINKPROGRAMPROC glLinkProgram;
PFNGLSHADERSOURCEPROC glShaderSource;
PFNGLVERTEXATTRIBPOINTERPROC glVertexAttribPointer;
PFNGLBINDATTRIBLOCATIONPROC glBindAttribLocation;
PFNGLGETUNIFORMLOCATIONPROC glGetUniformLocation;
PFNGLUNIFORMMATRIX4FVPROC glUniformMatrix4fv;
PFNGLACTIVETEXTUREPROC glActiveTexture;
PFNGLUNIFORM1IPROC glUniform1i;
PFNGLGENERATEMIPMAPPROC glGenerateMipmap;
PFNGLDISABLEVERTEXATTRIBARRAYPROC glDisableVertexAttribArray;
PFNGLUNIFORM1FPROC glUniform1f;
PFNGLUNIFORM2FPROC glUniform2f;
PFNGLUNIFORM3FVPROC glUniform3fv;
PFNGLUNIFORM4FVPROC glUniform4fv;
PFNGLVEXTEXATTRIB3FPROC glVertexAttrib3f;
PFNGLUNIFORM4FPROC glUniform4f;

PFNWGLCHOOSEPIXELFORMATARBPROC wglChoosePixelFormatARB;
PFNWGLCREATECONTEXTATTRIBSARBPROC wglCreateContextAttribsARB;
PFNWGLSWAPINTERVALEXTPROC wglSwapIntervalEXT;

void load_opengl_extensions()
{
	wglChoosePixelFormatARB = (PFNWGLCHOOSEPIXELFORMATARBPROC)wglGetProcAddress("wglChoosePixelFormatARB");
	wglCreateContextAttribsARB = (PFNWGLCREATECONTEXTATTRIBSARBPROC)wglGetProcAddress("wglCreateContextAttribsARB");
	wglSwapIntervalEXT = (PFNWGLSWAPINTERVALEXTPROC)wglGetProcAddress("wglSwapIntervalEXT");
	glAttachShader = (PFNGLATTACHSHADERPROC)wglGetProcAddress("glAttachShader");
	glBindBuffer = (PFNGLBINDBUFFERPROC)wglGetProcAddress("glBindBuffer");
	glBindVertexArray = (PFNGLBINDVERTEXARRAYPROC)wglGetProcAddress("glBindVertexArray");
	glBufferData = (PFNGLBUFFERDATAPROC)wglGetProcAddress("glBufferData");
	glCompileShader = (PFNGLCOMPILESHADERPROC)wglGetProcAddress("glCompileShader");
	glCreateProgram = (PFNGLCREATEPROGRAMPROC)wglGetProcAddress("glCreateProgram");
	glCreateShader = (PFNGLCREATESHADERPROC)wglGetProcAddress("glCreateShader");
	glDeleteBuffers = (PFNGLDELETEBUFFERSPROC)wglGetProcAddress("glDeleteBuffers");
	glDeleteProgram = (PFNGLDELETEPROGRAMPROC)wglGetProcAddress("glDeleteProgram");
	glDeleteShader = (PFNGLDELETESHADERPROC)wglGetProcAddress("glDeleteShader");
	glDeleteVertexArrays = (PFNGLDELETEVERTEXARRAYSPROC)wglGetProcAddress("glDeleteVertexArrays");
	glDetachShader = (PFNGLDETACHSHADERPROC)wglGetProcAddress("glDetachShader");
	glEnableVertexAttribArray = (PFNGLENABLEVERTEXATTRIBARRAYPROC)wglGetProcAddress("glEnableVertexAttribArray");
	glGenBuffers = (PFNGLGENBUFFERSPROC)wglGetProcAddress("glGenBuffers");
	glGenVertexArrays = (PFNGLGENVERTEXARRAYSPROC)wglGetProcAddress("glGenVertexArrays");
	glGetAttribLocation = (PFNGLGETATTRIBLOCATIONPROC)wglGetProcAddress("glGetAttribLocation");
	glGetProgramInfoLog = (PFNGLGETPROGRAMINFOLOGPROC)wglGetProcAddress("glGetProgramInfoLog");
	glGetProgramiv = (PFNGLGETPROGRAMIVPROC)wglGetProcAddress("glGetProgramiv");
	glGetShaderInfoLog = (PFNGLGETSHADERINFOLOGPROC)wglGetProcAddress("glGetShaderInfoLog");
	glGetShaderiv = (PFNGLGETSHADERIVPROC)wglGetProcAddress("glGetShaderiv");
	glLinkProgram = (PFNGLLINKPROGRAMPROC)wglGetProcAddress("glLinkProgram");
	glShaderSource = (PFNGLSHADERSOURCEPROC)wglGetProcAddress("glShaderSource");
	glUseProgram = (PFNGLUSEPROGRAMPROC)wglGetProcAddress("glUseProgram");
	glVertexAttribPointer = (PFNGLVERTEXATTRIBPOINTERPROC)wglGetProcAddress("glVertexAttribPointer");
	glBindAttribLocation = (PFNGLBINDATTRIBLOCATIONPROC)wglGetProcAddress("glBindAttribLocation");
	glGetUniformLocation = (PFNGLGETUNIFORMLOCATIONPROC)wglGetProcAddress("glGetUniformLocation");
	glUniformMatrix4fv = (PFNGLUNIFORMMATRIX4FVPROC)wglGetProcAddress("glUniformMatrix4fv");
	glActiveTexture = (PFNGLACTIVETEXTUREPROC)wglGetProcAddress("glActiveTexture");
	glUniform1i = (PFNGLUNIFORM1IPROC)wglGetProcAddress("glUniform1i");
	glGenerateMipmap = (PFNGLGENERATEMIPMAPPROC)wglGetProcAddress("glGenerateMipmap");
	glDisableVertexAttribArray = (PFNGLDISABLEVERTEXATTRIBARRAYPROC)wglGetProcAddress("glDisableVertexAttribArray");
	glUniform1f = (PFNGLUNIFORM1FPROC)wglGetProcAddress("glUniform1f");
	glUniform2f = (PFNGLUNIFORM2FPROC)wglGetProcAddress("glUniform2f");
	glUniform3fv = (PFNGLUNIFORM3FVPROC)wglGetProcAddress("glUniform3fv");
	glUniform4fv = (PFNGLUNIFORM4FVPROC)wglGetProcAddress("glUniform4fv");
	glVertexAttrib3f = (PFNGLVEXTEXATTRIB3FPROC)wglGetProcAddress("glVertexAttrib3f");
	glUniform4f = (PFNGLUNIFORM4FPROC)wglGetProcAddress("glUniform4f");
}

Texture load_texture(string filename)
{
    int width, height, comp;
    unsigned char* image = stbi_load(filename, &width, &height, &comp, STBI_rgb_alpha);

    glBindTexture(GL_TEXTURE_2D, 0); // Free any old binding

    GLuint id = 0;

    glGenTextures(1, &id); // Generate Pointer to the texture

    glBindTexture(GL_TEXTURE_2D, id);

	glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_RGBA,
        width,
        height,
        0,
        GL_RGBA,
        GL_UNSIGNED_BYTE,
        image);

    glGenerateMipmap(GL_TEXTURE_2D);

    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	if (PIXEL_ART) 
	{
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    }
	else
	{
    	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	}

    // Unbind current texture
    glBindTexture(GL_TEXTURE_2D, 0);

    Texture result;

    result.id = id;
    result.position = VZero;
    result.pivot = VZero;
    result.width = width;
    result.height = height;
    result.rotation = 0;
    result.flip_x = false;
    result.flip_y = false;
    result.scale = 1.0f;
    result.source.x = 0;
    result.source.y = 0;
    result.source.width = width;
    result.source.height = height;

    return result;
}

void unload_texture(Texture texture)
{
    if (texture.id != 0)
	{
		glDeleteTextures(1, &texture.id);

		debug("[TEX ID %i] Unloaded texture data from VRAM (GPU)", texture.id);
		texture.id = 0;
	}
}

word load_shader_program(const string vertex_str, const string fragment_str)
{   
    word program = 0;
    int maxLength = 0;
    int length;
    char msg[1024];

    GLuint vertex_shader = glCreateShader(GL_VERTEX_SHADER);
    GLuint fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);

    glShaderSource(vertex_shader, 1, &vertex_str, 0);
    glShaderSource(fragment_shader, 1, &fragment_str, 0);

    GLint success = 0;

    glCompileShader(vertex_shader);

    glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &success);

    if (success != GL_TRUE)
    {
        debug("[VSHDR ID %i] Failed to compile vertex shader...", vertex_shader);

        glGetShaderiv(vertex_shader, GL_INFO_LOG_LENGTH, &maxLength);        

        glGetShaderInfoLog(vertex_shader, maxLength, &length, msg);

        debug("%s", msg);
    }
    else
    {
        debug("[VSHDR ID %i] Vertex shader compiled successfully", vertex_shader);
    }

    glCompileShader(fragment_shader);

    glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &success);

    if (success != GL_TRUE)
    {
        debug("[FSHDR ID %i] Failed to compile fragment shader...", fragment_shader);

        glGetShaderiv(fragment_shader, GL_INFO_LOG_LENGTH, &maxLength);
        glGetShaderInfoLog(fragment_shader, maxLength, &length, msg);

        debug("%s", msg);

    }
    else debug("[FSHDR ID %i] Fragment shader compiled successfully", fragment_shader);

    program = glCreateProgram();

    glAttachShader(program, vertex_shader);
    glAttachShader(program, fragment_shader);

    glLinkProgram(program);

    // NOTE: All uniform variables are initialized to 0 when a program links

    glGetProgramiv(program, GL_LINK_STATUS, &success);

    if (success != GL_TRUE)
    {
        debug("[SHDR ID %i] Failed to link shader program...", program);

        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &maxLength);
        glGetProgramInfoLog(program, maxLength, &length, msg);

        debug("%s", msg);

        glDeleteProgram(program);

        program = 0;
    }
    else debug("[SHDR ID %i] Shader program loaded successfully", program);

    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);

    return program;
}

Shader load_shader_verbose(const string vs, const string fs)
{
    Shader result;

    result.id = load_shader_program(vs, fs);

    if (result.id != 0)
    {
        result.vertex_position = glGetAttribLocation(result.id, "vertex_position");
        result.texture_position = glGetAttribLocation(result.id, "texture_position");
        
        debug("[SHDR ID %i] shader locations set", result.id);
    }
    
    return result;
}

Shader load_shader(const string vs_filename, const string fs_filename)
{
    string vertex_str = load_text(vs_filename);
    string fragment_str = load_text(fs_filename);

    Shader result = load_shader_verbose(vertex_str, fragment_str);

    free(vertex_str);
    free(fragment_str);

    return result;
}

void unload_shader(Shader shader)
{
    glUseProgram(0);
    glDeleteProgram(shader.id);

    shader.id = 0;
}

void draw(const Texture texture)
{
	Quad destination = calculate_quad(texture);
	
	// top left
	square_vertices[0] = translate_x(destination.top_left.x);
	square_vertices[1] = translate_y(destination.top_left.y);

	// top right
	square_vertices[2] = translate_x(destination.top_right.x);
	square_vertices[3] = translate_y(destination.top_right.y);

    // bottom left
    square_vertices[4] = translate_x(destination.bottom_left.x);
    square_vertices[5] = translate_y(destination.bottom_left.y);

    // bottom right
    square_vertices[6] = translate_x(destination.bottom_right.x);
    square_vertices[7] = translate_y(destination.bottom_right.y);

	//////
	// top left
	texture_vertices[0] = (float)texture.source.x / texture.width;
	texture_vertices[1] = (float)texture.source.y / texture.height;

	//top right
	texture_vertices[2] =
		(float)(texture.source.x + texture.source.width) /
		texture.width;
	texture_vertices[3] = (float)texture.source.y / texture.height;

    // bottom left
    texture_vertices[4] = (float)texture.source.x / texture.width;
    texture_vertices[5] =
        (float)(texture.source.y + texture.source.height) /
        texture.height;

	// bottom right
	texture_vertices[6] =
		(float)(texture.source.x + texture.source.width) /
		texture.width;
	texture_vertices[7] =
		(float)(texture.source.y + texture.source.height) /
		texture.height;


    /*
    for (int h = 0; h < 8; h++)
        debug("square_vertices v: %f", square_vertices[h]);

	for (int h = 0; h < 8; h++)
		debug("texture_vertices v: %f", texture_vertices[h]);
    */

	// Set the shader
	glUseProgram(current_shader.id);

	// Load the vertex data (vec2)
	glVertexAttribPointer(
		current_shader.vertex_position,
		2,
		GL_FLOAT,
		GL_FALSE,
		0,
		square_vertices);
	glEnableVertexAttribArray(current_shader.vertex_position);

	// Load the texture coordinates (vec2)
	glVertexAttribPointer(
		current_shader.texture_position,
		2,
		GL_FLOAT,
		GL_FALSE,
		0, texture_vertices);
	glEnableVertexAttribArray(current_shader.texture_position);

	// load texture
	glBindTexture(GL_TEXTURE_2D, texture.id);

	// finally draw
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

	glDisableVertexAttribArray(current_shader.vertex_position);
	glDisableVertexAttribArray(current_shader.texture_position);

	glUseProgram(0); // Unbind shader program
}

//**************************************************
// WIN32
//**************************************************

HDC device_context;
HGLRC opengl_context;
bool quit = false;

LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
        case WM_CREATE:
    	{
			if (DEBUG)
				debug_clean();
        
    		device_context = GetDC(hwnd);
            int pixel_format[1];
            unsigned int formatCount;
            PIXELFORMATDESCRIPTOR pixelFormatDescriptor;

    		SetPixelFormat(device_context, 1, &pixelFormatDescriptor);
            opengl_context = wglCreateContext(device_context);
            wglMakeCurrent(device_context, opengl_context);

            load_opengl_extensions();

			wglMakeCurrent(NULL, NULL);
        	wglDeleteContext(opengl_context);
            ReleaseDC(hwnd, device_context);

            device_context = GetDC(hwnd);
            
            int attributes[] =
            {
              WGL_DRAW_TO_WINDOW_ARB, GL_TRUE,
              WGL_SUPPORT_OPENGL_ARB, GL_TRUE,
              WGL_DOUBLE_BUFFER_ARB, GL_TRUE,
              WGL_PIXEL_TYPE_ARB, WGL_TYPE_RGBA_ARB,

              WGL_ACCELERATION_ARB, WGL_FULL_ACCELERATION_ARB,
              WGL_SWAP_METHOD_ARB, WGL_SWAP_EXCHANGE_ARB,
              WGL_PIXEL_TYPE_ARB, WGL_TYPE_RGBA_ARB,

              WGL_COLOR_BITS_ARB, 32,
              WGL_DEPTH_BITS_ARB, 24,
              WGL_STENCIL_BITS_ARB, 8,

              0
            };

            wglChoosePixelFormatARB(device_context, attributes, NULL, 1, pixel_format, &formatCount);
            SetPixelFormat(device_context, pixel_format[0], &pixelFormatDescriptor);
            

            int attributes_version[] =
            {
              WGL_CONTEXT_MAJOR_VERSION_ARB, 2,
              WGL_CONTEXT_MINOR_VERSION_ARB, 0,
              0
            };

    		opengl_context = wglCreateContextAttribsARB(device_context, 0, attributes_version);
    	    wglMakeCurrent(device_context, opengl_context);

            wglSwapIntervalEXT(1); // VSYNC ON

            glDisable(GL_DEPTH_TEST);
            glEnable(GL_BLEND);
            glEnable(GL_TEXTURE0);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            glHint(GL_GENERATE_MIPMAP_HINT, GL_NICEST);

            ShowCursor(SHOW_CURSOR);
            }
            break;
			
        case WM_DESTROY:
            wglDeleteContext(opengl_context);
            ReleaseDC(hwnd, device_context);
            PostQuitMessage(0);
            break;

        case WM_RBUTTONUP:
            DestroyWindow(hwnd);
            break;

        case WM_KEYDOWN:
		{
			input_keys[(unsigned int)wParam] = true;
			
			if (VK_ESCAPE == wParam)
                DestroyWindow(hwnd);
		}
        break;

	    case WM_KEYUP:
		{
			key_any = true;
			input_keys[(unsigned int)wParam] = false;
			released_keys[(unsigned int)wParam] = true;
		}
		break;

        default:
            return DefWindowProc(hwnd, message, wParam, lParam);
    }

    return 0;
}

void center_window(HWND hwnd_self)
{
    HWND hwnd_parent;
    RECT rw_self, rc_parent, rw_parent;
    int xpos, ypos;

    hwnd_parent = GetParent(hwnd_self);
    if (NULL == hwnd_parent)
        hwnd_parent = GetDesktopWindow();

    GetWindowRect(hwnd_parent, &rw_parent);
    GetClientRect(hwnd_parent, &rc_parent);
    GetWindowRect(hwnd_self, &rw_self);

    xpos = rw_parent.left + (rc_parent.right + rw_self.left - rw_self.right) / 2;
    ypos = rw_parent.top + (rc_parent.bottom + rw_self.top - rw_self.bottom) / 2;

    SetWindowPos(
        hwnd_self, NULL,
        xpos, ypos, 0, 0,
        SWP_NOSIZE|SWP_NOZORDER|SWP_NOACTIVATE
        );
}

int APIENTRY WinMain(
        HINSTANCE hInstance,
        HINSTANCE hPrevInstance,
        LPSTR lpCmdLine,
        int nCmdShow
        )
{
    MSG msg;
    WNDCLASS wc;
    HWND hwnd;

    HICON  hWindowIcon = (HICON)LoadImage(NULL, "res/icon.ico", IMAGE_ICON, 16, 16, LR_LOADFROMFILE);
    
    ZeroMemory(&wc, sizeof wc);
    wc.hInstance     = hInstance;
    wc.lpszClassName = APP_NAME;
    wc.lpfnWndProc   = (WNDPROC)WndProc;
    wc.style         = CS_DBLCLKS|CS_VREDRAW|CS_HREDRAW;
    wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    wc.hIcon         = hWindowIcon;
    wc.hCursor       = NULL;
	
    if (FALSE == RegisterClass(&wc))
        return 0;

	// Initialize the message structure.
	ZeroMemory(&msg, sizeof(MSG));
	
    // get full screen size
    int screen_width = GetSystemMetrics(SM_CXSCREEN);
    int screen_height = GetSystemMetrics(SM_CYSCREEN);

    // create the windows
    hwnd = CreateWindow(
        APP_NAME,
        APP_NAME,
		FULL_SCREEN ? 
        WS_POPUP | WS_VISIBLE : // fullscreen
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_CLIPCHILDREN | WS_VISIBLE, // windowed
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        FULL_SCREEN ? screen_width : screen_width / 3 * 2,
        FULL_SCREEN ? screen_height : screen_height / 3 * 2,
        0,
        0,
        hInstance,
        0);

    if (NULL == hwnd)
        return 0;

	if (! FULL_SCREEN)
		center_window(hwnd);
	
    base_shader = load_shader_verbose(direct_vs, direct_fs);
    current_shader = base_shader;
    game_init(); // after window created and opengl context	

    const int SKIP_TICKS = 1000 / FRAMES_PER_SECOND;
    long next_game_tick = GetTickCount();
    int sleep_time = 0;

	srand(next_game_tick);
	
	while (!quit)
	{
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		if (msg.message == WM_QUIT)
			quit = true;
		
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glClearColor(0.14f, 0.14f, 0.14f, 0); // #2e2e2e

        game_tick(1.f); // delta time

        glFinish();
        SwapBuffers(device_context);

		memset(&released_keys, 0, sizeof(released_keys));
		key_any = false;
		
        next_game_tick += SKIP_TICKS;
        sleep_time = next_game_tick - GetTickCount();
        
        if (sleep_time >= 0)
            Sleep(sleep_time);
        //else
        //   debug("Shit, we are running behind!");     
    }

    game_terminate();
    unload_shader(base_shader);

    return msg.wParam;
}
