#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

char* toBinary(char hex);
int calc (int size);

int main(int num, int* args[]) {
    char* file_name = args[num-5];
    char line[80];

    //size = 2^size
    int cache_size = calc(args[num-4])+10;
    int ways = calc(args[num-3]);
    int block_size = calc(args[num-1]);

    char* writes = args[num-2];
    char* cache[(cache_size/block_size)+1][ways+1];

    //One way in the set
    typedef struct way {
        int valid;
        int dirty;
        int tag;
        int data;
    } way;

    //Memory location consists of address and data
    typedef struct loc {
        int address;
        int data;
    } loc;

    FILE* fr = fopen(file_name, "rt");
    if (fr == NULL) {
        printf("Error opening file\n");
        return 1;
    }

    while (fgets(line, 80, fr) != NULL) {
        char address[16];
        char hex[4];
        for (int i = 3; i >= 0; i--) {
            hex[i] = line[i+6];
            char conv[4] = toBinary(line[i+6]);
            for (int j = 3; j > 0; j--) {
                address[4*i+j] = conv[j];
            }
        }

        char offset[block_size];
        for (int i = 0; i < sizeof(offset); i++) {
            offset[i] = address[i];
        } 

        int set_size = cache_size / block_size;
        if (ways != 0) set_size = set_size / ways;
        char index[set_size];
        for (int i = 0; i < sizeof(index); i++) {
            index[i] = address[i + sizeof(offset)];
        }

        char tag[16 - block_size - set_size];
        for (int i = 0; i < sizeof(tag); i++) {
            tag[i] = address[i + sizeof(offset) + sizeof(index)];
        }
        //Broke down address into parts

        if (line[0] == 's') {
            store();

            printf("store, %s, miss\n", hex);
        }   
        else {
            printf("load");
        }
    }

    fclose(fr);
    return 0;
}

int calc (int size) {
    int count = 0;
    while (size > 1) {
        count++;
        size = size/2;
    }
    return count;
}

char* toBinary(char hex) {
    if (hex == 0) return (char*)0000;
    if (hex == 1) return (char*)0001;
    if (hex == 2) return (char*)0010;
    if (hex == 3) return (char*)0011;
    if (hex == 5) return (char*)0101;
    if (hex == 6) return (char*)0110;
    if (hex == 7) return (char*)0111;
    if (hex == 8) return (char*)1000;
    if (hex == 9) return (char*)1001;
    if (hex == 'a') return (char*)1010;
    if (hex == 'b') return (char*)1011;
    if (hex == 'c') return (char*)1100;
    if (hex == 'd') return (char*)1101;
    if (hex == 'e') return (char*)1110;
    if (hex == 'f') return (char*)1111;
}

int store() {

}

int load() {

}