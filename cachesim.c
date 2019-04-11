#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

typedef struct way {
    int valid;
    int dirty;
    char tag[16];
    char data[16];
} way;
typedef struct loc {
    int address;
    char data[16];
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
void toBinary(char* binary, char hex);
int toDecimal(char* binary, int size);
void toHex(char* hex, char* binary);

int main(int num, char* args[]) {
    char* file_name = args[num-5];
    char line[80];
    int csize = 0;
    sscanf(args[num-4], "%d", &csize); 
    csize = csize*1024;
    int ws = 0;
    sscanf(args[num-3], "%d", &ws);    
    int bsize = 0;
    sscanf(args[num-1], "%d", &bsize);

    int num_sets = csize / (bsize*ws);
    //Calculate sizes in base 2
    int cache_size = calc(csize);
    int num_ways = calc(ws);
    int block_size = calc(bsize);

    char write_method[2];
    strcpy(write_method, args[num-2]);

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
        char address[16-block_size];
        char all[17];
        char hex[4];
        int ext = 0;
        if (line[0] == 's') ext = 1;
        for (int i = 3; i >= 0; i--) {
            hex[i] = line[i+5+ext];
            char conv[4];
            toBinary(conv, line[i+5+ext]);
            for (int j = 3; j >= 0; j--) {
                if (4*i+j < 16-block_size) address[4*i+j] = conv[j];
                all[4*i+j] = conv[j];
            }
        }
        all[16] = '\0';

        char* split = strtok(line, " ");
        int access_size = split[2]*block_size;

        //Block offset
        char offset[block_size];
        int in = 0;
        for (int i = 15; i > 15-block_size; i--) {
            offset[in] = all[i];
            in++;
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

        //Create a way
        way current;
        current.valid = 1;
        current.dirty = 0;
        strcpy(current.tag, tag);
        strcpy(current.data, "0000000000000000");

        about info;
        strcpy(info.address, address);
        // printf("%s %s", address, info.address);
        info.set = toDecimal(index, sizeof(index));
        info.ways = ws;
        strcpy(info.write_method, write_method);

        char store[5];
        strcpy(store, "store");
        if (strncmp(line, store, 5) == 0) {
            char data[8];
            if (line[13] != ' ') sscanf(line+13, "%s", data);
            else sscanf(line+14, "%s", data);
            strcpy(current.data, data);

            int set = info.set;
            int caddress = toDecimal(address, sizeof(address));
            char hit[4];
            strcpy(hit, "miss");

            int found = 0;
            for (int i = 0; i < ws; i++) {
                if ((cache[set][i].valid == 1) && (sizeof(cache[set][i].tag) == sizeof(current.tag))) {
                    for (int i = 0; i < sizeof(current.tag); i++) {
                        if (cache[set][i].tag[i] != current.tag[i]) break;
                        if (i == sizeof(current.tag)-1) {
                            strcpy(cache[set][i].data, current.data);
                            found = 1;
                            strcpy(hit, "hit");
                        }
                    }
                }
                if (found == 1) break;
            }

            if (info.write_method[1] == 't') {
                strcpy(memory[caddress].data, current.data);
            }

            else if (found == 0) {
                current.dirty = 1;
                int dex = order[set][0].pos;
                if (cache[set][dex].dirty == 1) {
                    strcpy(memory[toDecimal(order[set][0].address, sizeof(order[set][0].address))].data, cache[set][dex].data);
                }
                cache[set][dex] = current;
                
                int shift = 0;
                for (int k = 0; k < ws-1; k++) {
                    if (k == dex) shift = 1;
                    if (shift == 1) {
                        order[set][k].pos = order[set][k+1].pos;
                        strcpy(order[set][k].address, order[set][k+1].address);
                    }
                }
                order[set][ws-1].pos = order[set][dex].pos;
                strcpy(order[set][ws-1].address, order[set][dex].address);
            }

            printf("store %s %s\n", hex, hit);
        }
        //WORKS
        //LOAD   
        else {
            info.offset = toDecimal(offset, sizeof(offset));
            info.access = access_size;
            int set = info.set;
            int caddress = toDecimal(info.address, sizeof(info.address));
            char hit[4];
            strcpy(hit, "miss");
            char temp[32];

            int found = 0;
            for (int i = 0; i < ws; i++) {
                if ((cache[set][i].valid == 1) && (sizeof(cache[set][i].tag) == sizeof(current.tag))) {
                    for (int j = 0; j < sizeof(current.tag); j++) {
                        if (cache[set][i].tag[j] != current.tag[j]) break;
                        if (j == sizeof(current.tag)-1) {
                            found = 1;
                            strcpy(hit, "hit");
                            strcpy(temp, cache[set][i].data);
                        }
                    }
                }
                if (found == 1) break;
            }

            if (found == 0 && info.write_method[1] == 't') {
                strcpy(temp, memory[caddress].data);
                // printf("test %s %d %s\n", info.address, caddress, temp);
                way l;
                l.valid = 1;
                strcpy(l.tag, current.tag);
                strcpy(l.data, temp);
                int dex = order[set][0].pos;
                cache[set][dex] = l;
                
                int shift = 0;
                for (int k = 0; k < ws-1; k++) {
                    if (k == dex) shift = 1;
                    if (shift == 1) {
                        order[set][k].pos = order[set][k+1].pos;
                        strcpy(order[set][k].address, order[set][k+1].address);
                    }
                }
                order[set][ws-1].pos = order[set][dex].pos;
                strcpy(order[set][ws-1].address, order[set][dex].address);
            }

            else if (found == 0 && info.write_method[1] == 'b') {
                strcpy(temp, memory[caddress].data);
                way l;
                l.valid = 1;
                strcpy(l.tag, current.tag);
                strcpy(l.data, temp);
                int dex = order[set][0].pos;

                if (cache[set][dex].dirty == 1) {
                    strcpy(memory[toDecimal(order[set][0].address, sizeof(order[set][0].address))].data, cache[set][dex].data);
                }
                cache[set][dex] = l;
                
                int shift = 0;
                for (int k = 0; k < ws-1; k++) {
                    if (k == dex) shift = 1;
                    if (shift == 1) {
                        order[set][k].pos = order[set][k+1].pos;
                        strcpy(order[set][k].address, order[set][k+1].address);
                    }
                }
                order[set][ws-1].pos = order[set][dex].pos;
                strcpy(order[set][ws-1].address, order[set][dex].address);
            }

            // printf("%s\n", temp);

            char value[sizeof(temp)];
            char tracker[sizeof(temp)];
            for (int i = 0; i < info.access; i++) {
                value[i] = temp[i+info.offset];
                tracker[i] = temp[i];
                if (strcmp(temp, tracker) == 0) break;
            }
            // printf("%s", value);
            // return 0;
            // char val[sizeof(value)/4 + 1];
            // toHex(val, value);
            printf("load %s %s %s\n", hex, hit, value); //change one val
            return 0;
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

void toBinary(char binary[4], char hex) {
    if (hex == '0') strcpy(binary, "0000");
    else if (hex == '1') strcpy(binary, "0001");
    else if (hex == '2') strcpy(binary, "0010");
    else if (hex == '3') strcpy(binary, "0011");
    else if (hex == '4') strcpy(binary, "0100");
    else if (hex == '5') strcpy(binary, "0101");
    else if (hex == '6') strcpy(binary, "0110");
    else if (hex == '7') strcpy(binary, "0111");
    else if (hex == '8') strcpy(binary, "1000");
    else if (hex == '9') strcpy(binary, "1001");
    else if (hex == 'a') strcpy(binary, "1010");
    else if (hex == 'b') strcpy(binary, "1011");
    else if (hex == 'c') strcpy(binary, "1100");
    else if (hex == 'd') strcpy(binary, "1101");
    else if (hex == 'e') strcpy(binary, "1110");
    else if (hex == 'f') strcpy(binary, "1111");
}

int toDecimal(char* binary, int size) {
    int dec = 0;
    // int temp = 1;
    int raised = 0;
    for (int i = 0; i < size; i++) {
        raised = 0;
        if (binary[i] == '1') {
            raised = 1;
            // temp = 1;
            if (i == 1) raised = 2;
            if (i == 2) raised = 4;
            if (i == 3) raised = 8;
            if (i == 4) raised = 16;
            if (i == 5) raised = 16;
            if (i == 6) raised = 16;
            if (i == 7) raised = 16;
            if (i == 8) raised = 16;
            if (i == 9) raised = 16;
            if (i == 10) raised = 16;
            if (i == 11) raised = 16;
            if (i == 12) raised = 16;
            if (i == 13) raised = 16;
            if (i == 14) raised = 16;
            if (i == 15) raised = 16;
            if (i == 16) raised = 16;
            if (i > 16) printf("exceeded");
            // for (int j = 0; j < i; j++) {
            //     temp = raised;
            //     raised = 2*temp;
            // }
            // dec = raised; //issue
        }
        dec = dec + raised;
    }
    // printf("%d\n", dec);
    return dec;
}

void toHex(char* hex, char* binary) {
    for (int i = 0; i < sizeof(binary); i++) {
        char individual[4];
        for (int j = i; j < 4; j++) {
            if (j >= sizeof(binary)) break;
            individual[j] = hex[i*4 + j];
        }
        int t = toDecimal(individual, 4);
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
}

//WILL THIS UPDATE VARIABLES??