#include <stdlib.h>
#include <stdio.h>
#include <string.h>

int CompareStrings(const char *self, const char *other)
{
    if (self == NULL && other == NULL)
        return 0;
    else if (self == NULL)
        return 1;
    else if (other == NULL)
        return -1;
    else
        return strcmp(self, other);
}

void PrintToBufferAtIndex(char* buffer_to_write, const char* text, int *index)
{
    for (unsigned int i = 0; i < strlen(text); ++i)    
        buffer_to_write[(*index)++] = text[i];
}

// Execute command stored in buffer and use pipe to read it's output.
void GetSystemCommandOneLineOutput(const char *command, char *output_buffer, int max_size)
{
    FILE *pipe = popen(command, "r");
    if (pipe == NULL) 
    {
        printf("Failed to run command\n" );
        exit(1);
    }

    // Get the first line of output:
    fgets(output_buffer, max_size, pipe);

    pclose(pipe);
}