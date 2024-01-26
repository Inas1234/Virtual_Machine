#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#define MAX_STACK_SIZE 256


typedef enum {
    PUSH,
    POP,
    ADD,
    PRINT,
    EXIT,
    SUB,
    MUL,
    DIV,
    MOD,
    JMP,
} Opperation;

typedef struct{
    char *name;
    int address;
} Label;


typedef struct{
    Opperation op;
    void* value;
} Instruction;

typedef struct{
    int *stack;
    int sp;
    Instruction *code;
    int pc;
    Label *labels;
    int labelCount;
    char *entryPoint;
} Machine;

void iniMachine(Machine *m, Instruction *code){
    m->stack = (int *)malloc(sizeof(int) * MAX_STACK_SIZE);
    m->sp = 0;
    m->code = code;
    m->pc = 0;
    m->labels = NULL;
    m->labelCount = 0;
}

void freeMachine(Machine *m){
    free(m->stack);
    free(m->code);
    for (int i = 0; i < m->labelCount; i++) {
        free(m->labels[i].name);
    }
    free(m->labels);
    free(m->entryPoint);
}

void push(Machine *m, int value){
    m->stack[m->sp++] = value;
}

int pop(Machine *m){
    return m->stack[--m->sp];
}

int sub(Machine *m){
    int a = pop(m);
    int b = pop(m);
    return a - b;
}

int mul(Machine *m){
    int a = pop(m);
    int b = pop(m);
    return a * b;
}

float division(Machine *m){
    int a = pop(m);
    int b = pop(m);
    return a / b;
}

int mod(Machine *m){
    int a = pop(m);
    int b = pop(m);
    return a % b;
}

void addLabel(Machine *m, char *name, int address){
    m->labels = (Label *)realloc(m->labels, sizeof(Label) * (m->labelCount + 1));
    m->labels[m->labelCount].name = strdup(name); 
    m->labels[m->labelCount].address = address;
    m->labelCount++;
}

char * readFile(const char * file_name){
    FILE *file = fopen(file_name, "r");
    char *buffer = 0;
    long length;

    if (file){
        fseek(file, 0, SEEK_END);
        length = ftell(file);
        fseek(file, 0, SEEK_SET);
        buffer = malloc(length + 1);
        if (buffer){
            fread(buffer, 1, length, file);
            buffer[length] = '\0';
        }
        fclose(file);
    }

    return buffer;
}


void run(Machine *m){
    if (m->entryPoint != NULL) {
        int found = 0;
        for (int j = 0; j < m->labelCount; j++) {
            if (strcmp(m->labels[j].name, m->entryPoint) == 0) {
                m->pc = m->labels[j].address;
                found = 1;
                break;
            }
        }
        if (!found) {
            printf("Error: Entry point label '%s' not found.\n", m->entryPoint);
            return;  
        }
    }

    Instruction *i;
    int a, b;
    while(1){
        i = &m->code[m->pc++];
        switch(i->op){
            case PUSH:
                push(m, (int)(intptr_t)i->value);
                break;
            case POP:
                pop(m);
                break;
            case ADD:
                a = pop(m);
                b = pop(m);
                push(m, (int)(a + b));
                break;
            case SUB:
                push(m, sub(m));
                break;
            case MUL:
                push(m, mul(m));
                break;
            case DIV:
                push(m, division(m));
                break;
            case MOD:
                push(m, mod(m));
                break;
            case PRINT:
                printf("%d\n", m->stack[m->sp - 1]);
                break;
            case JMP:
                for (int j = 0; j < m->labelCount; j++) {
                    if (strcmp(m->labels[j].name, (char *)i->value) == 0) {
                        m->pc = m->labels[j].address;
                        break;
                    }
                }
                break;
            case EXIT:
                return;
        }
    }
}

Instruction* tokenize(Machine *m,char *code){
    if (code == NULL) return NULL;

    Instruction *instructions = (Instruction *)malloc(sizeof(Instruction) * 256);
    if (instructions == NULL) return NULL;

    int i = 0;
    char *token = strtok(code, " \n\t");
    while(token != NULL && i < 255){ 
        if(strcmp(token, "push") == 0){
            instructions[i].op = PUSH;
            token = strtok(NULL, " \n\t");
            if (token != NULL) {
                instructions[i].value = (void *)(intptr_t)atoi(token);
            } else {
                free(instructions);
                return NULL;
            }
        } else if(strcmp(token, "pop") == 0){
            instructions[i].op = POP;
        } else if(strcmp(token, "add") == 0){
            instructions[i].op = ADD;
        } else if(strcmp(token, "sub") == 0){
            instructions[i].op = SUB;
        } else if(strcmp(token, "mul") == 0){
            instructions[i].op = MUL;
        } else if(strcmp(token, "div") == 0){
            instructions[i].op = DIV;
        } else if(strcmp(token, "mod") == 0){
            instructions[i].op = MOD;
        } else if(strcmp(token, "print") == 0){
            instructions[i].op = PRINT;
        }
        else if (token[strlen(token) - 1] == ':') {
            token[strlen(token) - 1] = '\0';
            addLabel(m, token, i); 
            token = strtok(NULL, " \n\t"); 
            continue;
        }
        else if(strcmp(token, "jmp") == 0){
            printf("JMP Tokenized\n");
            instructions[i].op = JMP;
            token = strtok(NULL, " \n\t");
            if (token != NULL) {
                instructions[i].value = strdup(token);
            } else {
                free(instructions);
                return NULL;
            }
        }
        else if(strcmp(token, "global") == 0){
            token = strtok(NULL, " \n\t");
            if (token != NULL) {
                m->entryPoint = strdup(token);
            } else {
                free(instructions);
                return NULL;
            }
            
            continue;
        }
        else if(strcmp(token, "exit") == 0){
            instructions[i].op = EXIT;
            i++;
            break; 
        }
        token = strtok(NULL, " \n\t");
        i++;
    }
    if (i < 256) {
        instructions[i].op = EXIT;
    }

    return instructions;
}


int main(void){
    
    char *code = readFile("test.masm");
    Machine m;
    iniMachine(&m, NULL);
    m.code = tokenize(&m, code);
    free(code);
    run(&m);
    freeMachine(&m);
    return 0;
}