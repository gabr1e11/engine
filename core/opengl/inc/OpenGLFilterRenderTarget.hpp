/**
 * @class	OpenGLFilterRenderTarget
 * @brief	Render target for OpenGL. A render target allows to render objects to it
 *          instead of to the main screen. Then the target can be rendered to the main screen as
 *          a texture
 *
 *          The Filter render target applies no anti-aliasing
 *
 * @author	Roberto Cano (http://www.robertocano.es)
 */
#pragma once

#include "OpenGL.h"
#include "RenderTarget.hpp"
#include "Shader.hpp"

#pragma warning( disable : 4250 )

class OpenGLFilterRenderTarget : public virtual RenderTarget
{
	public:
        ~OpenGLFilterRenderTarget();
        bool init(uint32_t width, uint32_t height, uint32_t maxSamples);
        void bind();
        void bindDepth();
        void unbind();
        bool blit(uint32_t dstX, uint32_t dstY, uint32_t width, uint32_t height, bool bindMainFB = true);
        void clear();

    protected:
		virtual bool customInit(void) = 0;
		virtual void setCustomParams(void) = 0;
		virtual void unsetCustomParams(void) = 0;

        /**
         * Frame buffer object ID to reference
         * both the color buffer and the depth buffer
         */
        GLuint _frameBuffer;

        /**
         * Frame buffer texture to hold the color buffer
         */
        GLuint _colorBuffer;

        /**
         * Render buffer object to hold the depth buffer
         */
        GLuint _depthBuffer;

        /**
         * Render target vertices buffer
         */
        GLuint _vertexArray;
        GLuint _vertexBuffer;

        /**
         * Shader for the target rendering to screen
         */
        Shader *_shader;
};
