#include <exception>
#include <string>
#include <iostream>

#include <SDL2/SDL.h>
#include <GL/glew.h>
#include <SDL2/SDL_opengl.h>
#include <png.h>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/filesystem.hpp>
#include <boost/format.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "mesh.h"


using namespace glm;
using namespace XMLMesh;

#define VERTEX_POSITION_INDEX 0
#define VERTEX_NORMAL_INDEX 1
#define VERTEX_TEXCOORDS_INDEX 2


class MessageError: public std::exception
{
private:
    std::string message;
public:
    MessageError(const std::string &msg)
    {
        message = msg;
    }
    MessageError(const boost::format &fmt)
    {
        message = fmt.str();
    }

    const char *what(void) const noexcept
    {
        return message.c_str();
    }
};

class InitError: public MessageError
{
public:
    InitError(const std::string &msg): MessageError(msg) {}
    InitError(const boost::format &fmt): MessageError(fmt) {}
};

class PNGError: public MessageError
{
public:
    PNGError(const std::string &msg): MessageError(msg) {}
    PNGError(const boost::format &fmt): MessageError(fmt) {}
};

class ShaderError: public MessageError
{
public:
    ShaderError(const std::string &msg): MessageError(msg) {}
    ShaderError(const boost::format &fmt): MessageError(fmt) {}
};

class RenderError: public MessageError
{
public:
    RenderError(const std::string &msg): MessageError(msg) {}
    RenderError(const boost::format &fmt): MessageError(fmt) {}
};

class TextureError: public MessageError
{
public:
    TextureError(const std::string &msg): MessageError(msg) {}
    TextureError(const boost::format &fmt): MessageError(fmt) {}
};

class GLError : public std::exception
{
private:
    std::string message;
public:
    GLError(const GLenum err, const char *filename, const size_t lineNumber)
    {
        switch (err)
        {
        case GL_NO_ERROR:
            message = (boost::format("GL_NO_ERROR at %1% line %2%") % filename % lineNumber).str();
            break;
        case GL_INVALID_ENUM:
            message = (boost::format("GL_INVALID_ENUM at %1% line %2%") % filename % lineNumber).str();
            break;
        case GL_INVALID_VALUE:
            message = (boost::format("GL_INVALID_VALUE at %1% line %2%") % filename % lineNumber).str();
            break;
        case GL_INVALID_OPERATION:
            message = (boost::format("GL_INVALID_OPERATION at %1% line %2%") % filename % lineNumber).str();
            break;
        case GL_INVALID_FRAMEBUFFER_OPERATION:
            message = (boost::format("GL_INVALID_FRAMEBUFFER_OPERATION at %1% line %2%") % filename % lineNumber).str();
            break;
        case GL_OUT_OF_MEMORY:
            message = (boost::format("GL_OUT_OF_MEMORY at %1% line %2%") % filename % lineNumber).str();
            break;
        case GL_STACK_UNDERFLOW:
            message = (boost::format("GL_STACK_UNDERFLOW at %1% line %2%") % filename % lineNumber).str();
            break;
        case GL_STACK_OVERFLOW:
            message = (boost::format("GL_STACK_OVERFLOW at %1% line %2%") % filename % lineNumber).str();
            break;
        default:
            message = (boost::format("glGetError: %1$#x at %2% line %3%") % err % filename % lineNumber).str();
            break;
        }
    }

    const char *what(void) const noexcept
    {
        return message.c_str();
    }
};

void CheckGL(const char *filename, const size_t lineNumber)
{
    GLenum err = glGetError();
    if (err != GL_NO_ERROR)
        throw GLError(err, filename, lineNumber);
};

#define CHECKGL() CheckGL(__FILE__, __LINE__);


GLuint CreateShader(const std::string &source, GLenum type)
{
    GLint result;
    int logLength;

    GLuint shader = glCreateShader(type);
    CHECKGL();

    const char *pSource = source.c_str();
    glShaderSource(shader, 1, &pSource, NULL);
    CHECKGL();

    glCompileShader(shader);
    CHECKGL();

    glGetShaderiv(shader, GL_COMPILE_STATUS, &result);
    CHECKGL();

    if (result != GL_TRUE)
    {
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLength);
        CHECKGL();

        char *errorString = new char[logLength + 1];
        glGetShaderInfoLog(shader, logLength, NULL, errorString);
        CHECKGL();

        ShaderError error(boost::format("error while compiling shader: %1%") % errorString);
        delete[] errorString;

        glDeleteShader(shader);
        CHECKGL();

        throw error;
    }

    return shader;
}

GLuint LinkShaderProgram(const GLuint vertexShader,
                         const GLuint fragmentShader,
                         const std::map<size_t, std::string> &vertexAttribLocations)
{
    GLint result;
    int logLength;
    GLuint program;

    program = glCreateProgram();
    CHECKGL();

    glAttachShader(program, vertexShader);
    CHECKGL();

    glAttachShader(program, fragmentShader);
    CHECKGL();

    for (const auto &pair : vertexAttribLocations)
    {
        glBindAttribLocation(program, std::get<0>(pair),
                                      std::get<1>(pair).c_str());
        CHECKGL();
    }

    glLinkProgram(program);
    CHECKGL();

    glGetProgramiv(program, GL_LINK_STATUS, &result);
    CHECKGL();

    if (result != GL_TRUE)
    {
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &logLength);
        CHECKGL();

        char *errorString = new char[logLength + 1];
        glGetProgramInfoLog(program, logLength, NULL, errorString);
        CHECKGL();

        ShaderError error(boost::format("error while linking shader: %1%") % errorString);
        delete[] errorString;

        glDeleteProgram(program);
        CHECKGL();

        throw error;
    }

    return program;
}


const char
meshVertexShaderSrc[] = R"shader(
#version 150

in vec3 position;
in vec2 texCoords;
in vec3 normal;

out VertexData
{
    vec3 worldSpaceNormal;
    vec2 texCoords;
} vertexOut;

uniform mat4 modelViewMatrix;
uniform mat4 projectionMatrix;
uniform mat4 normalMatrix;

void main()
{
    gl_Position = projectionMatrix * modelViewMatrix * vec4(position, 1.0);
    vertexOut.texCoords = texCoords;
    vertexOut.worldSpaceNormal = (normalMatrix * vec4(normal, 0.0)).xyz;
}
)shader",

meshFragmentShaderSrc[] = R"shader(
#version 150

uniform sampler2D tex;

// light points down, into the screen and to the right:
const vec3 lightDirection = normalize(vec3(1.0, -1.0, -1.0));

// Dark blue shade:
const vec4 shadeColor = vec4(0.0, 0.0, 0.3, 1.0);

in VertexData
{
    vec3 worldSpaceNormal;
    vec2 texCoords;
} vertexIn;

out vec4 fragColor;

void main()
{
    vec3 n = normalize(vertexIn.worldSpaceNormal);

    // Add some ambient lighting too:
    float f = (2.0 - dot(lightDirection, n)) / 3;

    fragColor = (1.0 - f) * shadeColor + f * texture(tex, vertexIn.texCoords);
}
)shader";


struct MeshRenderVertex
{
    vec3 position,
         normal;
    vec2 texCoords;
};

typedef unsigned int MeshRenderIndex;

class MeshRenderer
{
private:
    GLuint vboID, iboID;

    size_t vertexCount,
           indexCount;

    const MeshState *pMeshState;
public:
    MeshRenderer(const MeshState *pMS): pMeshState(pMS)
    {
        size_t quadCount, triangleCount;
        std::tie(quadCount, triangleCount) = pMeshState->CountQuadsTriangles();

        vertexCount = 4 * quadCount + 3 * triangleCount;
        indexCount = 6 * quadCount + 3 * triangleCount;

        glGenBuffers(1, &vboID);
        CHECKGL();
        glBindBuffer(GL_ARRAY_BUFFER, vboID);
        CHECKGL();
        glBufferData(GL_ARRAY_BUFFER, vertexCount * sizeof(MeshRenderVertex), NULL, GL_DYNAMIC_DRAW);
        CHECKGL();

        glGenBuffers(1, &iboID);
        CHECKGL();
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, iboID);
        CHECKGL();
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indexCount * sizeof(MeshRenderIndex), NULL, GL_DYNAMIC_DRAW);
        CHECKGL();
    }

    ~MeshRenderer(void)
    {
        glDeleteBuffers(1, &vboID);
        CHECKGL();
        glDeleteBuffers(1, &iboID);
        CHECKGL();
    }

    void UpdateBuffer(void)
    {
        glBindBuffer(GL_ARRAY_BUFFER, vboID);
        CHECKGL();
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, iboID);
        CHECKGL();

        MeshRenderVertex *pVertexBuffer = (MeshRenderVertex *)glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
        CHECKGL();
        MeshRenderIndex *pIndexBuffer = (MeshRenderIndex *)glMapBuffer(GL_ELEMENT_ARRAY_BUFFER, GL_WRITE_ONLY);
        CHECKGL();

        size_t vertexNumber = 0,
               indexNumber = 0;

        vec3 faceNormal;
        for (const MeshFace *pFace : pMeshState->IterFaces())
        {
            if (pFace->CountCorners() == 4)
            {
                pIndexBuffer[indexNumber++] = vertexNumber;
                pIndexBuffer[indexNumber++] = vertexNumber + 1;
                pIndexBuffer[indexNumber++] = vertexNumber + 2;
                pIndexBuffer[indexNumber++] = vertexNumber;
                pIndexBuffer[indexNumber++] = vertexNumber + 2;
                pIndexBuffer[indexNumber++] = vertexNumber + 3;
            }
            else if (pFace->CountCorners() == 3)
            {
                pIndexBuffer[indexNumber++] = vertexNumber;
                pIndexBuffer[indexNumber++] = vertexNumber + 1;
                pIndexBuffer[indexNumber++] = vertexNumber + 2;
            }
            else
                throw RenderError(boost::format("encountered a face with %1% corners")
                                  % pFace->CountCorners());

            if (!pFace->IsSmooth())
                faceNormal = CalculateFaceNormal(pFace);

            for (const MeshCorner &corner : pFace->IterCorners())
            {
                pVertexBuffer[vertexNumber].position = corner.GetVertex()->GetPosition();
                pVertexBuffer[vertexNumber].texCoords = corner.GetTexCoords();

                if (pFace->IsSmooth())
                    pVertexBuffer[vertexNumber].normal = CalculateVertexNormal(corner.GetVertex());
                else
                    pVertexBuffer[vertexNumber].normal = faceNormal;

                vertexNumber++;
            }
        }

        glUnmapBuffer(GL_ELEMENT_ARRAY_BUFFER);
        CHECKGL();
        glUnmapBuffer(GL_ARRAY_BUFFER);
        CHECKGL();

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        CHECKGL();
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
        CHECKGL();
    }

    void Render(void)
    {
        glBindBuffer(GL_ARRAY_BUFFER, vboID);
        CHECKGL();
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, iboID);
        CHECKGL();

        // Position
        glEnableVertexAttribArray(VERTEX_POSITION_INDEX);
        CHECKGL();
        glVertexAttribPointer(VERTEX_POSITION_INDEX, 3, GL_FLOAT, GL_FALSE, sizeof(MeshRenderVertex), 0);
        CHECKGL();

        // Normal
        glEnableVertexAttribArray(VERTEX_NORMAL_INDEX);
        CHECKGL();
        glVertexAttribPointer(VERTEX_NORMAL_INDEX, 3, GL_FLOAT, GL_FALSE, sizeof(MeshRenderVertex), (GLvoid *)sizeof(vec3));
        CHECKGL();

        // TexCoords
        glEnableVertexAttribArray(VERTEX_TEXCOORDS_INDEX);
        CHECKGL();
        glVertexAttribPointer(VERTEX_TEXCOORDS_INDEX, 2, GL_FLOAT, GL_FALSE, sizeof(MeshRenderVertex), (GLvoid *)(2 * sizeof(vec3)));
        CHECKGL();

        glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, 0);
        CHECKGL();

        glDisableVertexAttribArray(VERTEX_POSITION_INDEX);
        CHECKGL();
        glDisableVertexAttribArray(VERTEX_NORMAL_INDEX);
        CHECKGL();
        glDisableVertexAttribArray(VERTEX_TEXCOORDS_INDEX);
        CHECKGL();

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        CHECKGL();
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
        CHECKGL();
    }
};

struct PNGImage
{
    png_bytep data;
    png_uint_32 width, height;
    int colorType, bitDepth;
};

class PNGReader
{
private:
    png_struct *pPNG;
    png_info *pInfo,
             *pEnd;
public:
    static void ErrorCallback(png_structp pPNG, png_const_charp message)
    {
        throw PNGError(message);
    }

    PNGReader(void)
    {
        pPNG = png_create_read_struct(PNG_LIBPNG_VER_STRING,
                                      (png_voidp)NULL, (png_error_ptr)ErrorCallback,
                                                       (png_error_ptr)NULL);
        if (pPNG == NULL)
            throw PNGError("error creating png struct");

        pInfo = png_create_info_struct(pPNG);
        if (pInfo == NULL)
            throw PNGError("error creating info struct");

        pEnd = png_create_info_struct(pPNG);
        if (pEnd == NULL)
            throw PNGError("error creating end info struct");
    }

    ~PNGReader(void)
    {
        png_destroy_read_struct(&pPNG, &pInfo, &pEnd);
    }

    static void ReadCallback(png_structp pPNG, png_bytep outData, png_size_t length)
    {
        std::istream *pIs = (std::istream *)png_get_io_ptr(pPNG);

        pIs->read((char *)outData, length);
        std::streamsize n = pIs->gcount();

        if (n < length)
            throw PNGError(boost::format("only %1% bytes read, %2% requested") % n % length);
    }

    void ReadImage(std::istream &is, PNGImage &image)
    {
        // Tell libpng that it must take its data from a callback function.
        png_set_read_fn(pPNG, (png_voidp)&is, ReadCallback);

        // Read info from the header.
        png_read_info(pPNG, pInfo);

        // Get info about the data in the image.
        png_get_IHDR(pPNG, pInfo,
                     &(image.width), &(image.height),
                     &(image.bitDepth), &(image.colorType),
                     NULL, NULL, NULL);

        // Get bytes per row.
        png_uint_32 bytesPerRow = png_get_rowbytes(pPNG, pInfo);

        // Allocate memory to store pixel data in.
        image.data = (png_bytep)png_malloc(pPNG, image.height * bytesPerRow);
        if (image.data == NULL)
            throw PNGError("cannot allocate image data");

        // Make row pointers for reading.
        png_bytepp pRows = new png_bytep[image.height];

        // Place the row pointers correctly.
        for(png_uint_32 i = 0; i < image.height; i++)
            pRows[i] = image.data + (image.height - i - 1) * bytesPerRow;

        // Read the entire image at once.
        try
        {
            png_read_image(pPNG, pRows);
        }
        catch (...)
        {
            png_free(pPNG, image.data);
            delete[] pRows;

            std::rethrow_exception(std::current_exception());
        }

        // At this point, the row pointers aren't needed anymore.
        delete[] pRows;

        // Read the end of the file.
        png_read_end(pPNG, pEnd);
    }

    void DeleteImage(PNGImage &image)
    {
        png_free(pPNG, image.data);
    }
};

GLuint MakePNGTexture(const PNGImage &image)
{
    GLenum format;
    switch (image.colorType)
    {
    case PNG_COLOR_TYPE_RGB:
        format = GL_RGB;
        break;
    case PNG_COLOR_TYPE_RGB_ALPHA:
        format = GL_RGBA;
        break;
    default:
        throw TextureError("Image format is not RGB or RGBA");
    }
    if (image.bitDepth != 8)
    {
        throw TextureError("Image bit depth is not 8");
    }

    GLuint tex;
    glGenTextures(1, &tex);
    CHECKGL();
    if (tex == 0)
        throw TextureError("No texture was allocated");

    glBindTexture(GL_TEXTURE_2D, tex);
    CHECKGL();

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    CHECKGL();
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    CHECKGL();

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,
                 (GLsizei)image.width, (GLsizei)image.height, 0, format,
                 GL_UNSIGNED_BYTE, image.data);
    CHECKGL();

    glGenerateMipmap(GL_TEXTURE_2D);
    CHECKGL();

    return tex;
}


class DemoApp
{
private:
    SDL_Window *mainWindow;
    SDL_GLContext mainGLContext;
    MeshState *pMeshState;
    MeshRenderer *pRenderer;
    GLuint shaderProgram;
    GLuint tex;

    bool running;
    float angle;
    std::unordered_map<std::string, MeshBoneTransformation> mBoneTransformations;
public:
    void Init(const MeshData *pMeshData, const PNGImage &image)
    {
        int error = SDL_Init(SDL_INIT_EVERYTHING);
        if (error != 0)
            throw InitError(boost::format("Unable to initialize SDL: %1%") % SDL_GetError());

        SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
        SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
        SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
        SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);
        SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
        SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
        SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
        SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
        SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 4);

        Uint32 flags = SDL_WINDOW_SHOWN | SDL_WINDOW_OPENGL;
        mainWindow = SDL_CreateWindow("Mesh Test",
                                      SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                                      800, 600, flags);
        if (!mainWindow)
            throw InitError(boost::format("SDL_CreateWindow failed: %1%") % SDL_GetError());

        mainGLContext = SDL_GL_CreateContext(mainWindow);
        if (!mainGLContext)
            throw InitError(boost::format("Failed to create a GL context: %1%") % SDL_GetError());

        GLenum err = glewInit();
        if (GLEW_OK != err)
            throw InitError(boost::format("glewInit failed: %1%") % glewGetErrorString(err));

        if (!GLEW_VERSION_3_2)
            throw InitError("OpenGL 3.2 is not supported");

        pMeshState = DeriveMeshState(pMeshData);

        pRenderer = new MeshRenderer(pMeshState);

        GLuint vertexShader = CreateShader(meshVertexShaderSrc, GL_VERTEX_SHADER),
               fragmentShader = CreateShader(meshFragmentShaderSrc, GL_FRAGMENT_SHADER);

        std::map<size_t, std::string> vertexAttribLocations;
        vertexAttribLocations[VERTEX_POSITION_INDEX] = "position";
        vertexAttribLocations[VERTEX_NORMAL_INDEX] = "normal";
        vertexAttribLocations[VERTEX_TEXCOORDS_INDEX] = "texCoords";

        shaderProgram = LinkShaderProgram(vertexShader, fragmentShader, vertexAttribLocations);

        glDeleteShader(vertexShader);
        CHECKGL();

        glDeleteShader(fragmentShader);
        CHECKGL();

        tex = MakePNGTexture(image);
    }

    void Destroy(void)
    {
        DestroyMeshState(pMeshState);

        glDeleteTextures(1, &tex);

        glDeleteProgram(shaderProgram);

        delete pRenderer;

        SDL_GL_DeleteContext(mainGLContext);
        SDL_DestroyWindow(mainWindow);
        SDL_Quit();
    }

    void HandleEvent(const SDL_Event &event)
    {
        if (event.type == SDL_QUIT)
            running = false;
    }

    void Render(void)
    {
        int screenWidth, screenHeight;
        SDL_GL_GetDrawableSize(mainWindow, &screenWidth, &screenHeight);

        mat4 matProjection, matView, matNormal;


        matProjection = perspective(pi<GLfloat>() / 4, GLfloat(screenWidth) / GLfloat(screenHeight), 0.1f, 1000.0f);

        matView = rotate(translate(matView,  vec3(0.0f, 0.0f, -10.0f)), angle, vec3(0.0f, 1.0f, 0.0f));

        matNormal = transpose(inverse(matView));

        glEnable(GL_CULL_FACE);
        CHECKGL();

        glEnable(GL_DEPTH_TEST);
        CHECKGL();

        glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
        CHECKGL();

        glClearDepth(1.0f);
        CHECKGL();

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        CHECKGL();

        glUseProgram(shaderProgram);
        CHECKGL();

        GLint projectionMatrixLocation,
              modelViewMatrixLocation,
              normalMatrixLocation;

        projectionMatrixLocation = glGetUniformLocation(shaderProgram, "projectionMatrix");
        CHECKGL();
        if (projectionMatrixLocation == -1)
            throw RenderError("projection matrix location is -1");

        modelViewMatrixLocation = glGetUniformLocation(shaderProgram, "modelViewMatrix");
        CHECKGL();
        if (modelViewMatrixLocation == -1)
            throw RenderError("model view matrix location is -1");

        normalMatrixLocation = glGetUniformLocation(shaderProgram, "normalMatrix");
        CHECKGL();
        if (normalMatrixLocation == -1)
            throw RenderError("normal matrix location is -1");

        glUniformMatrix4fv(projectionMatrixLocation, 1, GL_FALSE, value_ptr(matProjection));
        CHECKGL();

        glUniformMatrix4fv(modelViewMatrixLocation, 1, GL_FALSE, value_ptr(matView));
        CHECKGL();

        glUniformMatrix4fv(normalMatrixLocation, 1, GL_FALSE, value_ptr(matNormal));
        CHECKGL();

        glActiveTexture(GL_TEXTURE0);
        CHECKGL();

        glBindTexture(GL_TEXTURE_2D, tex);
        CHECKGL();

        pRenderer->UpdateBuffer();
        pRenderer->Render();
    }

    void RunDemo(const MeshData *pMeshData,
                 const PNGImage &image, const char *animationName)
    {
        Init(pMeshData, image);

        boost::posix_time::ptime tStart = boost::posix_time::microsec_clock::local_time(),
                                 tNow;
        boost::posix_time::time_duration delta;
        running = true;
        while (running)
        {
            try
            {
                SDL_Event event;
                while (SDL_PollEvent(&event)) {
                    HandleEvent(event);
                }

                tNow = boost::posix_time::microsec_clock::local_time();
                delta = (tNow - tStart);

                angle = float(delta.total_milliseconds()) / 1000;

                GetBoneTransformationsAt(pMeshData, animationName,
                                         delta.total_milliseconds(), 25.0f, true,
                                         mBoneTransformations);

                ApplyBoneTransformations(pMeshData, mBoneTransformations, pMeshState);

                Render();

                SDL_GL_SwapWindow(mainWindow);
            }
            catch (...)
            {
                Destroy();
                std::rethrow_exception(std::current_exception());
            }
        }

        Destroy();
    }
};

int main(int argc, char **argv)
{
    if (argc != 4)
    {
        std::cout << "Usage: " << argv[0]
                  << " mesh_file png_file animation_name" << std::endl;
        return 1;
    }

    boost::filesystem::path meshPath = argv[1];
    if (!boost::filesystem::is_regular_file(meshPath))
    {
        std::cerr << "No such file: "<< argv[1] << std::endl;
        return 1;
    }

    boost::filesystem::ifstream meshFile;
    meshFile.open(meshPath);
    if (!meshFile.good())
    {
        std::cerr << "Error opening " << argv[1] << std::endl;
        return 1;
    }

    MeshData* pMeshData = NULL;
    try
    {
        pMeshData = ParseMeshData(meshFile);
    }
    catch (const std::exception &e)
    {
        meshFile.close();

        std::cerr << e.what() << std::endl;
        return 1;
    }
    meshFile.close();

    boost::filesystem::path imagePath = argv[2];
    if (!boost::filesystem::is_regular_file(imagePath))
    {
        std::cerr << "No such file: "<< argv[2] << std::endl;
        return 1;
    }

    boost::filesystem::ifstream textureFile;
    textureFile.open(imagePath, std::ios::binary);
    if (!textureFile.good())
    {
        std::cerr << "Error opening " << argv[2] << std::endl;
        return 1;
    }

    PNGImage image;
    PNGReader imageReader;
    imageReader.ReadImage(textureFile, image);
    textureFile.close();

    try
    {
        DemoApp app;
        app.RunDemo(pMeshData, image, argv[3]);
    }
    catch (const std::exception &e)
    {
        DestroyMeshData(pMeshData);
        imageReader.DeleteImage(image);

        std::cerr << e.what() << std::endl;
        return 1;
    }
    DestroyMeshData(pMeshData);
    imageReader.DeleteImage(image);
    return 0;
}
