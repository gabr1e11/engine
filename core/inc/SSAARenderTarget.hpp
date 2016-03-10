/**
 * @class	SSAARenderTarget
 * @brief	A render target allows to render objects to it instead of
 *          to the main screen. The target can then be blitted onto the
 *          screen
 *
 *          The SSAA render target applies no anti-aliasing
 *
 * @author	Roberto Cano (http://www.robertocano.es)
 */
#pragma once

#include "RenderTarget.hpp"

class SSAARenderTarget : public RenderTarget
{
	public:
        static SSAARenderTarget *New();
        static void Delete(SSAARenderTarget *target);

        virtual ~SSAARenderTarget() {}

        virtual bool init(uint32_t width, uint32_t height, uint32_t factor) = 0;
        virtual void bind() = 0;
        virtual void unbind() = 0;
        virtual void clear(float r, float g, float b, float a) = 0;
        virtual bool blit(uint32_t dstX, uint32_t dstY, uint32_t width, uint32_t height) = 0;
};
