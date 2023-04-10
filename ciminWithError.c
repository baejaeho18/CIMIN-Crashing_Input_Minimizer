#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <signal.h>

char output_path[30];
char error_keyword[30];
char* target_program;
int latest_reduced_input_size;
char* latest_reduced_input;

char*
loadInput(char* filename)
{
    FILE *fp;
    char *buffer;
    int file_size;

    fp = fopen(filename, "r"); // open the file in binary mode
    if (fp == NULL)
    {
        printf("Error opening file.\n");
        return NULL;
    }
    // find the size of the file
    fseek(fp, 0, SEEK_END); // seek to the end of the file
    file_size = ftell(fp); // get the current file position (which is the size of the file)
    printf("%d\n", file_size);
    fseek(fp, 0, SEEK_SET);
    // allocate memory for the buffer to hold the file contents
    buffer = (char*) malloc(file_size + 1); // add space for null terminator
    memset(buffer, 0, file_size+1);
    if (buffer == NULL)
    {
        printf("Error allocating memory.\n");
        return NULL;
    }

    fread(buffer, file_size, 1, fp);
    buffer[file_size] = '\0';
    fclose(fp);

    return buffer;
}

void
saveResult(){
    printf("Reduced_input size : %d\n", latest_reduced_input_size);

    FILE *fp;
    fp = fopen(output_path, "w");
    fprintf(fp, "%s", latest_reduced_input);
    fclose(fp);
    
    exit(0);
}

void 
timeout(int sig)
{
    if(sig==SIGALRM)
        puts("Time out!");

    saveResult();
    exit(0);
}
void
keycontrol(int sig)
{
    if(sig==SIGINT)
        puts("CTRL+C pressed");

    saveResult();
    exit(0);
}

char*
reduce(char* original_input)
{
    latest_reduced_input_size = strlen(original_input);
    latest_reduced_input = original_input;
    printf("%s\n", original_input);

    int input_size = strlen(original_input) - 1;
    char* reduced_input;
    char* error_message = (char*)malloc(sizeof(char) * 4096); // pipe read로 이동

    // Used to store two ends of pipe
    int p2c[2];
    int c2p[2];
    // Used to store process id generated by fork()
    pid_t pid;

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
            char* head = (char*)malloc(sizeof(char) * (i+1));
            char* tail = (char*)malloc(sizeof(char) * (strlen(original_input)-i-input_size+1));
            reduced_input = (char*)malloc(sizeof(char) * (strlen(original_input)-input_size+1));
            if (reduced_input == NULL) {
                // Handle malloc() failure
                printf("Error: Failed to allocate memory for reduced_input\n");
                exit(EXIT_FAILURE);
            }
            // memset(reduced_input, 0, sizeof(char) * (strlen(original_input)-input_size));
            strcpy(reduced_input, "");
            //printf("rin:%s\n", reduced_input);
            if(0 > i-1)
            {
                strcpy(head, "");
                //printf("head:%s,", head);
            }
            else
            {
                // head = (char*)malloc(sizeof(char) * (i+1));
                if (head == NULL) {
                    // Handle malloc() failure
                    printf("Error: Failed to allocate memory for reduced_input\n");
                    exit(EXIT_FAILURE);
                }
            }

            strncpy(head, original_input, i);
            printf("head:%s,", head);
            // strcat(reduced_input, head);
            // sprintf(reduced_input, "%s", head);
            printf("rinput:%s\n", reduced_input);
            free(head);
            
            if(i+input_size > strlen(original_input)-1)
            {
                strcpy(tail,"");
                printf("tail:%s,", tail);
            }
            else
            {    // i = 0
                // tail = (char*)malloc(sizeof(char) * (strlen(original_input)-i-input_size+1));
                if (tail == NULL) {
                    // Handle malloc() failure
                    printf("Error: Failed to allocate memory for reduced_input\n");
                    exit(EXIT_FAILURE);
                }
                // strncpy(tail, original_input+i+input_size, strlen(original_input)-i-input_size+1);
                printf("tail:%s,", tail);
                // sprintf(reduced_input, "%s", tail);
                strcat(reduced_input, tail);
                printf("rinput:%s\n", reduced_input);
                free(tail);
            }

            //for test
            // printf("(%d) head: %s, tail: %s\n", i, head, tail);

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
                // dup2(p2c[1], 0);
                write(p2c[1], reduced_input, strlen(reduced_input) + 1);
                alarm(5);
                // Wait for child to send a string
                close(p2c[1]);
                wait(NULL);

                read(c2p[0], error_message, 100);
                alarm(0);
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
                close(c2p[0]);
                close(p2c[1]);
                dup2(c2p[1], 2);
                dup2(p2c[0], 0);
                read(p2c[0], reduced_input, strlen(reduced_input) + 1);
                close(p2c[0]);
                printf("(child) %s\n", reduced_input);
                // execl(target_p[0], target_p[0], target_p[1], target_p[2], NULL);
                execl(target_program, target_program, NULL);
            }
        }
        for(int i=0; i<=strlen(original_input)-input_size; i++)
        {
            if(i > i+input_size-1)
            {
                strcpy(reduced_input, "");
            }
            else
            {
                reduced_input = (char*)malloc(sizeof(char) * (input_size+1));
                if (reduced_input == NULL) {
                // Handle malloc() failure
                    printf("Error: Failed to allocate memory for reduced_input\n");
                    exit(EXIT_FAILURE);
                }
                strncpy(reduced_input, original_input+i, input_size);
            }
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
                // dup2(p2c[1], 0);
                write(p2c[1], reduced_input, strlen(reduced_input) + 1);
                alarm(5);
                // Wait for child to send a string
                close(p2c[1]);
                wait(NULL);

                read(c2p[0], error_message, 100);
                alarm(0);
                close(c2p[0]);
                printf("parent: %s\n", error_message);

                // check if error_message contains error_keyword
                if(strstr(error_message, error_keyword) != NULL)
                    break;
                else
                    return reduce(reduced_input);
            }
            else 
            { // Child process
                close(c2p[0]);
                close(p2c[1]);
                dup2(c2p[1], 2);
                dup2(p2c[0], 0);
                read(p2c[0], reduced_input, strlen(reduced_input) + 1);
                close(p2c[0]);
                printf("(child) %s\n", reduced_input);
                // execl(target_p[0], target_p[0], target_p[1], target_p[2], NULL);
                execl(target_program, target_program, NULL);
            }
        }
        input_size = input_size - 1;
    }
    free(reduced_input);
    free(error_message);

    return original_input;
}

int
main(int argc, char* argv[])
{
    // Used to get option(input, key error, ouput)
    int op; // option
    char input_path[30] ="";
    //char* target_p_argu_1 = '\0';
    //char* target_p_argu_2 = "\0";
    // Used to get option(input, key error, ouput)
    // get option i, m, o
    while ( (op = getopt(argc, argv, ":i:m:o:")) != -1 )
    {
        switch (op)
        {
            case 'i':
                memcpy(input_path, optarg, strlen(optarg));
                break;
            case 'm':
                memcpy(error_keyword, optarg, strlen(optarg));
                break;
            case 'o':
                memcpy(output_path, optarg, strlen(optarg));
                break;
            case ':':
                printf("err : %c option requires string\n", optopt);
                return 0;
            case '?':
                printf("err : %c is undetermined option\n", optopt);
                return 0;
        }
    }
    if(argc < 7)    // how to change?
    {
        printf("err : three parameters must be given with options\n");
        return 0;
    }
    if(argc == 7)
    {
        printf("err : no target program path\n");
        return 0;
    }
    else if(argc == 8)
    {
        target_program = argv[7];
    }
    else
    {
        printf("err : wrong arguments\n");
        return 0;
    }
    printf("i : %s\n", input_path);
    printf("m : %s\n", error_keyword);
    printf("o : %s\n", output_path);
    printf("p : %s\n", target_program);
    // printf("pa: %s %s\n", target_p_argu_1, target_p_argu_2);

    signal(SIGALRM, timeout);   // output_path
    signal(SIGINT, keycontrol); // output_path
    
    char * crashing_input = loadInput(input_path);
    // crashing_input = "abcdef";
    printf("%s\n", crashing_input);
    reduce(crashing_input);

    saveResult();
}