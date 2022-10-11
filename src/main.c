#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "png.h"

void usage(char* exe_name);




int main(int argc, char* argv[]) {
	char* out_path, *pattern;
	char* in_paths[4];
	int in_cnt = 0;
	
	Bitmap* inputs[4];	

	if(argc < 4) {
		usage(argv[0]);
		exit(1);
	}
	
	out_path = argv[1];
	pattern = argv[2];
	
	// collect the input paths
	for(int a = 3; a < argc; a++) {
		in_paths[in_cnt++] = argv[a];
		if(in_cnt >= 4) break;
	} 
	
	Bitmap* out = calloc(1, sizeof(*out));
	
	// load the input data
	for(int i = 0; i < in_cnt; i++) {
		inputs[i] = read_PNG(in_paths[i]);
		
		if(!inputs[i]) {
			fprintf(stderr, "FATAL: Could not read file '%s'\n", in_paths[i]);
			exit(1);
		}
		
		// infer the size and verify they are all identical
		if(i == 0) {
			out->w = inputs[i]->w;
			out->h = inputs[i]->h;
		}
		else {
			if(out->w != inputs[i]->w || out->h != inputs[i]->h) {
				fprintf(stderr, "FATAL: Input image sizes do not match\n");
				exit(1);
			}
		}
	}
	
	// allocate the output bitmap 
	for(int i = 0; i < 4; i++) {
		out->data[i] = calloc(1, out->w * out->h);
	}
	
	int out_channel = 0;
	int in_channel = 0;
	int in_image = 0;
	// now for the fun bit
	for(int q = 0; q == 0 || pattern[q-1]; q++) {
		//  r1,g2,b3,a4
		
		char c = pattern[q];
		if(c >= '1' && c <= '4') {
			in_image = c - '1';
		}
		else if(c == 'r' || c == 'R') in_channel = 0;
		else if(c == 'g' || c == 'G') in_channel = 1;
		else if(c == 'b' || c == 'B') in_channel = 2;
		else if(c == 'a' || c == 'A') in_channel = 3;
		else if(c == ',' || c == 0) {
			printf("Copying channel %d from %s as output channel %d\n", in_channel, in_paths[in_image], out_channel);
			
			memcpy(out->data[out_channel], inputs[in_image]->data[in_channel], out->w * out->h);
			
			out_channel++;	
			in_channel = out_channel;
			in_image = out_channel;
		}
		else if(c == '\'' || c == '"') {
			// do nothing
		}
		else {
			fprintf(stderr, "Unknown character in pattern: '%c'\n", c);
		}
	
	}
	
	
	uint8_t* tmp = malloc(out->w * out->h * 4);
	for(long i = 0; i < out->w * out->h; i++) {
		for(int c = 0; c < 4; c++) {
			tmp[i * 4 + c] = out->data[c][i];
		}
	}
	
	write_PNG(out_path, 4, tmp, out->w, out->h);
	
	
	// no need to free anything.
	

	return 0;
}






void usage(char* exe_name) {
	char* usage = "\
Usage: %s [output_file] [pattern] [input_files...]\n\
";

	printf(usage, exe_name);
}

