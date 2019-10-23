#include "libmin.h"
#include <stdlib.h>
#include <stdio.h>

int main(int argc, char **argv) {
				int *a = (int *) malloc((argc - 1) * sizeof (int));
				int i;
				for (i = 0; i < argc - 1; i++) {
								a[i] = atoi(argv[i + 1]);
				}
				printf("0. Min value of array = %d\n", min(a, argc - 1));
				free(a);
				return (0);
}
