#ifndef MISC_H
#define MISC_H

int CompareStrings(
    const char *self, 
    const char *other);

void PrintToBufferAtIndex(
    char* buffer_to_write, 
    const char* text, 
    int *index);

void GetSystemCommandOneLineOutput(
    const char *command, 
    char *output_buffer, 
    int max_size);

#endif