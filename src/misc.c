#include <stdlib.h>
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