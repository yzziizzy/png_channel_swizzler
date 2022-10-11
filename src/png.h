// Public Domain

#include <stdint.h>


typedef struct Bitmap {
	long w,h;
	uint8_t chan_mask;

	uint8_t* data[4];
} Bitmap;






int write_PNG(char* path, unsigned int channels, char* data, unsigned int w, unsigned int h); 
int write_PNG_inverted(char* path, unsigned int channels, char* data, unsigned int w, unsigned int h); 

Bitmap* read_PNG(char* path);

