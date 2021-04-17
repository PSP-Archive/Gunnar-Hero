#include <stdlib.h>
#include <malloc.h>
#include <png.h>

#include "loadpng.h"

#ifdef DMALLOC
#include"dmalloc.h"
#endif

#define MAX(X, Y) ((X) > (Y) ? (X) : (Y))

static int getNextPower2(int width)
{
	int b = width;
	int n;
	for (n = 0; b != 0; n++) b >>= 1;
	b = 1 << n;
	if (b == 2 * width) b >>= 1;
	return b;
}

static void user_warning_fn(png_structp png_ptr, png_const_charp warning_msg)
{
	//printf("PNGERROR: %s\n",warning_msg);
}

Image* loadImage(const char* filename)
{
	png_structp png_ptr;
	png_infop info_ptr;
	unsigned int sig_read = 0;
	png_uint_32 width, height;
	int bit_depth, color_type, interlace_type, x, y;
	unsigned int* line;
	FILE *fp;
	Image* image = (Image*) malloc(sizeof(Image));
	if (!image) return NULL;
	image->swizzled=0;

printf("Loading image '%s'\n",filename);
#if 1
	char path[256];
	if(strstr(filename,".tga") || strstr(filename,".TGA")) {
		strcpy(path,filename);
		char *s=strstr(path,".tga");
		if(!s) s=strstr(path,".TGA");
		strcpy(s,".PNG");
		filename=path;
		printf("Actually loading image %s\n",filename);
	}
	if(strstr(filename,".bmp") || strstr(filename,".BMP")) {
		strcpy(path,filename);
		char *s=strstr(path,".bmp");
		if(!s) s=strstr(path,".BMP");
		strcpy(s,".PNG");
		filename=path;
		printf("Actually loading image %s\n",filename);
	}
	if(strstr(filename,".jpg") || strstr(filename,".JPG")) {
		strcpy(path,filename);
		char *s=strstr(path,".jpg");
		if(!s) s=strstr(path,".JPG");
		strcpy(s,".PNG");
		filename=path;
		printf("Actually loading image %s\n",filename);
	}
	if(strstr(filename,".gif") || strstr(filename,".GIF")) {
		strcpy(path,filename);
		char *s=strstr(path,".gif");
		if(!s) s=strstr(path,".GIF");
		strcpy(s,".PNG");
		filename=path;
		printf("Actually loading image %s\n",filename);
	}
#endif

	if ((fp = fopen(filename, "rb")) == NULL) return NULL;
	png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if (png_ptr == NULL) {
		free(image);
		fclose(fp);
		return NULL;;
	}
	png_set_error_fn(png_ptr, (png_voidp) NULL, (png_error_ptr) NULL, user_warning_fn);
	info_ptr = png_create_info_struct(png_ptr);
	if (info_ptr == NULL) {
		free(image);
		fclose(fp);
		png_destroy_read_struct(&png_ptr, png_infopp_NULL, png_infopp_NULL);
		return NULL;
	}
	png_init_io(png_ptr, fp);
	png_set_sig_bytes(png_ptr, sig_read);
	png_read_info(png_ptr, info_ptr);
	png_get_IHDR(png_ptr, info_ptr, &width, &height, &bit_depth, &color_type, &interlace_type, int_p_NULL, int_p_NULL);
	if (width > 512 || height > 512) {
		free(image);
		fclose(fp);
		png_destroy_read_struct(&png_ptr, png_infopp_NULL, png_infopp_NULL);
		return NULL;
	}
	image->imageWidth = width;
	image->imageHeight = height;
	image->textureWidth = getNextPower2(width);
	image->textureHeight = getNextPower2(height);
	png_set_strip_16(png_ptr);
	png_set_packing(png_ptr);
	if (color_type == PNG_COLOR_TYPE_PALETTE) png_set_palette_to_rgb(png_ptr);
	if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8) png_set_gray_1_2_4_to_8(png_ptr);
	if (color_type == PNG_COLOR_TYPE_GRAY) png_set_gray_to_rgb(png_ptr);
	if (png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS)) png_set_tRNS_to_alpha(png_ptr);
	png_set_filler(png_ptr, 0xff, PNG_FILLER_AFTER);
	//image->data = (Color*) memalign(16, image->textureWidth * image->textureHeight * sizeof(Color));
	image->data = (Color*) malloc( image->textureWidth * image->textureHeight * sizeof(Color));
	if (!image->data) {
		free(image);
		fclose(fp);
		png_destroy_read_struct(&png_ptr, png_infopp_NULL, png_infopp_NULL);
		return NULL;
	}
	line = (unsigned int *) malloc(width * 4);
	if (!line) {
		free(image->data);
		free(image);
		fclose(fp);
		png_destroy_read_struct(&png_ptr, png_infopp_NULL, png_infopp_NULL);
		return NULL;
	}
	for (y = 0; y < height; y++) {
		png_read_row(png_ptr, (unsigned char *) line, png_bytep_NULL);
		for (x = 0; x < width; x++) {
			unsigned int color = line[x];
			image->data[x + y * image->textureWidth] =  color;
		}
	}
	free(line);
	png_read_end(png_ptr, info_ptr);
	png_destroy_read_struct(&png_ptr, &info_ptr, png_infopp_NULL);
	fclose(fp);
printf("Loaded %s (%08x)\n",filename,image);
	return image;
}

void freeImage(Image *image)
{
#if 1
	if(!image) return;
	if(image->data) free(image->data);
	image->data=0;
	free(image);
#endif
}

void saveImage(const char* filename, Color* data, int width, int height, int lineSize, int saveAlpha)
{
	png_structp png_ptr;
	png_infop info_ptr;
	FILE* fp;
	int i, x, y;
	unsigned char* line;
	
	if ((fp = fopen(filename, "wb")) == NULL) return;
	png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if (!png_ptr) return;
	info_ptr = png_create_info_struct(png_ptr);
	if (!info_ptr) {
		png_destroy_write_struct(&png_ptr, (png_infopp)NULL);
		return;
	}
	png_init_io(png_ptr, fp);
	png_set_IHDR(png_ptr, info_ptr, width, height, 8,
		saveAlpha ? PNG_COLOR_TYPE_RGBA : PNG_COLOR_TYPE_RGB,
		PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
	png_write_info(png_ptr, info_ptr);
	line = (unsigned char *) malloc(width * (saveAlpha ? 4 : 3));
	for (y = 0; y < height; y++) {
		for (i = 0, x = 0; x < width; x++) {
			Color color = data[x + y * lineSize];
			unsigned char r = color & 0xff; 
			unsigned char g = (color >> 8) & 0xff;
			unsigned char b = (color >> 16) & 0xff;
			unsigned char a = saveAlpha ? (color >> 24) & 0xff : 0xff;
			line[i++] = r;
			line[i++] = g;
			line[i++] = b;
			if (saveAlpha) line[i++] = a;
		}
		png_write_row(png_ptr, line);
	}
	free(line);
	png_write_end(png_ptr, info_ptr);
	png_destroy_write_struct(&png_ptr, (png_infopp)NULL);
	fclose(fp);
}
