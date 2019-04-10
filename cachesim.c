#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

typedef struct way {
    int valid;
    int dirty;
    char tag[16];
    char data[16];
} way;
typedef struct loc {
    int address;
    char data[16];
    // char tag[16];
} loc;
typedef struct about {
    char address[16];
    char write_method[2];
    int set;
    int ways;
    int tag_length;
    int offset;
    int access;
} about;
typedef struct saved {
    int pos;
    char address[16];
} saved;

FILE* fr;

int calc(int size);
char* toBinary(char hex);
int toDecimal(char* binary);
char* toHex(char* binary);
char* store(way cache[][], int order[][], loc memory[], way current, about info);
char** load(way cache[][], int order[][], loc memory[], way current, about info);
int update (int order[][], int set, int ways, int dex);

int main(int num, char* args[]) {
    char* file_name = args[num-5];
    char line[80];
    int csize;
    sscanf(args[num-4], "%d", &csize); 
    csize += 1024;
    int ws;
    sscanf(args[num-3], "%d", &ws);    
    int bsize;
    sscanf(args[num-1], "%d", &bsize);

    int num_sets = csize / (bsize*ws);
    //Calculate sizes in base 2
    int cache_size = calc(csize);
    int num_ways = calc(ws);
    int block_size = calc(bsize);

    char* write_method;
    strcopy(write_method, args[num-2]);

    fr = fopen(file_name, "rt");
    if (fr == NULL) {
        printf("Error opening file\n");
        return 1;
    }

    way cache[num_sets][ws];
    saved order[num_sets][ws];
    loc memory[num_sets*ws];

    while (fgets(line, 80, fr) != NULL) {
        //Copy address: binary into address, hex into hex
        char* l = strtok(line, " ");
        char address[16];
        char hex[4];
        for (int i = 3; i >= 0; i--) {
            hex[i] = line[i+6];
            char conv[4] = toBinary(line[i+6]);
            for (int j = 3; j > 0; j--) {
                address[4*i+j] = conv[j];
            }
        }

        int access_size = l[2];

        //Block offset
        char offset[block_size];
        for (int i = 0; i < sizeof(offset); i++) {
            offset[i] = address[i];
        } 
        //Index
        int set_size = calc(num_sets);
        char index[set_size];
        for (int i = 0; i < sizeof(index); i++) {
            index[i] = address[i + sizeof(offset)];
        }
        //Tag
        char tag[16 - block_size - set_size];
        for (int i = 0; i < sizeof(tag); i++) {
            tag[i] = address[i + sizeof(offset) + sizeof(index)];
        }

        way current;
        current.valid = 1;
        strcopy(current.tag, tag);

        about info;
        strcopy(info.address, address);
        info.set = toDecimal(index);
        info.ways = ways;
        strcopy(info.write_method, write_method);

        if (line[0] == 's') {
            strcopy(current.data, l[3]);
            char* hit = store(cache, order, memory, current, info);
            printf("store, %s, %s\n", hex, hit);
        }   
        else {
            info.offset = toDecimal(offset);
            info.access = access_size;
            char** r = load(cache, order, memory, current, info);
            char* hit;
            strcopy(hit, r[0]);
            char* value;
            strcopy(value, r[1]);
            printf("load, %s, %s, %s\n", hex, hit, toHex(value));
        }
    }

    fclose(fr);
    return 0;
}

int calc(int size) {
    int count = 0;
    while (size > 1) {
        count++;
        size = size/2;
    }
    return count;
}

char* toBinary(char hex) {
    if (hex == 0) return '0000';
    if (hex == 1) return '0001';
    if (hex == 2) return '0010';
    if (hex == 3) return '0011';
    if (hex == 5) return '0101';
    if (hex == 6) return '0110';
    if (hex == 7) return '0111';
    if (hex == 8) return '1000';
    if (hex == 9) return '1001';
    if (hex == 'a') return '1010';
    if (hex == 'b') return '1011';
    if (hex == 'c') return '1100';
    if (hex == 'd') return '1101';
    if (hex == 'e') return '1110';
    return '1111';
}

int toDecimal(char* binary) {
    int dec;
    for (int i = 0; i < sizeof(binary); i++) {
        dec += pow(2, i);
    }
    return dec;
}

char* toHex(char* binary) {
    char hex[sizeof(binary)/4 + 1];
    for (int i = 0; i < sizeof(binary); i++) {
        char individual[4];
        for (int j = i; j < 4; j++) {
            if (j >= sizeof(binary)) break;
            individual[j] = hex[i*4 + j];
        }
        int t = toDecimal(individal);
        if (t == 0) hex[i] = '0';
        else if (t == 1) hex[i] = '1';
        else if (t == 2) hex[i] = '2';
        else if (t == 3) hex[i] = '3';
        else if (t == 4) hex[i] = '4';
        else if (t == 5) hex[i] = '5';
        else if (t == 6) hex[i] = '6';
        else if (t == 7) hex[i] = '7';
        else if (t == 8) hex[i] = '8';
        else if (t == 9) hex[i] = '9';
        else if (t == 10) hex[i] = 'a';
        else if (t == 11) hex[i] = 'b';
        else if (t == 12) hex[i] = 'c';
        else if (t == 13) hex[i] = 'd';
        else if (t == 14) hex[i] = 'e';
        else if (t == 15) hex[i] = 'f';
    }
    return hex;
}

char* store(way cache[][], int order[][], loc memory[], way current, about info) {
    int ways = info.ways;
    int set = info.set;
    int address = toDecimal(info.address);

    //Check if the element is already in the cache
    boolean found = false;
    for (int i = 0; i < ways; i++) {
        if ((cache[set][i].valid == 1) && (sizeof(cache[set][i].tag) == sizeof(current.tag))) {
            for (int i = 0; i < sizeof(current.tag); i++) {
                if (cache[set][i].tag[i] != current.tag[i]) break;
                if (i == sizeof(current.tag)-1) {
                    cache[set][i].data = current.data;
                    found = true;
                }
            }
        }
        if (found == true) break;
    }

    if (info.write_method[1] = 't') {
        memory[address] = current.data;
        if (found == true) return 'hit';
        return 'miss';
    }

    else {
        if (found == true) return 'hit';
        current.dirty = 1;
        int dex = order[set][0].pos;
        if (cache[set][dex].dirty == 1) {
            memory[order[set][0].address] = cache[set][dex];
        }
        cache[set][dex] = current;
        update(order, set, ways, dex);
    }

    return 'miss';
}

char** load(way cache[][], int order[][], loc memory[], way current, about info) {
    int ways = info.ways;
    int set = info.set;
    int address = toDecimal(info.address);
    char res[4];
    char* temp;
    char* ret[2];

    boolean contains = false;
    for (int i = 0; i < ways; i++) {
        if ((cache[set][i].valid == 1) && (sizeof(cache[set][i].tag) == sizeof(current.tag))) {
            for (int i = 0; i < sizeof(current.tag); i++) {
                if (cache[set][i].tag[i] != current.tag[i]) break;
                if (i == sizeof(current.tag)-1) {
                    contains = true;
                    res = 'hit';
                    strcopy(temp, cache[set][i].data);
                }
            }
        }
        if (contains == true) break;
    }

    if (contains == true) {
        res = 'hit';
    }

    else if (info.write_method[1] = 't') {
        res = 'miss';
        strcopy(temp, memory[address].data);
        way l;
        l.valid = 1;
        l.tag = current.tag;
        strcopy(l.data, temp);
        int dex = order[set][0].pos;
        cache[set][dex] = l;
        update(order, set, ways, dex);
    }

    else if (info.write_method[1] = 'b') {
        res = 'miss';
        strcopy(temp, memory[address].data);
        way l;
        l.valid = 1;
        l.tag = current.tag; //CORRECT??
        strcopy(l.data, temp);
        int dex = order[set][0].pos;
        //Write to memory
        if (cache[set][dex].dirty == 1) {
            memory[order[set][0].address] = cache[set][dex];
        }
        cache[set][dex] = l;
        update(order, set, ways, dex);
    }

    strcopy(ret[0], res);
    char value[sizeof(temp)];
    for (int i = 0; i < info.access; i++) {
        value[i] = temp[i+info.offset];
    }
    strcopy(ret[1], value);
    return ret;
}

int update (int order[][], int set, int ways, int dex) {
    boolean shift = false;
    for (int i = 0; i < ways-1) {
        if (i = dex) shift = true;
        if (shift == true) {
            order[set][i].pos = order[set][i+1].pos;
            strcopy(order[set][i].address, order[set][i+1].address);
        }
    }
    order[set][ways-1].pos = order[set][dex].pos;
    strcopy(order[set][ways-1].address, order[set][dex].address);
    return 0;
}

//WILL THIS UPDATE VARIABLES??