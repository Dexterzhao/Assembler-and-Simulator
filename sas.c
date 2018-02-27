#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "ctype.h"

#define MAXLEN 200
#define HASHSIZE 101
#define TYPEAMOUNT 8
#define INSTR_SYM {"HLT", "JMP", "CJMP", "OJMP", "CALL", "RET","PUSH",\
                    "POP", "LOADB", "LOADW", "STOREB", "STOREW", "LOADI",\
                    "NOP", "IN", "OUT", "ADD", "ADDI", "SUB", \
                    "SUBI", "MUL", "DIV", "AND", "OR", "NOR", \
                    "NOTB" ,"SAL", "SAR", "EQU", "LT", "LTE", "NOTC"\
                    }
typedef struct Label{
    char name[10];
    int line_num;
    struct Label *next;
}label;

char *g_instr_name[] = INSTR_SYM;
char instr_format[33] = "12222133444451667575777778778881";
struct Instr_list {
    struct Instr_list *next;
    char *name;
    int op_num;
};

typedef struct Variable{
    char name[10];
    int size;           /*个数*/
    int excursion;      /*偏移量*/
    int value[200];     /*值*/
    int type;           /*类型*/
    struct Variable *next;
}varia;

struct Instr_list *hashtab[HASHSIZE];
unsigned hash(char *);
struct Instr_list *lookup( char *);
char *strddup(char *);
struct Instr_list *install(char *,int );



int GetInstrCode(char *op_sym);                                     /*由指令助记符得指令操作码*/
unsigned long TransToCode(char *, int , label *, varia *);          /*指令的译码*/
int GetRegNum(char *, char *);                                       /*由寄存器名（A-G，Z)得到寄存器编号*/
int SaveVaria(char *, int , int , varia *, int );                   /*保存变量*/
void PrintVaria(FILE *, varia *);                                   /*在目标文件中输出变量*/
int SaveLabel(char *, int , label *);                               /*保存标号的信息在链表里*/

int main(int argc, char **argv)
{
    char a_line[MAXLEN];
    char op_sym[TYPEAMOUNT];
    int op_num , i;
    char *pcPos;
    int type;
    FILE *pfIn, *pfOut;
    label *head_label = (label *)malloc(sizeof(label));
    head_label->next = NULL;
    varia *head_varia = (varia *)malloc(sizeof(varia));
    head_varia->next = NULL;
    char label_temp[20];                /*存放标号名称*/
    char string[200];                   /*存放定义变量name和size的字符串*/
    char varia_size_str[20];
    int varia_size_int;
    int excursion_num = 0;
    int line_num = 0;                   /*行数*/



    int count;                          /*建立助记符操作码哈希表*/
    for(count = 0; count < 32; count++)
    {
        install(g_instr_name[count],count);

    }

    int n;
    if(argc<3)       /*检查命令行参数数目*/
    {
        printf("ERROR: no enough  command line arguments\n");
        return 0;
    }
    if((pfIn = fopen(argv[1], "r")) == NULL)/*打开源代码文件*/
    {
        printf("ERROR: cannot open file %s for reading!\n", argv[1]);
        return 0;
    }
    if((pfOut = fopen(argv[2], "w")) == NULL)/*打开目标代码文件*/
    {
        printf("ERROR: cannot open file %s for writing!\n", argv[2]);
        return 0;
    }
    //printf("ok");
    while(!feof(pfIn))
    {
        fgets(a_line, 200, pfIn);   /*从源文件取一行命令*/
        if((pcPos = strchr(a_line, '#')) != NULL)
        {
            *pcPos = '\0';     /*去掉注释*/
        }
        n = sscanf(a_line, "%s", op_sym); /*取指令助记符*/
        if(n<1)        /*空行和注释行的处理*/
        {
            continue;
        }
        if((pcPos = strchr(a_line, ':')) != NULL)
        {
            sscanf(a_line,"%[^:]",label_temp);
            SaveLabel(label_temp, line_num, head_label);
        }
        else if(strstr(a_line,"WORD") != NULL)
        {
            sscanf(a_line,"%*s %s",string);
            type = 2;
            if((pcPos = strchr(string, '[')) != NULL)
            {
                sscanf(string, "%*[^[][%[^]]", varia_size_str);         /*保存变量的大小*/
                varia_size_int = atoi(varia_size_str);
            }
            else
            {
                varia_size_int = 1;
            }
            SaveVaria(a_line, excursion_num, varia_size_int, head_varia, type);
            excursion_num += 2*varia_size_int;
            line_num--;
        }
        else if(strstr(a_line,"BYTE") != NULL)
        {
            sscanf(a_line,"%*s %s",string);
            type = 1;
            if((pcPos = strchr(string, '[')) != NULL)
            {
                sscanf(string, "%*[^[][%[^]]", varia_size_str);
                varia_size_int = atoi(varia_size_str);
            }
            else
            {
                varia_size_int = 1;
            }
            SaveVaria(a_line, excursion_num, varia_size_int, head_varia, type);
            excursion_num += varia_size_int;
            line_num--;
        }
        line_num++;
    }
    fclose(pfIn);
    pfIn=fopen(argv[1],"r");
    while(!feof(pfIn))
    {
        fgets(a_line, 200, pfIn);
        if((pcPos = strchr(a_line,'#'))!=NULL)
        {
            *pcPos='\0';
        }
        if((pcPos = strstr(a_line,"WORD"))!=NULL)
        {
            continue;
        }
        else if((pcPos = strstr(a_line,"BYTE"))!=NULL)
        {
            continue;
        }
        else if((pcPos = strchr(a_line, ':')) != NULL)
        {
            for(i = 0; a_line[i] != ':'; i++)
            {
                a_line[i]=' ';                          /*label处理*/
            }
            a_line[i]=' ';
        }
        n=sscanf(a_line,"%s",op_sym);
        if(n<1){
            continue;
        }
        op_num = GetInstrCode(op_sym);
        if(op_num > 31)
        {
            printf("ERROR: %s is a invalid instruction! \n", a_line);
            exit(-1);
        }
        fprintf(pfOut, "0x%08lx\n", TransToCode(a_line, op_num, head_label, head_varia));
    }
    PrintVaria(pfOut, head_varia);
    fclose(pfIn);
    fclose(pfOut);
    return 0;
}

/*由指令助记符得到指令操作码*/
int GetInstrCode( char *op_sym)
{
    struct Instr_list *np;
    if((np=lookup(op_sym))!=NULL)
        return np->op_num;
    return 0;
}

unsigned long TransToCode(char *instr_line, int instr_num, label *head1, varia *head2)
{
    unsigned long op_code;
    unsigned long arg1, arg2, arg3;
    unsigned long instr_code = 0ul;
    char op_sym[8], reg0[8], reg1[8], reg2[8];
    unsigned long addr;
    int immed, port;
    char string[20];
    label *p1;
    varia *p2;
    int n;

    switch(instr_format[instr_num])     /*根据指令格式，分别进行译码*/
    {
        case '1':
        {
            op_code = instr_num;
            instr_code = op_code << 27;
            break;
        }
        case '2':
        {
            n = sscanf(instr_line, "%s %s", op_sym, string);

            if(n<2)
            {
                printf("ERROR:bad instruction format!\n");
                exit(-1);
            }
            for(p1 = head1->next; p1 != NULL; p1 = p1->next)
            {

                if(strcmp(p1->name,string)==0)
                {

                    break;
                }
            }
            if(p1 == NULL)
            {
                printf("ERROR:%s wrong instruction line!",instr_line);
                exit(-1);
            }
            addr = (unsigned long)((p1->line_num)*4);                           /*地址为行数乘四*/
            op_code = GetInstrCode(op_sym);
            instr_code = (op_code << 27) | (addr & 0x00ffffff);
            break;
        }
        case '3':
        {
            n = sscanf(instr_line, "%s %s", op_sym, reg0);
            if(n < 2)
            {
                printf("ERROR:bad instruction format!\n");
                exit(-1);
            }
            op_code = GetInstrCode(op_sym);

            arg1 = GetRegNum(instr_line, reg0);
            instr_code = (op_code << 27) | (arg1 << 24);
            break;
        }
        case '4':
        {
            n = sscanf(instr_line, "%s %s %s", op_sym, reg0, string);
            if(n < 3)
            {
                printf("ERROR:bad instruction format!\n");
                exit(-1);
            }
            for(p2 = head2->next; p2 != NULL; p2 = p2->next)
            {
                if(strcmp(p2->name ,string)==0)
                {
                    break;
                }
            }
            if(p2 == NULL)
            {
                printf("ERROR:%s,wrong instruction line!",instr_line);
                exit(-1);
            }
            addr = (unsigned long)(p2->excursion);
            op_code = GetInstrCode(op_sym);
            arg1 = GetRegNum(instr_line, reg0);
            instr_code = (op_code << 27) | (arg1 << 24) | (addr&0x00ffffff);
            break;
        }
        case '5':
        {
            n = sscanf(instr_line, "%s %s %i", op_sym, reg0, &immed);
            if(n < 3)
            {
                printf("ERROR:bad instruction format!\n");
                exit(-1);
            }
            op_code = GetInstrCode(op_sym);
            arg1 = GetRegNum(instr_line, reg0);
            instr_code = (op_code << 27) | (arg1 << 24) | (immed & 0x0000ffff);
            break;
        }
        case '6':
        {
            n = sscanf(instr_line, "%s %s %i", op_sym, reg0, &port);
            if(n < 3)
            {
                printf("ERROR:bad instruction format!\n");
                exit(-1);
            }
            op_code =GetInstrCode(op_sym);
            arg1 = GetRegNum(instr_line, reg0);
            instr_code = (op_code << 27) | (arg1 << 24) | (port & 0x0000ffff);
            break;
        }
        case '7':
        {
            n = sscanf(instr_line, "%s%s%s%s", op_sym, reg0, reg1, reg2);
            if(n < 4)
            {
                printf("ERROR:bad instruction format!\n");
                exit(-1);
            }
            op_code = GetInstrCode(op_sym);
            arg1 = GetRegNum(instr_line, reg0);
            arg2 = GetRegNum(instr_line, reg1);
            arg3 = GetRegNum(instr_line, reg2);
            instr_code = (op_code << 27) | (arg1 << 24) | (arg2 << 20) | (arg3 << 16);
            break;
        }
        case '8':
        {
            n = sscanf(instr_line, "%s%s%s", op_sym, reg0, reg1);
            if(n < 3)
            {
                printf("ERROR:bad instruction format!\n");
                exit(-1);
            }
            op_code = GetInstrCode(op_sym);
            arg1 = GetRegNum(instr_line, reg0);
            arg2 = GetRegNum(instr_line, reg1);
            instr_code = (op_code << 27) | (arg1 << 24) | (arg2 << 20);
            break;
        }
    }
    return instr_code;           /*返回目标代码*/
}
/*由寄存器名（A-G,Z）得到寄存器编号*/
int GetRegNum(char *instr_line, char *reg_name)
{
    int reg_num;
    if(tolower(*reg_name) == 'z')
    {
        reg_num = 0;
    }
    else if((tolower(*reg_name) >= 'a') && (tolower(*reg_name) <= 'g'))
    {
        reg_num = tolower(*reg_name) - 'a' + 1;
    }
    else
    {
        printf("ERROR:bad register name in %s!\n", instr_line);
        exit(-1);
    }
    return reg_num;
}

int SaveLabel(char *label_name, int line_num, label *head)
{
    label *np;
    for(np = head; np->next != NULL; np = np->next)
        ;
    label *p = (label *)malloc(sizeof(label));
    np->next = p;
    p->line_num = line_num;
    strcpy(p->name, label_name);
    p->next = NULL;
    return 0;
}

int SaveVaria(char *a_line, int excursion, int size, varia *head, int type)
{
    varia *p1;
    short value1;
    char *p = NULL;
    char value_str[10];
    int i, k;
    char varianame_temp[10];
    char string[200];
    int assign;
    sscanf(a_line,"%*s %s",string);
    sscanf(string,"%[^[]",varianame_temp);

    for(p1 = head; p1->next != NULL; p1 = p1->next)
        ;

    varia *pnew = (varia *)malloc(sizeof(varia));
    p1->next = pnew;
    pnew->excursion = excursion;
    pnew->size = size;
    pnew->type = type;
    strcpy(pnew->name, varianame_temp);
    pnew->next = NULL;

    for(i = 0; i < size; i++)
    {
        pnew->value[i] = 0;
    }

    if(p = strchr(a_line,'='))                   /*当只存一个值时*/
    {
        if(size == 1)
        {
            sscanf(a_line, "%*[^=]=%d", &value1);
            pnew->value[0] = value1;
        }
        else
        {
            for(; p != NULL; p++)
            {
                if(*p == '{')
                {
                    ++p;
                    assign = 1;
                    k = 0;
                    while(*p != '}')
                    {
                        for(i = 0; *p != ',' && *p != '}'; p++)
                        {
                            value_str[i++] = *p;
                        }
                        value_str[i] = '\0';
                        pnew->value[k++] = atoi(value_str);
                        if(*p == ',')
                        {
                            ++p;
                        }
                    }
                }
                else if(*p == '"')
                {
                    assign = 1;
                    ++p;
                    k = 0;
                    for(i = 0; *p != '"'; p++)
                    {
                        pnew->value[k++] = *p;
                    }
                    pnew->value[k] = '\0';
                }
                if(assign)
                    break;
            }
        }
    }
    return 0;
}

char *strddup(char *s)
{
    char *p;
	p = (char *) malloc(strlen(s)+1);
	if (p != NULL)
		strcpy(p, s);
	return p;

}

unsigned hash(char *s)
{
	unsigned hashval;

	for (hashval = 0; *s != '\0'; s++)
		hashval = *s + 31 * hashval;
	return hashval % HASHSIZE;
}

struct Instr_list *lookup( char *s)
{
	struct Instr_list *np;

	for (np = hashtab[hash(s)];  np != NULL; np = np->next)
		if (strcmp(s, np->name) == 0)
			return np;     /* found */
	return NULL;           /* not found */
}

struct Instr_list *install(char *name, int number)
{
	struct Instr_list *np;
	unsigned hashval;
	if ((np = lookup(name)) == NULL) { /* not found */

		np = (struct Instr_list *) malloc(sizeof(struct Instr_list));
		if (np == NULL || (np->name = strddup(name)) == NULL)
			return NULL;
		hashval = hash(name);
		np->next = hashtab[hashval];
		hashtab[hashval] = np;
		np->op_num=number;
       } else       /* already there */
			free((void *) np->op_num);

	return np;
}

void PrintVaria(FILE *pfOut, varia *head)
{
    varia *p;
    int i;
    long k = 0;
    unsigned long addr = 0ul;
    for(p = head->next; p != NULL; p = p->next)
    {
        for(i = 0; i < p->size; i++)
        {
            if((k % 4) == 0)
            {
                if((p->type) == 1)
                {
                    addr += p->value[i];
                    k += 1;
                }
                else if((p->type) == 2)
                {
                    addr += (p->value[i])<<8;
                    k += 2;
                }
            }
            else if((k % 4) == 1)
            {
                if((p->type) == 1)
                {
                    addr += (p->value[i])<<8;
                    k += 1;
                }
                else if((p->type) == 2)
                {
                    addr += (p->value[i])<<16;
                    k += 2;
                }
            }
            else if((k % 4) == 2)
            {
                if((p->type) == 1)
                {
                    addr += (p->value[i])<<16;
                    k += 1;
                }
                else if((p->type) == 2)
                {
                    addr += (p->value[i])<<24;
                    k += 2;
                    fprintf(pfOut, "0x%08lx\n", addr);
                    addr = 0ul;
                }
            }
            else if((k % 4) == 3)
            {
                if((p->type) == 1)
                {
                    addr += (p->value[i])<<24;
                    k += 1;
                    fprintf(pfOut, "0x%08lx\n", addr);
                    addr = 0ul;
                }
                else if((p->type) == 2)
                {
                    addr += ((p->value[i])&0x00ff)<<24;
                    k += 2;
                    fprintf(pfOut, "0x%08lx\n", addr);
                    addr = (p->value[i])>>8;
                }
            }
        }
    }
    if((k % 4) != 0)
    {
        fprintf(pfOut, "0x%08lx\n", addr);
    }
    fprintf(pfOut, "0x%08lx\n", k);
    return ;
}


