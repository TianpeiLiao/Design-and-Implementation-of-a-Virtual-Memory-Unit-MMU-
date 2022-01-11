#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#define PAGES 128
#define PAGE_SIZE 256
#include <sys/mman.h>
#include <fcntl.h>
struct entry{
    int key; //key is the logical address page num
    int value; //value is the physical address page num
    int count; //history of using
};
//convert int to binary
void int_to_bin_digit(unsigned int in, int count, char *out)
{
    /* assert: count <= sizeof(int)*CHAR_BIT */
    unsigned int mask = 1U << (count-1);
    int i;
    for (i = 0; i < count; i++) {
        out[i] = (in & mask) ? '1' : '0';
        in <<= 1;
    }
}

//initialize char array
void init_bin(char *bin,int size){
    for(int i=0; i< size;i++)
        bin[i]='0';

}

//find if the page_num in TLB
int check_tlb(int p_n,struct entry e[16]){
    for(int i=0; i<16;i++){
        if(e[i].key==p_n){
            return i;
        }

    }
    return -1;
}

int check_page_t(int p_n,struct entry *p_t,int size){
    for(int i=0; i<size;i++){
        if(p_t[i].key==p_n){
            return i;
        }

    }
    return -1;
}

//return the page number
int page_number(char in[16]){
    int page_num=0;
    int j=7;
    for(int i=0; i<=7; i++){
        page_num +=  (int)pow(2,j) * (in[i]-'0');
        j--;
    }
    return page_num;
}

//return the offset
int offset(char in[16]){
    int offset=0;
    int j=7;
    for(int i=8; i<=15; i++){
        offset +=  (int)pow(2,j) * (in[i]-'0');
        j--;
    }
    return offset;
}

//combine frame num and offset
int combine(int f_n,int o_s){
    char *f=malloc(50);
    char *o=malloc(50);
    char *result=malloc(50);
    int j=14;
    int r=0;
    init_bin(f,7);
    init_bin(o,8);
    int_to_bin_digit(f_n,7,f);
    int_to_bin_digit(o_s,8,o);

    strcpy(result, f);
    strcat(result, o);
    for(int i=0; i<=14; i++){
        r +=  (int)pow(2,j) * (result[i]-'0');
        j--;
    }


    return r;

}

void init_bin_256(char *bin){
    for(int i=0; i< sizeof(bin);i++)
        bin[i]='0';
}

//combine frame num and offset
int combine_256(int f_n,int o_s){
    char *f=malloc(50);
    char *o=malloc(50);
    char *result = malloc(50);
    int j=15;
    int r=0;
    init_bin_256(f);
    init_bin_256(o);
    int_to_bin_digit(f_n,8,f);
    int_to_bin_digit(o_s,8,o);

    strcpy(result, f);
    strcat(result, o);
    for(int i=0; i<=15; i++){
        r +=  (int)pow(2,j) * (result[i]-'0');
        j--;
    }
//    printf("%d",r);
    return r;
}


int main(int argc, char **argv) {

    if (strcmp(argv[1], "128") == 0) {


    int total_address = 0; //total translated address
    int tlb_hit = 0; //num of tlb hitting
    int num_page_fault = 0; //num of page fault
    //TLB table
    struct entry tlb[16];
    //Page table
    struct entry page_table[128];
    //buffer
    signed char *backing_ptr=malloc(50);
    signed char main_memo[256 * 256];
    int ptr;
    int full = 0;
    FILE *create_csv=malloc(50);
    int p_address = 0;
    create_csv = fopen("output128.csv", "w+");
    //open .bin backing store
    ptr = open(argv[2], O_RDONLY);
    //create virtual memory
    backing_ptr = mmap(0, 256 * 256, PROT_READ, MAP_PRIVATE, ptr, 0);
    //table position

    //physical page number
    int value = 0;
    //tlb position
    int tlb_pos = 0;
    //frame num
    int frame_num = 0;
    FILE *fp=malloc(50);
    char str[100];//string type for scan logical address

    char *filename = argv[3]; //logical address

    char binary[16]; //store temp binary array of char

    int scan_result; //int type for scan logical address

    int page_num = 0; //page number

    int off_set; //offset value

    fp = fopen(filename, "r"); //open file

    //checking the validity of opening file
    if (fp == NULL) {
        printf("Could not open file %s", filename);
        return 1;
    }

    //initialize page table
    for (int i = 0; i < 128; i++) {
        page_table[i].key = -1;
        page_table[i].value = -1;
        page_table[i].count = -1;
    }
    //initialize tlb table
    for (int i = 0; i < 16; i++) {
        tlb[i].key = -1;
        tlb[i].value = -1;
        tlb[i].count = -1;
    }

    //reading file
    while (fgets(str, 100, fp) != NULL) {
        total_address += 1;
        //initializing binary array
        init_bin(binary, 16);

        //converting string into int
        scan_result = atoi(str);


        //converting int
        int_to_bin_digit(scan_result, 16, binary);

        page_num = page_number(binary);
        off_set = offset(binary);
        //if tlb is missed
        if (check_tlb(page_num, tlb) == -1) {
            signed char value1;
            //if page table if missed
            if (check_page_t(page_num, page_table,128) == -1) {
                num_page_fault += 1;
                if (full == 1) {
                    //the page_table is full, looking for oldest use
                    int max = -100;
                    int max_index = -100;
                    //find the max value
                    for (int i = 0; i < 128; i++) {
                        if (page_table[i].count > max)
                            max = page_table[i].count;
                    }
                    //find the max value's index
                    for (int i = 0; i < 128; i++) {
                        if (page_table[i].count == max) {
                            max_index = i;
                            break;
                        }

                    }
//                    printf("max value: %d i:%d \n",max,max_index);
                    //page replacement
                    page_table[max_index].key = page_num;
                    page_table[max_index].value = max_index;

                    page_table[max_index].count = -1;
                    p_address = combine(max_index, off_set);
                    memcpy(main_memo + max_index * 256, backing_ptr + page_num * 256, 256);
                    value1 = main_memo[max_index * 256 + off_set];

                    //replace for tlb
                    tlb[tlb_pos].key = page_table[check_page_t(page_num, page_table,128)].key;
                    tlb[tlb_pos].value = page_table[check_page_t(page_num, page_table,128)].value;
                    tlb[tlb_pos].count = -1;

                    tlb_pos += 1;
                    if (tlb_pos > 15)
                        tlb_pos = 0;

                }
                    //if it is not full
                else {
                    page_table[frame_num].key = page_num;
                    page_table[frame_num].value = frame_num;
                    page_table[frame_num].count = -1;
                    p_address = combine(frame_num, off_set);
                    tlb[tlb_pos].key = page_num;
                    tlb[tlb_pos].value = frame_num;
                    tlb[tlb_pos].count = -1;
                    memcpy(main_memo + frame_num * 256, backing_ptr + page_num * 256, 256);
                    value1 = main_memo[frame_num * 256 + off_set];
                    tlb_pos += 1;
                    if (tlb_pos > 15)
                        tlb_pos = 0;

                    frame_num += 1;
                    if (frame_num >= 128)
                        full = 1;
                }


                fprintf(create_csv, "%d,%d,%d\n", scan_result, p_address, value1);
            } else if (check_page_t(page_num, page_table,128) >= 0) {

                //if page table hit, update the TLB table
                tlb[tlb_pos].key = page_table[check_page_t(page_num, page_table,128)].key;
                tlb[tlb_pos].value = page_table[check_page_t(page_num, page_table,128)].value;
                tlb[tlb_pos].count = -1;

                tlb_pos += 1;
                if (tlb_pos > 15)
                    tlb_pos = 0;
                //reset the page_table accordingly to the newest one
                page_table[check_page_t(page_num, page_table,128)].count = -1;
                p_address = combine(page_table[check_page_t(page_num, page_table,128)].value, off_set);
                //read the pth page accordingly
                memcpy(main_memo + frame_num * 256, backing_ptr + page_num * 256, 256);
                //assign the value accordingly
                value1 = main_memo[frame_num * 256 + off_set];
                fprintf(create_csv, "%d,%d,%d\n", scan_result, p_address, value1);


            }
        } else if (check_tlb(page_num, tlb) != -1) {
            //printf("string: %s %d\n", str, tlb_hit);
            tlb_hit += 1;
            //reset the page_table accordingly to the newest one
            page_table[check_page_t(page_num, page_table,128)].count = -1;
            tlb[check_tlb(page_num, tlb)].count = -1;
            //read the pth page accordingly
            memcpy(main_memo + frame_num * 256, backing_ptr + page_num * 256, 256);
            //assign the value accordingly
            signed char value1 = main_memo[frame_num * 256 + off_set];
            p_address = combine(tlb[check_tlb(page_num, tlb)].value, off_set);
            fprintf(create_csv, "%d,%d,%d\n", scan_result, p_address, value1);
        }


        value += 1;
        //TLB miss
        //at the end, each entry's count ++
        for (int i = 0; i < 128; i++) {
            if (page_table[i].value != -1)
                page_table[i].count += 1;
        }


    }
    fprintf(create_csv, "Page Faults Rate, %.2f%%,\n", ((double) num_page_fault / total_address) * 100);
    fprintf(create_csv, "TLB Hits Rate, %.2f%%,", ((double) tlb_hit / total_address) * 100);

    fclose(create_csv);
    //closing fp
    fclose(fp);

    //printf("tlb hit rate:%.2f \n", ((double) tlb_hit / total_address) * 100);
    //printf("page fault rate:%.2f ", ((double) num_page_fault / total_address) * 100);
    return 0;
    }
    else if(strcmp(argv[1], "256") == 0){

        //TLB table
        struct entry tlb[16];
        //Page table
        struct entry page_table[256];
        //buffer
        signed char *backing_ptr=malloc(50);
        signed char main_memo[256*256*2];
        int ptr;
        FILE *create_csv=malloc(50);
        int p_address=0;
        create_csv = fopen("output256.csv", "w+");
        //open .bin backing store
        ptr = open(argv[2], O_RDONLY);
        //create virtual memory
        backing_ptr = mmap(0, 256*256, PROT_READ, MAP_PRIVATE, ptr, 0);
        //table position

        //physical page number
        int value=0;
        //tlb position
        int tlb_pos=0;
        //frame num
        int frame_num = 0;
        FILE *fp=malloc(50);
        char str[100];//string type for scan logical address

        int total_address=0; //total translated address
        int tlb_hit=0; //num of tlb hitting
        int num_page_fault=0; //num of page fault
        char* filename = argv[3]; //logical address

        char binary[16]; //store temp binary array of char

        int scan_result; //int type for scan logical address

        int page_num=0; //page number

        int off_set; //offset value

        fp = fopen(filename, "r"); //open file

        //checking the validity of opening file
        if (fp == NULL){
            printf("Could not open file %s",filename);
            return 1;
        }
        //initialize page table
        for (int i = 0; i < 256; i++) {
            page_table[i].key=-1;
            page_table[i].value=-1;
        }
        //initialize tlb table
        for (int i = 0; i < 16; i++) {
            tlb[i].key=-1;
            tlb[i].value=-1;
        }

        //reading file
        while (fgets(str, 100, fp) != NULL) {
            total_address+=1;
            //initializing binary array
            init_bin_256(binary);

            //converting stirng into int
            scan_result = atoi(str);


            //converting int
            int_to_bin_digit(scan_result,16,binary);

            page_num = page_number(binary);
            off_set = offset(binary);
            //if tlb is missed
            if(check_tlb(page_num,tlb)==-1){

                //if page table if missed
                if(check_page_t(page_num,page_table,256)==-1) {
                    num_page_fault+=1;
                    tlb[tlb_pos].key = page_num;
                    tlb[tlb_pos].value = frame_num;

                    page_table[frame_num].key = page_num;
                    page_table[frame_num].value = frame_num;
                    p_address =combine_256(frame_num,off_set);
                    //read the pth page accordingly
                    memcpy(main_memo + frame_num * 256, backing_ptr + page_num * 256, 256);
                    //assign the value accordingly
                    signed char value1 = main_memo[frame_num * 256 + off_set];
//                    printf(" %d \n",value1);

                    if(frame_num<256)
                        frame_num+=1;
                    tlb_pos+=1;
                    if(tlb_pos>15)
                        tlb_pos=0;
                    fprintf(create_csv,"%d,%d,%d\n", scan_result,p_address,value1);
                }
                else if(check_page_t(page_num,page_table,256)>=0){

                    //if page table hit, update the TLB table
                    tlb[tlb_pos].key = page_table[check_page_t(page_num,page_table,256)].key;
                    tlb[tlb_pos].value = page_table[check_page_t(page_num,page_table,256)].value;
                    p_address=combine_256(page_table[check_page_t(page_num,page_table,256)].value,off_set);
                    //read the pth page accordingly
                    memcpy(main_memo + frame_num * 256, backing_ptr + page_num * 256, 256);
                    //assign the value accordingly
                    signed char value1 = main_memo[frame_num * 256 + off_set];
                    fprintf(create_csv,"%d,%d,%d\n", scan_result,p_address,value1);

                    tlb_pos+=1;
                    if(tlb_pos>15)
                        tlb_pos=0;

                }

            }

            else if(check_tlb(page_num,tlb)!=-1){
                tlb_hit+=1;
                //read the pth page accordingly
                memcpy(main_memo + frame_num * 256, backing_ptr + page_num * 256, 256);
                //assign the value accordingly
                signed char value1 = main_memo[frame_num * 256 + off_set];
                p_address=combine_256(tlb[check_tlb(page_num,tlb)].value,off_set);
                fprintf(create_csv,"%d,%d,%d\n", scan_result,p_address,value1);

            }


            value+=1;
            //TLB miss



        }

        //printf("%.2f \n",((double) tlb_hit/total_address)*100);
        //printf("%.2f ",((double) num_page_fault/total_address)*100);
        fprintf(create_csv, "Page Faults Rate, %.2f%%,\n", ((double) num_page_fault / total_address) * 100);
        fprintf(create_csv, "TLB Hits Rate, %.2f%%,", ((double) tlb_hit / total_address) * 100);

        fclose(create_csv);
        //closing fp
        fclose(fp);
        return 0;
    }
}




