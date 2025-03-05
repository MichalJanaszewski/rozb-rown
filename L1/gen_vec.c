#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define VECTOR_SIZE 200000

int main() {
    FILE *file = fopen("vector_file", "w");
    if (!file) {
        perror("Error opening file");
        return EXIT_FAILURE;
    }

    
    for (int i = 0; i < VECTOR_SIZE; i++) {
        fprintf(file, "%d ", rand() % 100);
    }
    
    fclose(file);
    return EXIT_SUCCESS;
}

