/**
 * @file    PNGLoader.h
 * @brief	PNG image loader helper
 *
 * @author	Roberto Cano (http://www.robertocano.es)
 */
#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

int loadPNG(const char *filename, uint8_t **image, uint32_t *width, uint32_t *height, uint32_t *bytesPerPixel);

#ifdef __cplusplus
}
#endif
