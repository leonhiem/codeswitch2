#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>
#include <string.h>

int debug=0;

// generate random number:
uint16_t get_number16(uint16_t min,uint16_t max)
{
    float rf=(float)rand() / (float)RAND_MAX * (float)(max+1);
    uint16_t r = (uint16_t)rf;

    if(r<min) r=min;
    
    //printf("r=%d\n",r);
    return (uint16_t)r;
}
uint32_t get_number32(uint32_t min,uint32_t max)
{
    float rf=(float)rand() / (float)RAND_MAX * (float)(max+1);
    uint32_t r = (uint32_t)rf;

    if(r<min) r=min;
    
    //printf("r=%d\n",r);
    return (uint32_t)r;
}

/*
 * id: 9 digit with a range from 000000000 up to 999999999
 * number of days, use 3 digits with a range from 000 up to 999
 * sequence, I do not have any specific requirements, 2 or 3 digits seem fine
 *
 * Some notes about the number of bits required:
 * - id max is: 999999999  this needs 30 bits 
 *     (2^30 = 1073741824)
 * - days max is: 999 this needs 10 bits (2^10 = 1024)
 * - seq max is: 99 this needs 7 bits (2^7 = 128)
 *
 * total nr of bits= 30+10+7=47  (2^47 = 140737488355328)
 */

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


uint64_t generate_rcode(uint64_t code, unsigned int seq)
{
    int i;
    char str[16];
    char str1[16];
    char str2[16];
    sprintf(str,"%012lu",code);
    for(i=0;i<seq;i++) {
        strncpy(str1,str,11);
        str1[11]='\0';
        strncpy(str2,&str[11],1);
        str2[1]='\0';
        //printf("str1=%s str2=%s\n",str1,str2);
        strcpy(str,str2);
        strcat(str,str1);
    }
    //printf("code=%012lu seq=%02d ",code,seq);
    code = strtoul(str,NULL,10);
    //printf("sizeof=%ld\n",sizeof(code));
    //printf("==> code=%012lu\n",code);
    return code;
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
        //printf("str1=%s str2=%s\n",str1,str2);
        strcpy(str,str2);
        strcat(str,str1);
    }
    //printf("==> code=%s\n",str);

    for(seqc=0;seqc<100;seqc++) { 
        if(seqcode==seqlut[seqc]) break;
    }
    *seq=seqc;
    //printf("seqcode=%u --> seqlut --> seq=%u\n",seqcode,seqc);

    strcpy(str1,str);
    strcpy(str2,str);
    // 123456789012
    // 0        9
    str1[9]='\0';
    str2[8]=' ';
    //    printf("str1=%s str2=%s\n",str1,&str2[9]);
    *id = strtoul(str1,NULL,10);
    *days = strtoul(&str2[9],NULL,10);
    return 1;
}


int main(int argc, char* argv[])
{
    uint64_t code;
    unsigned int shsid,days,seq=0,runs=100,i,r;
    unsigned int rruns=10000;
    uint32_t chk_shsid;
    uint16_t chk_days;
    uint8_t chk_seq;
    char code_str[32];

    for(r=0;r<rruns;r++) {
      shsid=get_number32(1,999999999);
      days=get_number16(1,999);
      seq=0;
      for(i=0;i<runs;i++) {
          code = (uint64_t)shsid * 1000UL;
          code += (uint64_t)days;
          code = generate_rcode(code,seqlut[seq]);
          printf("codegen id=%09u days=%03d seq=%02d ==> code = %09u%03u%02u -> ",
                 shsid,days,seq,shsid,days,seqlut[seq]);
          sprintf(code_str,"%012lu%02u",code,seqlut[seq]);

          printf("%s -> ",code_str);
          crypt_digits(code_str);
          printf("%s\n",code_str);
  
          // decode, test and check:
          if(decode_rcode(code_str,&chk_seq,&chk_shsid,&chk_days) <0) exit(EXIT_FAILURE);
          printf("codechk id=%09u days=%03d seq=%02d <== code = %s\n",
                 chk_shsid,chk_days,chk_seq,code_str);
  
          if(chk_seq != seq) { fprintf(stderr,"chk_seq != seq\n"); exit(EXIT_FAILURE);}
          if(chk_shsid != shsid) { fprintf(stderr,"chk_shsid != shsid\n"); exit(EXIT_FAILURE);}
          if(chk_days != days) { fprintf(stderr,"chk_days != days\n"); exit(EXIT_FAILURE);}
  
          seq++;
      }
    }
    printf("PASS\n");
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
