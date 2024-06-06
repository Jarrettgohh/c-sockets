#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main()
{

	//long dec_msg[10] = {106, 107, 108, 109};
	//long hex_msg[10] = {0x6a, 0x6b, 0x6c, 0x6d};
	long oct_msg[10] = {0152, 0153, 0154, 0155};

	//long hex_long = 106; // char 'j'
	char *str = (char *)malloc(10);

	for (int i = 0; i < 4; i++)
	{
		long msg = (long)oct_msg[i];
		sprintf(&str[i], "%c", (char)msg);
	}

	printf("%s\n", str);

	//sprintf(&hex_str[0], "%c", (char)hex_long);
	//printf("%c\n", hex_str[0]);



	
	//ltoa(hex_long, hex_str, 16);

	return 0;
}


