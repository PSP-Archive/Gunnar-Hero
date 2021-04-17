#ifndef GRAPHICS_H
#define GRAPHICS_H

#include <psptypes.h>
#include "loadpng.h"

#define	PSP_LINE_SIZE 512
#define SCREEN_WIDTH 480
#define SCREEN_HEIGHT 272

typedef u32 Color;
#define A(color) ((u8)(color >> 24 & 0xFF))
#define B(color) ((u8)(color >> 16 & 0xFF))
#define G(color) ((u8)(color >> 8 & 0xFF))
#define R(color) ((u8)(color & 0xFF))

#if 0
typedef struct
{
	int textureWidth;  // the real width of data, 2^n with n>=0
	int textureHeight;  // the real height of data, 2^n with n>=0
	int imageWidth;  // the image width
	int imageHeight;
	Color* data;
	int swizzled;
} Image;
#endif
/**
 * Load a PNG image.
 *
 * @pre filename != NULL
 * @param filename - filename of the PNG image to load
 * @return pointer to a new allocated Image struct, or NULL on failure
 */
extern Image* loadImage(const char* filename);
extern void freeImage(Image *image);

/**
 * Save an image or the screen in PNG format.
 *
 * @pre filename != NULL
 * @param filename - filename of the PNG image
 * @param data - start of Color type pixel data (can be getVramDisplayBuffer())
 * @param width - logical width of the image or SCREEN_WIDTH
 * @param height - height of the image or SCREEN_HEIGHT
 * @param lineSize - physical width of the image or PSP_LINE_SIZE
 * @param saveAlpha - if 0, image is saved without alpha channel
 */
extern void saveImage(const char* filename, Color* data, int width, int height, int lineSize, int saveAlpha);

#endif
