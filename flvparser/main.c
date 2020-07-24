#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <memory.h>

#include "io.h"
#include "flv.h"
#include "ts.h"

int main(int argc, char** argv)
{
	if(argc < 2){
		printf("no input file.\n");
		return 0;
	}

	const char* input = argv[1];
	IOContext* ctx = open_input(input);
	if(ctx == NULL){
		printf("open input file failed. filename = %s\n", input);
		return 0;
	}

	parse_flv(ctx);
	// parse_ts(ctx);


	close_input(ctx);

	return 0;
}