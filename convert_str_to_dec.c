#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int main()
{

	char c = 'j';
	char i[10];

	sprintf(i+0, "%d", c);
	printf("%s\n", i);

	return 0;
}
