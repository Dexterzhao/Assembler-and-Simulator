#include <stdio.h>
#include <stdlib.h>
#include <io.h>
#include <string.h>

/*定义宏来模拟指令的解码*/
#define REG0 ((IR >> 24)& 0x07)
#define REG1 ((IR >> 20)& 0x0F)
#define REG2 ((IR >> 16)& 0x0F)
#define IMMEDIATE (IR & 0xFFFF)
#define ADDRESS (IR & 0xFFFFFF)
#define PORT (IR & 0xFF)
#define OPCODE ((IR >> 27)& 0x1F)

typedef struct _PROG_STATE_WORD
{
  unsigned short overflow_flg:1;
  unsigned short compare_flg:1;
  unsigned short reserve:14;
}PROG_STATE_WORD;

typedef struct _ES
{
    short reg[7];
    unsigned short overflow_flg;
    unsigned short compare_flg;
    unsigned short reserve;
    unsigned long *pc_address;
}es;


unsigned char *MEM;    /*用动态存储区模拟内存,大小由命令行参数确定*/
unsigned long * PC;     /*指令计数器，用来存放下条指令的内存地址*/
short GR[8];            /*通用寄存器的模拟*/
PROG_STATE_WORD PSW;
unsigned long IR;
unsigned long *CS;
unsigned char *DS;
short *SS;
es *ES;


int HLT(void);
int JMP(void);
int CJMP(void);
int OJMP(void);
int CALL(void);
int RET(void);
int PUSH(void);
int POP(void);
int LOADB(void);
int LOADW(void);
int STOREB(void);
int STOREW(void);
int LOADI(void);
int NOP(void);
int IN(void);
int OUT(void);
int ADD(void);
int ADDI(void);
int SUB(void);
int SUBI(void);
int MUL(void);
int DIV(void);
int AND(void);
int OR(void);
int NOR(void);
int NOTB(void);
int SAL(void);
int SAR(void);
int EQU(void);
int LT(void);
int LTE(void);
int NOTC(void);

int main(int argc,char **argv)
{
    unsigned long instruction;
    unsigned long mem_size = 0x1000000;
    /*函数指针数组，用于指令对应函数的调用*/
    int ( *ops[])(void)={HLT,JMP,CJMP,OJMP,CALL,RET,PUSH,POP,LOADB,\
                         LOADW,STOREB,STOREW,LOADI,NOP,IN,OUT,ADD,\
                         ADDI,SUB,SUBI,MUL,DIV,AND,OR,NOR,NOTB,SAL,\
                         SAR,EQU,LT,LTE,NOTC};
    FILE *pfIn;
    int ret=1;
    long length;

    if(argc<2){
       printf("ERROR:no enough command line arguments!\n");
       exit(-1);
    }
    /*向系统申请动态存储区，模拟内存*/
    if((MEM = (unsigned char * )malloc(mem_size)) == NULL)
    {
       printf("ERROR:fail to allocate memory!\n");
       exit(-1);
    }
    PC = (unsigned long *)MEM;    /*使指令计数器指向模拟内存的顶端*/
    if((pfIn = fopen(argv[1], "r")) == NULL)
    {
        printf("ERROR:cannot open the file %s for reading!\n", argv[1]);
        exit(-1);
    }
    CS = PC;
    while(!feof(pfIn))
    {
       fscanf(pfIn,"%li",&instruction);
       memcpy(PC,&instruction,sizeof(instruction));
       PC++;
    }
    ES = MEM - sizeof(es) + mem_size;
    SS = (short *)PC;
    PC--;
    length = *PC;
    if(length%4 == 0)
    {
        DS = (unsigned char *)(PC -= length/4);
    }
    else
    {
        DS = (unsigned char *)(PC -= (length/4 + 1));
    }
    DS = DS-3;
    fclose(pfIn);
    PC = (unsigned long *)MEM;      /*使PC指向模拟内存MEM顶端的第一条指令*/
    while(ret)                      /*模拟处理器执行指令*/
   {
      IR = *PC;                     /*取指：将PC指示的指令加载到指令寄存器IR */
      PC++;                         /*PC指向下一条执行指令*/
      ret = ( *ops[OPCODE])();      /*解码并执行指令*/
   }
   free(MEM);
   return 0;
}

int HLT(void)
{
    return 0;
}

int JMP(void)
{
    PC=(unsigned long *)(MEM+ADDRESS);
    return 1;
}

int CJMP(void)
{
    if(PSW.compare_flg){
        PC=(unsigned long *)(MEM+ADDRESS);
    }
    return 1;
}

int OJMP(void)
{
    if(PSW.overflow_flg){
        PC=(unsigned long *)(MEM+ADDRESS);
    }
    return 1;
}

int CALL(void)
{
    int i;
    for(i = 0; i < 7; i++)
    {
        ES->reg[i] = GR[i+1];
    }
    ES->compare_flg = PSW.compare_flg;
    ES->overflow_flg = PSW.overflow_flg;
    ES->pc_address = PC;
    ES--;
    PC=(unsigned long *)(MEM + ADDRESS);
    return 1;
}

int RET(void)
{
    int i;
    ES++;
    for(i = 0; i<7; i++){
        GR[i+1] = ES->reg[i];
    }
    PSW.compare_flg = ES->compare_flg;
    PSW.overflow_flg = ES->overflow_flg;
    PC = ES->pc_address;
    return 1;
}

int PUSH(void)
{
    *SS = GR[REG0];
    SS++;
    return 1;
}

int POP(void)
{
    if(REG0==0){
        printf("ERROR!\n");
        exit(-1);
    }
    SS--;
    GR[REG0] = *SS;
    return 1;
}

int LOADB(void)
{
    GR[REG0]=(short)(*(DS + ADDRESS + GR[7] - 1));
    return 1;
}

int LOADW(void)
{
    GR[REG0]=(short)((*(DS+ADDRESS+GR[7]-1)<<8)+*(DS+ADDRESS+GR[7]));
    return 1;
}

int STOREB(void)
{
    *(DS + ADDRESS + GR[7] - 1) = GR[REG0];
    return 1;
}

int STOREW(void)
{
    *(DS + ADDRESS + GR[7] - 1) = ((GR[REG0]&0xff00) >> 8);
    *(DS + ADDRESS + GR[7]) = GR[REG0]&0x00ff;
    return 1;
}

int LOADI(void)
{
    GR[REG0]=(short)(IMMEDIATE);
    return 1;
}

int NOP(void)
{
    return 1;
}

int IN(void)
{
    read(0,(char *)(GR+REG0),1);
    return 1;
}

int OUT(void)
{
    write(1,(int *)(GR+REG0),1);
    return 1;
}

int ADD(void)
{
    GR[REG0]=GR[REG1]+GR[REG2];
    if(GR[REG2]>0){
        if(GR[REG0]<GR[REG1]){
            PSW.overflow_flg=1;
        }
        else{
            PSW.overflow_flg=0;
        }
    }
    else if(GR[REG2]<0){
        if(GR[REG0]>GR[REG1]){
            PSW.overflow_flg=1;
        }
        else{
            PSW.overflow_flg=0;
        }
    }
    else{
        PSW.overflow_flg=0;
    }
    return 1;
}

int ADDI(void)
{
    int  n;
    n=GR[REG0]+IMMEDIATE;
    if(IMMEDIATE > 0){
        if(n < GR[REG0]){
            PSW.overflow_flg=1;
        }
        else{
            PSW.overflow_flg=0;
        }
    }
    else if(IMMEDIATE < 0){
        if(n > GR[REG0]){
            PSW.overflow_flg=1;
        }
        else{
            PSW.overflow_flg=0;
        }
    }
    else{
        PSW.overflow_flg=0;
    }
    GR[REG0] = n;
    return 1;
}

int SUB(void)
{
    GR[REG0]=GR[REG1]-GR[REG2];
    if(GR[REG2]>0){
        if(GR[REG0]>GR[REG1]){
            PSW.overflow_flg=1;
        }
        else{
            PSW.overflow_flg=0;
        }
    }
    else if(GR[REG2]<0){
        if(GR[REG0]<GR[REG1]){
            PSW.overflow_flg=1;
        }
        else{
            PSW.overflow_flg=0;
        }
    }
    else{
        PSW.overflow_flg=0;
    }
    return 1;
}

int SUBI(void)
{
    int  n;
    n = GR[REG0] - IMMEDIATE;
    if(IMMEDIATE < 0){
        if(n < GR[REG0]){
            PSW.overflow_flg=1;
        }
        else{
            PSW.overflow_flg=0;
        }
    }
    else if(IMMEDIATE > 0){
        if(n > GR[REG0]){
            PSW.overflow_flg=1;
        }
        else{
            PSW.overflow_flg=0;
        }
    }
    else{
        PSW.overflow_flg=0;
    }
    GR[REG0] = n;
    return 1;
}

int MUL(void)
{
    GR[REG0]=GR[REG1]*GR[REG2];
    if(abs(GR[REG2])>1){
        if(abs(GR[REG0])>abs(GR[REG1])){
            PSW.overflow_flg=0;
        }
        else{
            PSW.overflow_flg=1;
        }
    }
    else{
        PSW.overflow_flg=0;
    }
    return 1;
}

int DIV(void)
{
    if(GR[REG2] == 0)
    {
        printf("0 cannot be divided\n");
        exit(-1);
    }
    GR[REG0]=GR[REG1]/GR[REG2];
    return 1;
}

int AND(void)
{
    GR[REG0]=GR[REG1]&GR[REG2];
    return 1;
}

int OR(void)
{
    GR[REG0]=GR[REG1]|GR[REG2];
    return 1;
}

int NOR(void)
{
    GR[REG0]=GR[REG1]^GR[REG2];
    return 1;
}

int NOTB(void)
{
    GR[REG0]=~GR[REG1];
    return 1;
}

int SAL(void)
{
    GR[REG0]=GR[REG1]<<GR[REG2];
    return 1;
}

int SAR(void)
{
    GR[REG0]=GR[REG1]>>GR[REG2];
    return 1;
}

int EQU(void)
{
    PSW.compare_flg=(GR[REG0]==GR[REG1]);
    return 1;
}

int LT(void)
{
    PSW.compare_flg=(GR[REG0]<GR[REG1]);
    return 1;
}

int LTE(void)
{
    PSW.compare_flg=(GR[REG0]<=GR[REG1]);
    return 1;
}

int NOTC(void)
{
    PSW.compare_flg=!PSW.compare_flg;
    return 1;
}
