/**
 * @class	OpenGLRenderTarget
 * @brief	Render target for OpenGL. A render target allows to render objects to it
 *          instead of to the main screen. Then the target can be rendered to the main screen as
 *          a texture
 *
 * @author	Roberto Sosa Cano (http://www.robertocano.es)
 */
#include "OpenGL.h"
#include "OpenGLRenderTarget.hpp"
#include "Renderer.hpp"

bool OpenGLRenderTarget::init(uint32_t width, uint32_t height)
{
    glActiveTexture(GL_TEXTURE0);

    /* Texture buffer */
    GL( glGenTextures(1, &_colorBuffer) );
    GL( glBindTexture(GL_TEXTURE_2D, _colorBuffer) );
    {
        GL( glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR) );
        GL( glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR) );
        GL( glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE) );
        GL( glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE) );
        GL( glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL) );
    }
    GL( glBindTexture(GL_TEXTURE_2D, 0) );

    /* Depth buffer */
    GL( glGenRenderbuffers(1, &_depthBuffer) );
    GL( glBindRenderbuffer(GL_RENDERBUFFER, _depthBuffer) );
    {
        GL( glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16, width, height) );
    }
    GL( glBindRenderbuffer(GL_RENDERBUFFER, 0) );

    /* Framebuffer to link everything together */
    GL( glGenFramebuffers(1, &_frameBuffer) );
    GL( glBindFramebuffer(GL_FRAMEBUFFER, _frameBuffer) );
    {
        GL( glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, _colorBuffer, 0) );
        GL( glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, _depthBuffer) );

        GLenum status;
        if ((status = glCheckFramebufferStatus(GL_FRAMEBUFFER)) != GL_FRAMEBUFFER_COMPLETE) {
            printf("ERROR OpenGLRenderTarget::init %d\n", status);
            return false;
        }
    }
    GL( glBindFramebuffer(GL_FRAMEBUFFER, 0) );

    /* Generate the render target surface */
    GLfloat verticesData[8] = {
        -1, -1,
        1, -1,
        -1,  1,
        1,  1,
    };

	GL( glGenVertexArrays(1, &_vertexArray) );
	GL( glBindVertexArray(_vertexArray) );
    {
        GL( glGenBuffers(1, &_vertexBuffer) );
        GL( glBindBuffer(GL_ARRAY_BUFFER, _vertexBuffer) );
        {
            GL( glBufferData(GL_ARRAY_BUFFER, sizeof verticesData, verticesData, GL_STATIC_DRAW) );

            GL( glEnableVertexAttribArray(0) );
            GL( glVertexAttribPointer(
                    0,        // attribute
                    2,        // number of elements per vertex, here (x,y)
                    GL_FLOAT, // the type of each element
                    GL_FALSE, // take our values as-is
                    0,        // no extra data between each position
                    0         // offset of first element
                    ) );
        }
        GL( glBindBuffer(GL_ARRAY_BUFFER, 0) );
    }
    GL( glBindVertexArray(0) );

    /* Create the shader */
    _shader = Renderer::GetRenderer()->getShader();

	std::string error;
	if (_shader->loadVertexShader("Shaders/effects/waves.vert", error) == false) {
		printf("ERROR compiling vertex shader: %s\n", error.c_str());
		return 1;
	}
	if (_shader->loadFragmentShader("Shaders/effects/waves.frag", error) == false) {
		printf("ERROR compiling fragment shader: %s\n", error.c_str());
		return 1;
	}
	if (_shader->linkProgram(error) == false) {
		printf("ERROR linking shader: %s\n", error.c_str());
		return 1;
	}

    return true;
}

bool OpenGLRenderTarget::render()
{
    /* Bind the target texture */
    GL( glActiveTexture(GL_TEXTURE0) );
    GL( glBindTexture(GL_TEXTURE_2D, _colorBuffer) );

    /* Tell the shader which texture unit to use */
    _shader->attach();
    _shader->setUniformTexture2D("fbo_texture", 0);

    static uint32_t counter = 0;
    float offset = counter++;
    if (_shader->setUniformFloat("offset", offset) == false) {
        printf("ERROR setting offset uniform\n");
        return false;
    }

    GL( glBindVertexArray(_vertexArray) );
    {
        GL( glDrawArrays(GL_TRIANGLE_STRIP, 0, 4) );
    }
    GL( glBindVertexArray(0) );

    _shader->detach();

    return true;
}

const uint32_t OpenGLRenderTarget::getID()
{
    return _frameBuffer;
}
