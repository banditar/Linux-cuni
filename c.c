#include <stdio.h>
#include <poll.h>
#include <stdlib.h>
#include <time.h>

void six()
{
				int i, n = 20;
				for( i = 0; i < n; i++ )
								(void)printf("-");
				putchar("\n");
}

int main( int argc, char **argv )
{
				char **Argv = argv;
				//1
				int i;
				printf("1.Print args:\n");
				for( i = 0; i < argc; i++ )
								printf("%s\t", argv[i]);
				//2
				printf("\n2.No brackets\n");
				for( i = 0; i < argc; i++ )
								printf("%s\t", *(argv + i));
				//3
				printf("\n3.No local variables\n");
				while( argc > 0 )
								printf("%s\t", *(argv+(argc--)-1));
				//4
				printf("\n4.Not using argc\n");
				while( *argv != NULL )
								printf("%s\t", *argv++);
				printf("\n");
				//5 STAR
				printf("\n5.Moving star\n");
				int k, n = 20, sleep = 20, j, times = 1;
				for( k = 0; k < times; k++ )	//x times
				{
								for( i = 0; i < n; i++ )
								{
												for( j = 0; j < i; j++ )
																printf(" ");
												printf("*");
												printf("\r");				//carriage return
												fflush(stdout);
												poll(NULL, 0, sleep);	//Sleep
								}
								for( j = 0; j <= i; j++ )
												printf(" ");//delete the star
								printf("\r");
								fflush(stdout);

								for( i = 0; i < n; i++ )
								{
												for( j = 0; j < n - i - 1; j++ )
																printf(" ");//backwards
												printf("*");
												printf("\r");
												fflush(stdout);
												poll(NULL, 0, sleep);
										
												for( j = 0; j <= n - i - 1; j++ )
																printf(" ");//delete star
												printf("\r");
												fflush(stdout);
								}
				}
				//6 Correct the code
				six();
				//7 second character of args
				argv = Argv;
				printf("\n7.2nd char of args:\n");
				while( *argv != NULL )
				{
								printf("%c\t", **argv+1);
								argv++;
				}
				printf("\n");
				//8 mountain generator
				int t[10][20] = { 0 };	//0 |1/|2\|3_
				srand(time(NULL));
				printf("%d\n", rand());
				return 0;
}
