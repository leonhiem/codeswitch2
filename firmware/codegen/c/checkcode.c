#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>
#include <string.h>


void print_binary(uint64_t val, int nof_bits);

const uint8_t seqlut[] = {
  35, 86, 69, 93, 99, 63, 74, 15, 84, 88,
  82, 26, 50, 43, 58, 55, 97, 71, 52, 96,
  13, 67, 56, 20, 27, 46,  9, 19, 73, 38,
   1,  2, 77, 64,  5, 57,  3, 25, 21, 31,
  61, 16,  8, 75, 24, 92, 78, 30, 33,  6,
  10, 44, 72, 83, 53, 95, 49, 11, 18, 42,
  40, 28, 76, 17,  0, 14, 12, 80, 47, 87,
  68, 98, 32, 51, 39, 81, 62, 34, 45, 54,
  85, 36,  4, 37, 41, 66, 91, 70, 89, 60,
  79, 94, 29, 90, 22,  7, 65, 48, 59, 23
};

const uint8_t digitlut[] = {1, 7, 2, 6, 4, 0, 9, 8, 5, 3};

void crypt_digits(char *code_str)
{
    int i;
    //printf("crypt_digits(%s)\n",code_str);
    for(i=0;i<14;i++) {
        code_str[i]='0'+digitlut[code_str[i]-'0'];
    }
    //printf("=(%s)\n",code_str);
}
void decrypt_digits(char *code_str)
{
    int i,c;
    //printf("decrypt_digits(%s)\n",code_str);
    for(i=0;i<14;i++) {
        for(c=0;c<10;c++) {
            if(code_str[i] == (digitlut[c]+'0')) break;
        }
        code_str[i]=c+'0';
    }
    //printf("=(%s)\n",code_str);
}

int decode_rcode(char *code_str, uint8_t *seq, uint32_t *id, uint16_t *days)
{
    uint64_t code;
    uint8_t seqc,seqcode;
    int i;
    char str[16];
    char str1[16];
    char str2[16];

    decrypt_digits(code_str);

    seqcode = strtoul(&code_str[12],NULL,10);
    if(seqcode < 0 || seqcode > 99) {
        printf("Error: illegal seq)\n");
        return -1;
    }
    strcpy(str,code_str);
    str[12]='\0';
    // rol <<
    for(i=0;i<seqcode;i++) {
        strncpy(str1,str,1); // take most left digit
        str1[1]='\0';
        strncpy(str2,&str[1],11); // take other right digits
        str2[11]='\0';
        printf("str1=%s str2=%s\n",str1,str2);
        strcpy(str,str2);
        strcat(str,str1);
    }
    printf("==> code=%s\n",str);

    for(seqc=0;seqc<100;seqc++) {
        if(seqcode==seqlut[seqc]) break;
    }
    *seq=seqc;
    printf("seqcode=%u --> seqlut --> seq=%u\n",seqcode,seqc);

    strcpy(str1,str);
    strcpy(str2,str);
    // 123456789012
    // 0        9
    str1[9]='\0';
    str2[8]=' ';
    printf("str1=%s str2=%s\n",str1,&str2[9]);
    *id = strtoul(str1,NULL,10);
    *days = strtoul(&str2[9],NULL,10);
    return 1;
}


int main(int argc, char* argv[])
{
    uint32_t shsid;
    uint16_t days;
    uint8_t seq;
    char argv_cpy[32];

    if(argc < 2) {
        fprintf(stderr,"Usage:   %s code\n",argv[0]);
        fprintf(stderr,"Example: %s 131418492926176\n",argv[0]);
        exit(EXIT_FAILURE);
    }

    if(strlen(argv[1]) != 14) {
        printf("Error: given code must be 14 digits (tip: use heading 0)\n");
        exit(EXIT_FAILURE);
    }

    // 2 least significant digits are sequence number
    strcpy(argv_cpy,argv[1]);
    if(decode_rcode(argv_cpy,&seq,&shsid,&days) <0) exit(EXIT_FAILURE);
    printf("[seq=%u days=%u id=%u]\n",seq,days,shsid);
}


void print_binary(uint64_t val, int nof_bits)
{
    uint64_t n=val;
    char str[100];
    int i,nof_bits_cnt=0;
    memset(str,0,sizeof(str));

    while (n) {
        if (n & 1)
            strcat(str,"1");
        else
            strcat(str,"0");

        nof_bits_cnt++;
        n >>= 1;
    }
    while(nof_bits_cnt < nof_bits) {
        strcat(str,"0");
        nof_bits_cnt++;
        if(strlen(str) >= sizeof(str)-1) break;
    }
    // print reverse:
    for(i=strlen(str);i>=0;i--) {
        printf("%c",str[i]);
    }
    printf("\n");
}
