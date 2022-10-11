// Public Domain 

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#include <unistd.h>

#include "png.h"

#include <png.h>
#include <setjmp.h>





Bitmap* read_PNG(char* path) {
	
	FILE* f;
	png_byte sig[8];
	png_bytep rowPtr;
	long i;
	png_byte colorType;
	png_byte bitDepth;
	
	f = fopen(path, "rb");
	if(!f) {
		fprintf(stderr, "Could not open \"%s\" (readPNG).\n", path);
		return NULL;
	}
	
	fread(sig, 1, 8, f);
	
	if(png_sig_cmp(sig, 0, 8)) {
		fprintf(stderr, "\"%s\" is not a valid PNG file.\n", path);
		fclose(f);
		return NULL;
	}
	
	png_structp readStruct = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if (!readStruct) {
		fprintf(stderr, "Failed to load \"%s\". readPNG Error 1.\n", path);
		fclose(f);
		return NULL;
	}
	
	png_set_option(readStruct, PNG_MAXIMUM_INFLATE_WINDOW, PNG_OPTION_ON);
	
	png_infop infoStruct = png_create_info_struct(readStruct);
	if (!infoStruct) {
		fprintf(stderr, "Failed to load \"%s\". readPNG Error 2.\n", path);
		png_destroy_read_struct(&readStruct, (png_infopp)0, (png_infopp)0);
		fclose(f);
		return NULL;
	};
	
	
	
	Bitmap* bmp = calloc(1, sizeof(*bmp));
	
	// exceptions are evil. the alternative with libPNG is a bit worse. ever heard of return codes libPNG devs?
	if (setjmp(png_jmpbuf(readStruct))) {
		
		fprintf(stderr, "Failed to load \"%s\". readPNG Error 3.\n", path);
		png_destroy_read_struct(&readStruct, (png_infopp)0, (png_infopp)0);
		
		if(bmp->data) free(bmp->data);
		free(bmp);
		fclose(f);
		return NULL;
	}
	
	
	png_init_io(readStruct, f);
	png_set_sig_bytes(readStruct, 8);

	png_read_info(readStruct, infoStruct);

	bmp->w = png_get_image_width(readStruct, infoStruct);
	bmp->h = png_get_image_height(readStruct, infoStruct);
	
	
	// coerce the fileinto 8-bit RGBA        
	
	bitDepth = png_get_bit_depth(readStruct, infoStruct);
	colorType = png_get_color_type(readStruct, infoStruct);
	
	if(colorType == PNG_COLOR_TYPE_PALETTE) {
		png_set_palette_to_rgb(readStruct);
	}
	if(colorType == PNG_COLOR_TYPE_GRAY) {
		png_set_expand_gray_1_2_4_to_8(readStruct);
		png_set_expand(readStruct);
	}
	
	if(bitDepth == 16) {
		png_set_strip_16(readStruct);
	}
	if(bitDepth < 8) {
		png_set_packing(readStruct);
	}
	if(png_get_valid(readStruct, infoStruct, PNG_INFO_tRNS)) {
		png_set_tRNS_to_alpha(readStruct);
	}
	if(colorType == PNG_COLOR_TYPE_GRAY) {
		png_set_gray_to_rgb(readStruct);
		png_set_expand(readStruct);
	}		
	if(colorType == PNG_COLOR_TYPE_GRAY_ALPHA) {
		png_set_gray_to_rgb(readStruct);
	}
	
	if(colorType == PNG_COLOR_TYPE_RGB) {
		png_set_filler(readStruct, 0xffff, PNG_FILLER_AFTER);
	}
	
// 	if(colorType > PNG_COLOR_TYPE_PALETTE) {
//  	png_color_16 background = {.red= 255, .green = 255, .blue = 255};
//  	png_set_background(readStruct, &background, PNG_BACKGROUND_GAMMA_SCREEN, 0, 1.0);
// 	}
//  	png_set_filler(readStruct, 0x00, PNG_FILLER_AFTER);
	
//	printf("interlace: %d\n", png_set_interlace_handling(readStruct));
	
	png_read_update_info(readStruct, infoStruct);
	
	
	// read the data
	for(int i = 0; i < 4; i++) {
		bmp->data[i] = malloc(bmp->w * bmp->h);
	}
	
	rowPtr = malloc(sizeof(png_bytep) * bmp->w);
	
	long cursor = 0;
	
	for(long y = 0; y < bmp->h; y++) {
		png_read_row(readStruct, rowPtr, NULL);
		
		for(long x = 0; x < bmp->w; x++, cursor++) {
			
			for(int i = 0; i < 4; i++) {
				bmp->data[i][cursor] = rowPtr[x*4 + i];
			}
		
		}
		
	}


	
	free(rowPtr);
	png_destroy_read_struct(&readStruct, (png_infopp)0, (png_infopp)0);

	fclose(f);
	
//	printf("Loaded \"%s\".\n", path);
	
	return bmp;
}


// data is densely packed
int write_PNG(char* path, unsigned int channels, char* data, unsigned int w, unsigned int h) {
	
	FILE* f;
	png_byte sig[8];
	png_bytep* rowPtrs;
	int i;
	
	png_structp pngPtr;
	png_infop infoPtr;
	
	int colorTypes[4] = {
		PNG_COLOR_TYPE_GRAY,
		PNG_COLOR_TYPE_GRAY_ALPHA,
		PNG_COLOR_TYPE_RGB,
		PNG_COLOR_TYPE_RGB_ALPHA
	};
	
	int ret = 2;

	//printf("png write | w: %d, h: %d \n", w, h);

	if(channels > 4 || channels < 1) {
		return 3;
	}
	
	// file stuff
	f = fopen(path, "wb");
	if(!f) {
		fprintf(stderr, "Could not open \"%s\" (writePNG).\n", path);
		return 1;
	}
	
	/*
	if(png_sig_cmp(sig, 0, 8)) {
		fprintf(stderr, "\"%s\" is not a valid PNG file.\n", path);
		fclose(f);
		return NULL;
	}
	*/
	// init stuff
	pngPtr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if (!pngPtr) {
		goto CLEANUP1;
	}
	//png_destroy_write_struct (&pngPtr, (png_infopp)NULL);
	infoPtr = png_create_info_struct(pngPtr);
	if (!infoPtr) {
		goto CLEANUP2;
	}
	//if(infoPtr != NULL) png_free_data(pngPtr, infoPtr, PNG_FREE_ALL, -1);
	// header stuff
	if (setjmp(png_jmpbuf(pngPtr))) {
		goto CLEANUP3;
	}
	png_init_io(pngPtr, f);


	if (setjmp(png_jmpbuf(pngPtr))) {
		goto CLEANUP3;
	}
	png_set_IHDR(pngPtr, infoPtr, w, h,
		8, colorTypes[channels - 1], PNG_INTERLACE_NONE,
		PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);
	
	png_write_info(pngPtr, infoPtr);

	rowPtrs = malloc(h * sizeof(png_bytep));
	for(i = 0; i < h; i++) {
		rowPtrs[i] = data + (i * w * channels);
	}
	
	// write data
	if (setjmp(png_jmpbuf(pngPtr))) {
		goto CLEANUP4;
	}
	png_write_image(pngPtr, rowPtrs);

	if (setjmp(png_jmpbuf(pngPtr))) {
		goto CLEANUP4;
	}
	png_write_end(pngPtr, NULL);
	
	// success
	
	ret = 0;

CLEANUP4:
	free(rowPtrs);
	
CLEANUP3:
	if(infoPtr != NULL) png_free_data(pngPtr, infoPtr, PNG_FREE_ALL, -1);
	
CLEANUP2:
	png_destroy_write_struct (&pngPtr, (png_infopp)NULL);
	
CLEANUP1:
	fclose(f);
	
	return ret;
}



// data is densely packed
int write_PNG_inverted(char* path, unsigned int channels, char* data, unsigned int w, unsigned int h) {
	
	FILE* f;
	png_byte sig[8];
	png_bytep* rowPtrs;
	int i;
	
	png_structp pngPtr;
	png_infop infoPtr;
	
	int colorTypes[4] = {
		PNG_COLOR_TYPE_GRAY,
		PNG_COLOR_TYPE_GRAY_ALPHA,
		PNG_COLOR_TYPE_RGB,
		PNG_COLOR_TYPE_RGB_ALPHA
	};
	
	int ret = 2;

	printf("png write | w: %d, h: %d \n", w, h);

	if(channels > 4 || channels < 1) {
		return 3;
	}
	
	// file stuff
	f = fopen(path, "wb");
	if(!f) {
		fprintf(stderr, "Could not open \"%s\" (writePNG).\n", path);
		return 1;
	}
	
	/*
	if(png_sig_cmp(sig, 0, 8)) {
		fprintf(stderr, "\"%s\" is not a valid PNG file.\n", path);
		fclose(f);
		return NULL;
	}
	*/
	// init stuff
	pngPtr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if (!pngPtr) {
		goto CLEANUP1;
	}
	//png_destroy_write_struct (&pngPtr, (png_infopp)NULL);
	infoPtr = png_create_info_struct(pngPtr);
	if (!infoPtr) {
		goto CLEANUP2;
	}
	//if(infoPtr != NULL) png_free_data(pngPtr, infoPtr, PNG_FREE_ALL, -1);
	// header stuff
	if (setjmp(png_jmpbuf(pngPtr))) {
		goto CLEANUP3;
	}
	png_init_io(pngPtr, f);


	if (setjmp(png_jmpbuf(pngPtr))) {
		goto CLEANUP3;
	}
	png_set_IHDR(pngPtr, infoPtr, w, h,
		8, colorTypes[channels - 1], PNG_INTERLACE_NONE,
		PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);
	
	png_write_info(pngPtr, infoPtr);

	rowPtrs = malloc(h * sizeof(png_bytep));
	for(i = 0; i < h; i++) {
		rowPtrs[h-i-1] = data + (i * w * channels);
	}
	
	// write data
	if (setjmp(png_jmpbuf(pngPtr))) {
		goto CLEANUP4;
	}
	png_write_image(pngPtr, rowPtrs);

	if (setjmp(png_jmpbuf(pngPtr))) {
		goto CLEANUP4;
	}
	png_write_end(pngPtr, NULL);
	
	// success
	
	ret = 0;

CLEANUP4:
	free(rowPtrs);
	
CLEANUP3:
	if(infoPtr != NULL) png_free_data(pngPtr, infoPtr, PNG_FREE_ALL, -1);
	
CLEANUP2:
	png_destroy_write_struct (&pngPtr, (png_infopp)NULL);
	
CLEANUP1:
	fclose(f);
	
	return ret;
}




