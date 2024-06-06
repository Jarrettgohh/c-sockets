#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int main()
{



//char hex_l = 0x6c;
char text[20] ="hello friend";

int text_len = strlen(text);
char *hex_output = malloc(text_len*2 + 1); // 2 hex = 1 ASCII char + 1 for null char

if(hex_output == NULL)
{
	perror("Failed to allocated memory.");
	return 1;
}

int hex_index = 0;

for (int i = 0; i < strlen(text); i++)
{
	char t = text[i];
	
	int w = sprintf(&hex_output[hex_index], "%x", t);
	
	if(w < 0)
	{
		perror("Failed to convert to hex.");
		return 1;
	}

	hex_index += 2;

}


//printf("%s\n", hex_output);
hex_output[strlen(hex_output)] = 0x0;
printf("%s\n", hex_output);

//printf("%li\n", long_l);
//sprintf(hex_output, "%lx", long_l);




//printf("long int: %i\n", (int)long_l);
//printf("hex: %s\n", hex_output);

//char **endptr;

//long hex_int = strtol(hex_output, endptr, 16);

//printf("original str from hex: %li\n", hex_int);


return 0;
}


