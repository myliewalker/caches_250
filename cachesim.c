#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

int main(int num, int* args[]) {
    char* file_name = args[num-5];
    char line[80];

    FILE* fr = fopen(file_name, "rt");
    if (fr == NULL) {
        printf("Error opening file\n");
        return 1;
    }

    while (fgets(line, 80, fr) != NULL) {
        if (line[0] == 's') {
            char address[4];
            for (int i = 0; i < 4; i++) {
                address[i] = line[i+6];
            }

            store();

            printf("store, %s, miss\n", address);
        }   
        else {
            printf("load");
        }
    }

    fclose(fr);
    return 0;
}

int store() {

}

int load() {

}