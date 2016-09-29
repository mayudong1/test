#ifndef __HELPER__H_
#define __HELPER__H_

char get_printable_char(unsigned char c)
{
	if(c > 128 || c <32)
		return '.';
	else
		return c;
}

void print_hex(char* buffer, int len)
{
	int count_per_line = 16;
	int line = len /count_per_line + 1;

	for(int i=0;i<line;i++)
	{
		for(int j=0;j<count_per_line;j++)
		{
			printf("%02X ", (unsigned char)buffer[i*count_per_line+j]);
		}
		printf("\t");
		for(int j=0;j<count_per_line;j++)
		{
			printf("%c", get_printable_char(buffer[i*count_per_line+j]));
		}
		printf("\n");
	}
}



#endif