#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

struct elem
{
				int val;
				struct elem *next;
};

void print_argv( char **argv )
{
				if( *argv != NULL )
				{
								printf("%s\n", *argv);
								argv++;
								print_argv( argv );
				}
}

int main( int argc, char **argv )
{
				//1 pointer to self
				void *p = &p;

				printf("1.\nAddress of the pointer: %p\nValue of the pointer: %p\n\n", &p, p );


				//2 print args
				printf("2.\nNumber of arguments: %d\nArguments:\n", argc);
				
				//while
				printf("using while loop\n");
				int i = 0;
				while( i < argc )
				{
								printf("%s\n", argv[i]);
								i++;
				}
				
				//for
				printf("\nusing for loop\n");
				
				for( i = 0; i < argc; i++ )
								printf("%s\n", argv[i]);

				//recursion
				printf("\nusing recursion\n");
				print_argv(argv);
				
				//3 detect endianness
				short x = 0x1234;
				if( *(char *)&x == 0x34 )
								printf("\n3.\nLittle Endian\n\n");
				else
								printf("\n3.\nBig Endian\n\n");
				
				//4 penultimate line of stdin
				char *s[3];
				s[0] = (char*)malloc(100*sizeof(char));
				s[1] = (char*)malloc(100*sizeof(char));
				s[2] = (char*)malloc(100*sizeof(char));

				int len = 0;
				int k = 0;
				while( (len = getline(&s[2], &len, stdin)) > 1 )
				{
								if( k > 0 )
												strcpy(s[0],s[1]);
								strcpy(s[1],s[2]);
								k++;
				}
				if( k > 0 )
								printf("4.\nPenultimate line:\n%s\n", s[0]);
				else
								printf("4.\nNo penultimate line\n\n");

				free(s[0]);
				free(s[1]);
				free(s[2]);

				//5 argv linked list
				
				printf("5\nArgument list:\n");
				struct elem *head = NULL;
				for( i = 1; i < argc; i++ )
				{
								struct elem *cur = (struct elem*)malloc(sizeof(struct elem));
								cur->val = atoi(argv[i]);
								cur->next = head;
								head = cur;
				}

				struct elem *temp = head;
				while( temp != NULL )
				{
								printf("%d ", temp->val);
								temp = temp->next;
				}

				while( head != NULL )
				{
								temp = head;
								head = head->next;
								free( temp );
				}
				printf("\n");

				return 0;
}
