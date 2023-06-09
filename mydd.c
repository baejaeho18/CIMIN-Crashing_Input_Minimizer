#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <signal.h>

char output_path[30];
char error_keyword[30];
char* target_p[5];
int latest_reduced_input_size;
char* latest_reduced_input;

char*
reduce(char* original_input)
{
    latest_reduced_input_size = strlen(original_input);
    latest_reduced_input = original_input;
    // Used to store two ends of pipe
    int p2c[2];
    int c2p[2];
    // Used to store process id generated by fork()
    pid_t pid;

    int input_size = strlen(original_input) - 1;
    char* reduced_input;
    char* error_message = (char*)malloc(sizeof(char) * 4096); // pipe read로 이동

    // make pipe
    if (pipe(p2c) == -1 || pipe(c2p) == -1) 
    {
        fprintf(stderr, "Pipe Failed");
        return NULL;
    }

    while(input_size > 0)
    {
        printf("%d\n", input_size);
        for(int i=0; i<=strlen(original_input) - input_size; i++)
        {
            char* head;
            char* tail;
            if(0 > i-1)
                head = "";
            else{
                head = (char*)malloc(sizeof(char) * i);
                strncpy(head, original_input, i);
            }
            if(i+input_size > strlen(original_input)-1)
                tail = "";
            else{    // i = 0
                tail = (char*)malloc(sizeof(char) * (strlen(original_input)-i-input_size));
                strncpy(tail, original_input+i+input_size, strlen(original_input)-i-input_size);
            }

            // for test
            printf("(%d) head: %s, tail: %s\n", i, head, tail);

            reduced_input = (char*)malloc(sizeof(char) * (strlen(original_input)-input_size-1));
            strcpy(reduced_input, head);
            strcat(reduced_input, tail);
            printf("REDUCED: %s\n", reduced_input);

            free(head);
            printf("HI!!!\n");
            free(tail);

            // for test
            printf("(%d) reduced input:%s\n", i, reduced_input);

            // fork - pipe - write(head+tail);
            if ((pid = fork()) < 0)
            {
                fprintf(stderr, "fork Failed");
                return NULL;
            }
            else if (pid > 0) // Parent process
            {
                close(p2c[0]);
                close(c2p[1]);
                dup2(p2c[1], 0);
                write(p2c[1], reduced_input, strlen(reduced_input) + 1);
                alarm(3);
                // Wait for child to send a string
                wait(NULL);

                read(c2p[0], error_message, 100);
                alarm(0);
                close(p2c[1]);
                close(c2p[0]);
                printf("parent: %s\n", error_message);

                // check if error_message contains error_keyword
                if(strstr(error_message, error_keyword) != NULL)
                    break;
                else
                    return reduce(reduced_input);
            }
            else 
            {   // Child process
                dup2(c2p[1], 2);
                execlp(target_p[0], target_p[0], target_p[1], target_p[2], target_p[3], target_p[4], NULL);
            }
        }
        for(int i=0; i<=strlen(original_input)-input_size; i++)
        {
            char* mid;
            if(i > i+input_size-1)
            {
                mid = "";
            }
            else
            {
                mid = (char*)malloc(sizeof(char) * input_size);
                strncpy(mid, original_input+i, input_size);
            }

            strcat(reduced_input, mid);
            free(mid);
            printf("(%d) reduced_input:%s\n", i, reduced_input);

            // fork - pipe - write(mid)
            if ((pid = fork()) < 0)
            {
                fprintf(stderr, "fork Failed");
                return NULL;
            }
            else if (pid > 0) // Parent process
            {
                close(p2c[0]);
                close(c2p[1]);
                dup2(p2c[1], 0);
                write(p2c[1], reduced_input, strlen(reduced_input) + 1);
                alarm(3);
                // Wait for child to send a string
                wait(NULL);

                read(c2p[0], error_message, 100);
                alarm(0);
                close(p2c[1]);
                close(c2p[0]);
                printf("parent: %s\n", error_message);

                // check if error_message contains error_keyword
                if(strstr(error_message, error_keyword) != NULL)
                    break;
                else
                    return reduce(reduced_input);
            }
            else // Child process
            { 
                dup2(c2p[1], 2);
                execlp(target_p[0], target_p[0], target_p[1], target_p[2], target_p[3], target_p[4], NULL);
            }
        }
        input_size = input_size - 1;
    }
    free(reduced_input);
    free(error_message);

    return original_input;
}

int
main()
{
    char* crashing_input = "jsmn/testcases/crash.json";
    reduce(crashing_input);
}