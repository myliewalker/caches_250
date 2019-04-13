#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

typedef struct way {
    int valid;
    int dirty;
    char tag[16];
    char data[80];
} way;
typedef struct loc {
    int address;
    char data[80];
    int accessed;
    int filled;
    // int times;
    // int set_times;
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

    int last = 0; //REMOVE

    int num_sets = csize / (bsize*ws);
    int set_size = calc(num_sets);
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
    loc memory[num_sets][ws];
    saved order[num_sets][ws];
    saved morder[num_sets][ws];
    for (int i = 0; i < num_sets; i++) {
        for (int j = 0; j < ws; j++) {
            order[i][j].pos = j;
            morder[i][j].pos = j;
        }
    }

    while (fgets(line, 80, fr) != NULL) {
        char address[17-block_size];
        char all[17];
        char hex[5];
        int ext = 0;
        if (line[0] == 's') ext = 1;
        for (int i = 3; i >= 0; i--) {
            hex[i] = line[i+5+ext];
            char conv[4];
            toBinary(conv, line[i+5+ext]);
            for (int j = 3; j >= 0; j--) {
                all[4*i+j] = conv[j];
            }
        }
        hex[4] = '\0';
        all[16] = '\0';
        int in = 0;
        for (int i = 15-block_size; i >= 0; i--) {
            address[in] = all[i];
            in++;
        }
        address[16-block_size] = '\0';

        int access_size = 2*(line[10+ext]-48);

        //Block offset
        char offset[block_size+1];
        in = 0;
        for (int i = 15; i > 15-block_size; i--) {
            offset[in] = all[i];
            in++;
        }
        offset[block_size] = '\0';

        //Index
        char index[set_size+1];
        in = 0;
        for (int i = 15-block_size; i > 15-block_size-set_size; i--) {
            index[in] = all[i];
            in++;
        }
        index[set_size] = '\0';

        //Tag
        char tag[17 - block_size - set_size];
        in = 0;
        for (int i = 15-block_size-set_size; i >= 0; i--) {
            tag[in] = all[i];
            in++;
        }
        tag[16-block_size-set_size] = '\0';

        //Create a way
        way current;
        current.valid = 1;
        current.dirty = 0;
        strcpy(current.tag, tag);
        strcpy(current.data, "00000000000000000000000000000000");

        about info;
        strcpy(info.address, address);
        info.set = toDecimal(index, sizeof(index));
        strcpy(info.write_method, write_method);

        int set = info.set;
        int caddress = toDecimal(address, sizeof(address));
        int maddress = caddress;
        if (caddress >= num_sets) {
                maddress = (caddress)%(num_sets);
        }

        char store[5];
        strcpy(store, "store");
        if (strncmp(line, store, 5) == 0) {
            char data[80];
            char tmp[80];
            if (current.valid != 1) current.valid = 0;
            if (current.dirty != 1) current.dirty = 0;

            if (line[13] != ' ') sscanf(line+13, "%s", data);
            else sscanf(line+14, "%s", data);
            strcpy(current.data, data);
            strcat(current.data, "0000000000000000");
            int hit = 0;
            
            //accounts for offset
            info.offset = 2*toDecimal(offset, sizeof(offset));
            char beginning[80];
            if (info.offset != 0) {
                for (int u = 0; u < 80; u++) {
                    if (u == info.offset) {
                        beginning[u] = '\0';
                        break;
                    }
                    beginning[u] = '0';
                }
                strcat(beginning, current.data);
                strncpy(current.data, beginning, 79);
            }

            //checks for a hit in cache
            for (int i = 0; i < ws; i++) {
                if (cache[set][i].valid == 1  && (sizeof(cache[set][i].tag) == sizeof(current.tag))) {
                    if (strcmp(cache[set][i].tag, current.tag) == 0) {
                        strcpy(cache[set][i].data, current.data);
                        hit = 1;
                    }
                }
                if (hit == 1) break;
            }
            
            //check for a hit in memory
            for (int i = 0; i < ws; i++) {
                if (memory[maddress][i].accessed == 1 && memory[maddress][i].address == caddress) {
                    hit = 1;
                    strcpy(cache[set][i].data, memory[maddress][i].data);
                    break;
                }
            }

            if (info.write_method[1] == 't') {
                int mdex = morder[maddress][0].pos;
                int add = memory[maddress][mdex].accessed;
                strcpy(memory[maddress][mdex].data, current.data);
                memory[maddress][mdex].filled = 1;
                memory[maddress][mdex].accessed = add;
                memory[maddress][mdex].address = caddress;

                int mshift = 0;
                saved cpy;
                if (ws == 2) {
                    cpy.pos = morder[maddress][0].pos;
                    strcpy(cpy.address, morder[maddress][0].address);
                    morder[maddress][0].pos = morder[maddress][1].pos;
                    strcpy(morder[maddress][0].address, morder[maddress][1].address);
                    morder[maddress][1].pos = cpy.pos;
                    strcpy(morder[maddress][1].address, cpy.address);
                }
                if (ws > 2) {
                    for (int k = 0; k < ws-1; k++) {
                        if (k == mdex) mshift = 1;
                        if (mshift == 1) {
                            cpy.pos = morder[maddress][k].pos;
                            strcpy(cpy.address, morder[maddress][k].address);
                            morder[maddress][k].pos = morder[maddress][k+1].pos;
                            strcpy(morder[maddress][k].address, morder[maddress][k+1].address);
                        }
                    }
                    morder[maddress][ws-1].pos = cpy.pos;
                    strcpy(morder[maddress][ws-1].address, cpy.address);
                }
            }

            else if (hit == 0 && info.write_method[1] == 'b') {
                int dex = order[set][0].pos;
                int mdex = 0;
                int loc = toDecimal(order[set][0].address, sizeof(order[set][0].address));
                if (loc >= num_sets) {
                    loc = (loc)%(num_sets);
                }
                if (cache[set][dex].dirty == 1) {
                    strcpy(memory[loc][mdex].data, cache[set][dex].data);
                    memory[loc][mdex].filled = 1;
                    memory[loc][mdex].accessed = 1;
                    memory[loc][mdex].address = toDecimal(order[set][0].address, sizeof(order[set][0].address));
                }

                strcpy(cache[set][dex].data, current.data);
                strcpy(cache[set][dex].tag, current.tag);
                cache[set][dex].valid = 1;
                cache[set][dex].dirty = 1;
                
                int shift = 0;
                saved ccpy;
                if (ws == 2) {
                    ccpy.pos = order[set][1].pos;
                    strcpy(ccpy.address, order[set][1].address);
                    order[set][0].pos = order[set][1].pos;
                    strcpy(order[set][0].address, order[set][1].address);
                    order[set][1].pos = ccpy.pos;
                    strcpy(order[set][0].address, ccpy.address);
                }
                if (ws > 2) {
                    for (int k = 0; k < ws-1; k++) {
                        if (k == dex) shift = 1;
                        if (shift == 1) {
                            ccpy.pos = order[set][k].pos;
                            strcpy(ccpy.address, order[set][k].address);
                            order[set][k].pos = order[set][k+1].pos;
                            strcpy(order[set][k].address, order[set][k+1].address);
                        }
                    }
                    order[set][ws-1].pos = ccpy.pos;
                    strcpy(order[set][ws-1].address, ccpy.address);
                }

                int mshift = 0;
                saved cpy;
                if (ws == 2) {
                    cpy.pos = morder[maddress][0].pos;
                    strcpy(cpy.address, morder[maddress][0].address);
                    morder[maddress][0].pos = morder[maddress][1].pos;
                    strcpy(morder[maddress][0].address, morder[maddress][1].address);
                    morder[maddress][1].pos = cpy.pos;
                    strcpy(morder[maddress][1].address, cpy.address);
                }
                if (ws > 2) {
                    for (int k = 0; k < ws-1; k++) {
                        if (k == mdex) mshift = 1;
                        if (mshift == 1) {
                            cpy.pos = morder[maddress][k].pos;
                            strcpy(cpy.address, morder[maddress][k].address);
                            morder[maddress][k].pos = morder[maddress][k+1].pos;
                            strcpy(morder[maddress][k].address, morder[maddress][k+1].address);
                        }
                    }
                    morder[maddress][ws-1].pos = cpy.pos;
                    strcpy(morder[maddress][ws-1].address, cpy.address);
                }
            }

            if (hit == 0) {
                printf("store %s miss\n", hex);
            }
            else printf("store %s hit\n", hex);
        }
        
        //LOAD   
        else {
            info.offset = 2*toDecimal(offset, sizeof(offset));
            char temp[80];
            for (int i = 0; i < ws; i++) {
                if (memory[maddress][i].accessed != 1) memory[maddress][i].accessed = 0;
                if (memory[maddress][i].filled != 1) memory[maddress][i].filled = 0;
            }

            //check if the value is in the cache
            int found = 0;
            for (int i = 0; i < ws; i++) {
                if (cache[set][i].valid == 1){
                    if (strcmp(cache[set][i].tag, current.tag) == 0) {
                        found = 1;
                        strcpy(temp, cache[set][i].data);
                    }
                }
                if (found == 1) break;
            }

            //check if the value is in memory
            for (int i = 0; i < ws; i++) {
                if (memory[maddress][i].accessed == 1 && memory[maddress][i].address == caddress) {
                    found = 1;
                    strcpy(temp, memory[maddress][i].data);
                    // printf("%s\n", memory[maddress][i].data);
                    break;
                }
            }
            
            if (found == 0 && info.write_method[1] == 't') {
                int mdex = morder[maddress][0].pos;
                // printf("evict %d\n", mdex);
                if (memory[maddress][mdex].filled == 0) {
                    for (int i = 0; i < 79; i++) {
                        memory[maddress][mdex].data[i] = '0';
                    }
                    memory[maddress][mdex].data[79] = '\0';
                }
                strcpy(temp, memory[maddress][mdex].data);
                memory[maddress][mdex].address = caddress;
                memory[maddress][mdex].accessed = 1;
                // printf("%s\n", memory[maddress][mdex].data);

                int mshift = 0;
                saved cpy;
                if (ws == 2) {
                    cpy.pos = morder[maddress][0].pos;
                    strcpy(cpy.address, morder[maddress][0].address);
                    morder[maddress][0].pos = morder[maddress][1].pos;
                    strcpy(morder[maddress][0].address, morder[maddress][1].address);
                    morder[maddress][1].pos = cpy.pos;
                    strcpy(morder[maddress][1].address, cpy.address);
                }
                if (ws > 2) {
                    for (int k = 0; k < ws-1; k++) {
                        if (k == mdex) mshift = 1;
                        if (mshift == 1) {
                            cpy.pos = morder[maddress][k].pos;
                            strcpy(cpy.address, morder[maddress][k].address);
                            morder[maddress][k].pos = morder[maddress][k+1].pos;
                            strcpy(morder[maddress][k].address, morder[maddress][k+1].address);
                        }
                    }
                    morder[maddress][ws-1].pos = cpy.pos;
                    strcpy(morder[maddress][ws-1].address, cpy.address);
                }
            }

            else if (found == 0 && info.write_method[1] == 'b') {
                int mdex = morder[maddress][0].pos;
                if (memory[maddress][mdex].filled == 0) {
                    for (int i = 0; i < 79; i++) {
                        memory[maddress][mdex].data[i] = '0';
                    }
                    memory[maddress][mdex].data[79] = '\0';
                }
                memory[maddress][mdex].accessed = 1;
                strcpy(temp, memory[maddress][mdex].data);

                way l;
                strcpy(l.tag, current.tag);
                strcpy(l.data, temp);
                int dex = order[set][0].pos;
                int loc = toDecimal(order[set][0].address, sizeof(order[set][0].address));
                if (loc >= num_sets) {
                    loc = (loc)%(num_sets);
                }

                if (cache[set][dex].dirty == 1) {
                    strcpy(memory[loc][mdex].data, cache[set][dex].data);
                    memory[loc][mdex].accessed = 1;
                    memory[loc][mdex].address = toDecimal(order[set][0].address, sizeof(order[set][0].address));
                }

                strcpy(cache[set][dex].data, current.data);
                strcpy(cache[set][dex].tag, current.tag);
                cache[set][dex].valid = 1;
                cache[set][dex].dirty = 1;
                
                int shift = 0;
                saved ccpy;
                if (ws == 2) {
                    ccpy.pos = order[set][1].pos;
                    strcpy(ccpy.address, order[set][1].address);
                    order[set][0].pos = order[set][1].pos;
                    strcpy(order[set][0].address, order[set][1].address);
                    order[set][1].pos = ccpy.pos;
                    strcpy(order[set][0].address, ccpy.address);
                }
                if (ws > 2) {
                    for (int k = 0; k < ws-1; k++) {
                        if (k == dex) shift = 1;
                        if (shift == 1) {
                            ccpy.pos = order[set][k].pos;
                            strcpy(ccpy.address, order[set][k].address);
                            order[set][k].pos = order[set][k+1].pos;
                            strcpy(order[set][k].address, order[set][k+1].address);
                        }
                    }
                    order[set][ws-1].pos = ccpy.pos;
                    strcpy(order[set][ws-1].address, ccpy.address);
                }
            }

            char value[access_size+1];
            if (info.offset == 0) {
                for (int i = 0; i < access_size; i++) {
                    value[i] = temp[i];
                }
            }
            else {
                char add[access_size];
                for (int i = 0; i < access_size; i++) {
                    add[i] = '0';
                }
                for (int i = 0; i < access_size; i++) {
                    if (i+info.offset >= sizeof(temp)-1) {
                        value[i] = add[i];
                    }
                    else value[i] = temp[i+info.offset];
                }
            }
            value[access_size] = '\0';

            if (found == 0) printf("load %s miss %s\n", hex, value);
            else printf("load %s hit %s\n", hex, value);

            //  printf("evict %d\n", morder[maddress][0].pos);
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
    int temp = 1;
    int raised = 0;
    for (int i = 0; i < size; i++) {
        raised = 0;
        if (binary[i] == '1') {
            raised = 1;
            for (int j = 0; j < i; j++) {
                raised = 2*raised;
            }
            dec = dec + raised;
        }
    }
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