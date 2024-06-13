#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TAM_MEMORIA 16384
#define salto 3

#define CS 0
#define DS 1
#define ES 2
#define SS 3
#define KS 4
#define IP 5
#define SP 6
#define BP 7
#define CC 8
#define AC 9
#define EAX 10
#define EBX 11
#define ECX 12
#define EDX 13
#define EEX 14
#define EFX 15

typedef struct{
    char nombre[4];
    int valor;
} registro;

typedef struct{
    short int base;
    short int size;
} tds;

typedef struct {
    char* memoria;
    registro registros[16];
    tds tabla_segmentos[8];
    const char* nombreVMI;
    const char* nombreVMX;
    int memoriaTotal;
} TMV;

typedef struct {
    int valor;
    char tipo;
} operando;

typedef void (*punteroFuncion)(operando, operando, TMV*);

void inicializaReg(TMV* mv);
void inicializaTdS(TMV* mv);
void guardaCodigo(TMV* mv, const char *nombreArch);
int nroOperandosOperacion(char instruccion);
void disassembly(TMV mv);
void ejecuta(TMV* mv);

int getop(operando opA, TMV mv);
void setop(operando op, int valor, TMV* mv);
void ejecutaUnaInstruccion(TMV* mv);
void chequeaCodOpValido (char codOperacion, TMV mv);
void levantaVMI(TMV* mv);

int main(int argc, char const *argv[]){ 
    /*
    TMV mv;  
    mv.nombreVMX = "sample1.vmx";
    mv.nombreVMI = NULL;
    mv.memoriaTotal = TAM_MEMORIA;
    mv.memoria = (char*)malloc(mv.memoriaTotal * sizeof(char));

    inicializaTdS(&mv);
    inicializaReg(&mv);
    if (mv.nombreVMX != NULL) // esto es si HAY un .vmx
        guardaCodigo(&mv, mv.nombreVMX);
    else //si hay un vmi Y NO HAY UN VMX:
        levantaVMI(&mv);
    // esto es si lo pide con comandos
    disassembly(mv); 

    //esto es siempre
    ejecuta(&mv);
    free(mv.memoria); //main hardcodeado
    */
    TMV mv;
    int banderaDisassembly = 0;
    inicializaReg(&mv);
    inicializaTdS(&mv);
    
    // valores por defecto
    mv.nombreVMX = NULL;
    mv.nombreVMI = NULL;
    mv.memoriaTotal = TAM_MEMORIA;

    for (int i = 1; i < argc; i++){
        if (strstr(argv[i], ".vmx")){
            mv.nombreVMX = argv[i];
        } else if (strstr(argv[i], ".vmi")){
            mv.nombreVMI = argv[i];
        } else if (strncmp(argv[i], "m=", 2) == 0) {
            mv.memoriaTotal = atoi(argv[i] + 2) * 1024;
        } else if (strcmp(argv[i], "-d") == 0) {
            banderaDisassembly = 1;
        }
     }
     mv.memoria = (char*)malloc(mv.memoriaTotal * sizeof(char));

     if (mv.nombreVMX == NULL){
        levantaVMI(&mv);
     } else {
        guardaCodigo(&mv, mv.nombreVMX);
     }

    if (banderaDisassembly)
        disassembly(mv); 
    ejecuta(&mv);
    free(mv.memoria);
    return 0;
}
void inicializaTdS(TMV* mv){
    for (int i=0; i<8; i++){
        mv->tabla_segmentos[i].base=0;
        mv->tabla_segmentos[i].size=0;
    }    
}

void inicializaReg(TMV* mv){ 
    strcpy(mv->registros[DS].nombre, "DS");
    strcpy(mv->registros[CS].nombre, "CS");
    strcpy(mv->registros[ES].nombre, "ES");
    strcpy(mv->registros[SS].nombre, "SS");
    strcpy(mv->registros[KS].nombre, "KS");
    strcpy(mv->registros[IP].nombre, "IP");
    strcpy(mv->registros[SP].nombre, "SP");
    strcpy(mv->registros[BP].nombre, "BP");
    strcpy(mv->registros[CC].nombre, "CC");
    strcpy(mv->registros[AC].nombre, "AC");
    strcpy(mv->registros[EAX].nombre, "EAX");
    strcpy(mv->registros[EBX].nombre, "EBX");
    strcpy(mv->registros[ECX].nombre, "ECX");
    strcpy(mv->registros[EDX].nombre, "EDX");
    strcpy(mv->registros[EEX].nombre, "EEX");
    strcpy(mv->registros[EFX].nombre, "EFX");

    mv->registros[CS].valor =  0;
    mv->registros[DS].valor =  0;
    mv->registros[ES].valor =  0;
    mv->registros[SS].valor =  0;
    mv->registros[KS].valor =  0;
    mv->registros[IP].valor =  0;
    mv->registros[SP].valor =  0;
    mv->registros[BP].valor =  0;
    mv->registros[CC].valor =  0;
    mv->registros[AC].valor =  0;
    mv->registros[EAX].valor = 0;
    mv->registros[EBX].valor = 0;
    mv->registros[ECX].valor = 0;
    mv->registros[EDX].valor = 0;
    mv->registros[EEX].valor = 0;
    mv->registros[EFX].valor = 0;
}

void encontroError(char tipo, TMV* mv){
    switch (tipo){
        case 1: //instruccion invalida
            printf("La instruccion es invalida\n");
            mv->registros[IP].valor = mv->registros[CS].valor | mv->tabla_segmentos[mv->registros[CS].valor>>16].size;
            break;
        case 2: // division por cero
            printf("Se intento dividir por cero\n");
            mv->registros[IP].valor = mv->registros[CS].valor | mv->tabla_segmentos[mv->registros[CS].valor>>16].size;
            break;
        case 3: //fallo de segmento
            printf("Segmentation Fault\n");
            mv->registros[IP].valor = mv->registros[CS].valor | mv->tabla_segmentos[mv->registros[CS].valor>>16].size;
            break;
        case 4: //memoria insuficiente
            printf("Memoria Insuficiente\n");
            mv->registros[IP].valor = mv->registros[CS].valor | mv->tabla_segmentos[mv->registros[CS].valor>>16].size;
            break;
        case 5: //stack overflow
            printf("Stack Overflow\n");
            mv->registros[IP].valor = mv->registros[CS].valor | mv->tabla_segmentos[mv->registros[CS].valor>>16].size;
            break;
        case 6: //stack underflow
            printf("Stack Underflow\n");
            mv->registros[IP].valor = mv->registros[CS].valor | mv->tabla_segmentos[mv->registros[CS].valor>>16].size;
            break;
    }
}

int calculaDirFisica(int dirLogica, TMV *mv){ 
    char iTDS = (dirLogica >> 16) & 0x00000F;
    short int offset = dirLogica & 0x0000FFFF;
    int dirFisica = mv->tabla_segmentos[iTDS].base + offset;
    if (dirFisica > (mv->tabla_segmentos[iTDS].base + mv->tabla_segmentos[iTDS].size))
        encontroError(3, mv);
    if (dirFisica < mv->tabla_segmentos[iTDS].base)
        encontroError(3, mv);
    return dirFisica;
}

void guardaCodigo(TMV* mv, const char *nombreArch){
    char codEjecucion[6];
    char version;
    int i = 0, j, memoriatotal = 0, posicionCS, tamanioCS, tamanioKS, posicionKS;
    char instruccion, aux[2], auxmem;
    unsigned short int tamanio;
    int offsetEntryPoint;
    FILE *arch = fopen(nombreArch, "rb");
    if (arch == NULL)
        printf("No existe el archivo");
    else{
        fread (codEjecucion, sizeof(char), 5, arch);
        codEjecucion[5] = '\0';
        fread(&version, sizeof(char), 1, arch);
        if ((strcmp (codEjecucion, "VMX24") != 0) || ((version != 1) && (version != 2))){
            printf("El archivo no es un archivo ejecutable por esta maquina virtual");
            fclose(arch);
        }
        else{
            if (version == 1){
                printf("El archivo es valido, leyendo version 1...");               
                fread(aux, 2*sizeof(char), 1, arch);
                tamanio = ((aux[0] << 8) & 0x0000FF00) | (aux[1] & 0x000000FF);
                
                mv->tabla_segmentos[CS].base = 0;
                mv->tabla_segmentos[CS].size = tamanio;
                mv->tabla_segmentos[DS].base = tamanio;
                mv->tabla_segmentos[DS].size = TAM_MEMORIA-tamanio;
                mv->registros[DS].valor = 0x00010000;
                mv->registros[IP].valor = mv->tabla_segmentos[CS].base;
                
                fread(mv->memoria, sizeof(char), tamanio, arch);
                printf("El codigo ya fue guardado \n"); 
            }
            else {
                printf("El archivo es valido, leyendo version 2...");
                fseek(arch, 14, SEEK_SET);
                fread(aux, 2*sizeof(char), 1, arch); //lee tamanio ks
                tamanioKS = ((aux[0] << 8) & 0x0000FF00) | (aux[1] & 0x000000FF);
                if (tamanioKS > 0){ //setear KS como 0 en TDS
                    mv->tabla_segmentos[0].base = 0;
                    mv->tabla_segmentos[0].size = tamanioKS;
                    mv->registros[KS].valor = 0;                 
                    fseek(arch, 6, SEEK_SET);
                    memoriatotal= tamanioKS; 
                    j = 0;
                } 
                else{ //setear CS como 0 en tds
                    fseek(arch, 6, SEEK_SET);
                    fread(aux, 2*sizeof(char), 1, arch);
                    tamanio = ((aux[0] << 8) & 0x0000FF00) | (aux[1] & 0x000000FF);
                    mv->tabla_segmentos[0].base = 0;
                    mv->tabla_segmentos[0].size = tamanio;
                    mv->registros[CS].valor = 0;
                    mv->registros[KS].valor = -1;  
                    memoriatotal+=tamanio;
                    j = 1;
                }
                for (j; j<4; j++){
                    fread(aux, 2*sizeof(char), 1, arch);
                    tamanio = ((aux[0] << 8) & 0x0000FF00) | (aux[1] & 0x000000FF);
                    if (tamanio > 0) {
                        i++;
                        mv->tabla_segmentos[i].base = mv->tabla_segmentos[i-1].base + mv->tabla_segmentos[i-1].size;
                        mv->tabla_segmentos[i].size = tamanio;
                        mv->registros[j].valor = i << 16;
                        memoriatotal += tamanio;
                    }     
                    else
                        mv->registros[j].valor = -1;
                }      
                fseek(arch, 16, SEEK_SET);
                fread(aux, 2*sizeof(char), 1, arch);
                offsetEntryPoint = ((aux[0] << 8) & 0x0000FF00) | (aux[1] & 0x000000FF);
                if (memoriatotal > mv->memoriaTotal)
                    encontroError(4, mv);
                int tamanioHeader = 18;
                tamanioCS = mv->tabla_segmentos[mv->registros[CS].valor>>16].size;
                if (tamanioKS > 0){
                    fseek(arch, tamanioHeader + tamanioCS, SEEK_SET);
                    fread(mv->memoria, sizeof(char), tamanioKS, arch);
                }
                fseek(arch, tamanioHeader, SEEK_SET);
                fread(mv->memoria + tamanioKS, sizeof(char), tamanioCS, arch);  
                printf("El codigo ya fue guardado \n"); 

                mv->registros[SP].valor = (mv->registros[SS].valor | (mv->tabla_segmentos[mv->registros[SS].valor>>16].size & 0x0000FFFF));
                mv->registros[IP].valor = (mv->registros[CS].valor | offsetEntryPoint);
            } 
            printf("El codigo es: \n"); 
            for ( int i = 0; i < (tamanioCS+tamanioKS); i++)
                printf("%02X  ", mv->memoria[i] & 0x000000FF);
        }
        //fclose(arch);
    }
}

void levantaVMI(TMV* mv){
    char codEjecucion[6];
    char version;
    int i = 0, j, memoriatotal = 0, tamanioCS, tamanioKS, aux;
    char instruccion, auxM[2], auxmem;
    unsigned short int tamanio;
    FILE *arch = fopen(mv->nombreVMI, "rb");
    if (arch == NULL){
        printf("No existe el archivo");
    }
    else{
        fread (codEjecucion, sizeof(char), 5, arch);
        codEjecucion[5] = '\0';
        fread(&version, sizeof(char), 1, arch);
        if ((strcmp(codEjecucion, "VMI24") != 0) || ((version != 1))){
            printf("El archivo no es un archivo ejecutable por esta maquina virtual");
            fclose(arch);
        }
        else
            if (version == 1){
                printf("El archivo DE IMAGEN es valido, leyendo version 1...");               
                fread(auxM, 2*sizeof(char), 1, arch); // lee el tamanio de la memoria principal
                tamanio = ((auxM[0] << 8) & 0x0000FF00) | (auxM[1] & 0x000000FF);
                mv->memoriaTotal = tamanio * 1024;
                for (int i= 0; i<16; i++){ // 64 bytes de registros
                    for (int j=3; j>=0; j--){
                        fread(&aux, sizeof(char), 1, arch);
                        mv->registros[i].valor = (mv->registros[i].valor) | (aux << (8*j));
                    }
                }
                inicializaTdS(mv);
                for (int i=0; i<8; i++){ // 32 bytes de tds
                    for (int j=1; j>=0; j--){
                        fread(&aux, sizeof(char), 1, arch);
                        mv->tabla_segmentos[i].base = mv->tabla_segmentos[i].base | (aux << (8*j));
                    }
                    for (int j=1; j>=0; j--){
                        fread(&aux, sizeof(char), 1, arch);
                        mv->tabla_segmentos[i].size = mv->tabla_segmentos[i].size | (aux << (8*j));
                    }
                 }
                 
                fread(mv->memoria, sizeof(char), mv->memoriaTotal, arch); // memoria principal
                printf("memoria guardada!");
            }
            //fclose(arch);
            tamanioCS = mv->tabla_segmentos[mv->registros[CS].valor>>16].size;
            tamanioKS = mv->tabla_segmentos[mv->registros[KS].valor>>16].size;
            printf("El codigo es: \n"); 
            for ( int i = 0; i < (tamanioCS+tamanioKS); i++)
                printf("%02X  ", mv->memoria[i] & 0x000000FF);
    }
}

int leeOperando(operando* op, TMV mv){
    int valor = 0;
    int dirFisicaIP = calculaDirFisica(mv.registros[IP].valor, &mv);
    switch (op->tipo) { 
        case 0: // memoria
            valor = mv.memoria[dirFisicaIP] & 0x00FF;
            valor = valor << 8;
            valor = valor | (mv.memoria[dirFisicaIP + 1] & 0x00FF);
            valor = valor << 8;
            valor = valor | (mv.memoria[dirFisicaIP + 2] & 0x00FF);
            break;

        case 1: // inmediato
            valor = mv.memoria[dirFisicaIP] & 0x00FF;
            valor = valor << 8;
            valor = valor | (mv.memoria[dirFisicaIP + 1] & 0x00FF);
            break;

        case 2: // registro
            valor = mv.memoria[dirFisicaIP] & 0x00FF;
            break;
    }
    return valor;
}

void inicializaVecCodOp(char* vecCodOp[]){
    vecCodOp[0x00] = "MOV";
    vecCodOp[0x01] = "ADD";
    vecCodOp[0x02] = "SUB";
    vecCodOp[0x03] = "SWAP";
    vecCodOp[0x04] = "MUL";
    vecCodOp[0x05] = "DIV";
    vecCodOp[0x06] = "CMP";
    vecCodOp[0x07] = "SHL";
    vecCodOp[0x08] = "SHR";
    vecCodOp[0x09] = "AND";
    vecCodOp[0x0A] = "OR";
    vecCodOp[0x0B] = "XOR";
    vecCodOp[0x0C] = "RND";

    vecCodOp[0x10] = "SYS";
    vecCodOp[0x11] = "JMP";
    vecCodOp[0x12] = "JZ";
    vecCodOp[0x13] = "JP";
    vecCodOp[0x14] = "JN";
    vecCodOp[0x15] = "JNZ";
    vecCodOp[0x16] = "JNP";
    vecCodOp[0x17] = "JNN";
    vecCodOp[0x18] = "LDL";
    vecCodOp[0x19] = "LDH";
    vecCodOp[0x1A] = "NOT";
    vecCodOp[0x1B] = "PUSH";
    vecCodOp[0x1C] = "POP";
    vecCodOp[0x1D] = "CALL";
    vecCodOp[0x1E] = "RET";
    vecCodOp[0x1F] = "STOP";
}

void escribeOperando(operando op, TMV mv){
    switch(op.tipo){
        case 0: {//memoria
            char nroReg = (op.valor >> 16) & 0x00000F;
            int offset = op.valor & 0x00FFFF;
            char tamaniocelda = (op.valor >> 22) & 0x03;
            switch (tamaniocelda){
                case 0:
                    printf("l");
                    break;
                case 2:
                    printf("w");
                    break;
                case 3:
                    printf("b");
                    break;
                }
            offset = offset << 16;
            offset = offset >> 16;
            printf("[%s + %d]", mv.registros[nroReg].nombre, offset);     
            }   
            break;
        case 1: //inmediato
            op.valor = op.valor << 16;
            op.valor = op.valor >> 16;
            printf("%d ", op.valor);  
            break;
        case 2:{ //registro
            char nroReg = op.valor & 0x0F; 
            char segReg = (op.valor >> 4) & 0x03;
            switch (segReg){
                case 0:
                    printf("%s", mv.registros[nroReg].nombre);
                    break;
                case 1: 
                    printf("%cL", mv.registros[nroReg].nombre[1]);
                    break;
                case 2:
                    printf("%cH", mv.registros[nroReg].nombre[1]);
                    break; 
                case 3:
                    printf("%cX", mv.registros[nroReg].nombre[1]);
                    break;
            }
        }         
    }
}

void disassembly(TMV mv){
    char instruccion, codOperacion;
    operando opA, opB;
    int tamanio, bandera, cantchar, aux;
    char *vecCodOp[0X20];
    inicializaVecCodOp(vecCodOp);
    int entryPoint = mv.registros[IP].valor & 0x0000FFFF;
    int valorOriginalIP = mv.registros[IP].valor;
    mv.registros[IP].valor = 0;
    int tamanioCodigo = mv.tabla_segmentos[mv.registros[CS].valor>>16].size;
    int dirFisicaIP;
    printf("\n");
    if (mv.registros[KS].valor != -1)
        while ((mv.registros[IP].valor) < mv.tabla_segmentos[mv.registros[KS].valor>>16].size){
            dirFisicaIP = calculaDirFisica(mv.registros[IP].valor, &mv);
            cantchar=0;
            bandera=0;
            printf(" [%04X] ", dirFisicaIP);
            aux=mv.registros[IP].valor;
            while ((mv.registros[IP].valor < mv.tabla_segmentos[mv.registros[KS].valor>>16].size) && (mv.memoria[mv.registros[IP].valor] != 0x00)){
                cantchar++;
                if (cantchar == 7){
                    bandera = 1;
                    printf(".. ");
                }
                if (!bandera)
                    printf("%X ", mv.memoria[mv.registros[IP].valor]);
                mv.registros[IP].valor++;
            }
            mv.registros[IP].valor++;
            if (!bandera)
                printf("00");
            tamanio = cantchar;
            for (tamanio; tamanio <= 6; tamanio++)
                printf("\t");
            printf(" | %c", 34);
            for (int i=0; i<cantchar; i++)
                if ((mv.memoria[aux+i]<32) | (mv.memoria[aux+i]>126))
                    printf(".");
                else
                    printf("%c", mv.memoria[aux+i]);
            printf("%c \n", 34);
        }
    mv.registros[IP].valor = mv.registros[CS].valor;
    while((mv.registros[IP].valor & 0x0000FFFF) < tamanioCodigo){
        dirFisicaIP = calculaDirFisica(mv.registros[IP].valor, &mv);
        if ((mv.registros[IP].valor  & 0x0000FFFF) == entryPoint)
            printf(">");
        else
            printf(" ");
        printf("[%04X] ", dirFisicaIP);
        tamanio = 0;
        instruccion = mv.memoria[dirFisicaIP];
        codOperacion = instruccion & 0x1F;
        mv.registros[IP].valor += 1;
        char nroOperandos = nroOperandosOperacion(instruccion);
        if (nroOperandos == 2){
            opB.tipo = (instruccion >> 6) & 0x03;
            opA.tipo = (instruccion >> 4) & 0x03;
            tamanio = (~(opA.tipo) & 0x03) + (~(opB.tipo) & 0x03);
        }else{
            if (nroOperandos == 1){
                opA.tipo = (instruccion >> 6) & 0x03;
                tamanio = (~(opA.tipo) & 0x03);
            }
        }
        int j = dirFisicaIP;
        for (j; j <= (dirFisicaIP + tamanio); j++)
            printf("%02X ", (mv.memoria[j] & 0x000000FF));

        for (tamanio; tamanio <= 8; tamanio++)
            printf("    ");
        
        printf("|    %s ", vecCodOp[codOperacion]);
        
        if (nroOperandos == 2) {
            opB.valor = leeOperando(&opB, mv);
            mv.registros[IP].valor += (~(opB.tipo) & 0x03);
            opA.valor = leeOperando(&opA, mv);
            mv.registros[IP].valor += (~(opA.tipo) & 0x03);
            escribeOperando(opA, mv);
            printf(", ");
            escribeOperando(opB, mv);
        } else {
            if (nroOperandos == 1){
                opA.valor = leeOperando(&opA, mv);
                mv.registros[IP].valor += (~(opA.tipo) & 0x03);
                escribeOperando(opA, mv);
            }
        }
        printf("\n");
    }
    mv.registros[IP].valor = valorOriginalIP;
    printf("\n");
}

int getop(operando opA, TMV mv){
    int valor = 0;
    switch (opA.tipo){
        case 0: { // memoria
            char codReg = (opA.valor >> 16) & 0x00000F;
            short int offset = opA.valor & 0x0000FFFF;
            int dirFisica = mv.tabla_segmentos[mv.registros[codReg].valor>>16].base + offset + (mv.registros[codReg].valor & 0x0000FFFF);
            //unsigned int dirFisica = calculaDirFisica(opA.valor, &mv);
            int tamaniocelda = 4 - ((opA.valor >> 22) & 0x0003);
            for (int i = 0; i < tamaniocelda; i++){
                valor = valor << 8;
                valor += mv.memoria[dirFisica + i] & 0x000000FF;
            }
            //extiende el signo
            valor = valor<<((4-tamaniocelda)*8); 
            valor = valor>>((4-tamaniocelda)*8);            
            break;
        }
        case 1: // inmediato
            valor = opA.valor;
            valor = valor << 16;
            valor = valor >> 16;
            break;

        case 2: { //registro 
            char sectorReg = (opA.valor & 0x30) >> 4;
            char codReg = opA.valor & 0x0F;
            switch (sectorReg){
                case 0: // registro de 4 bytes
                    valor = mv.registros[codReg].valor;
                    break;

                case 1: // 4to byte del registro
                    valor  = mv.registros[codReg].valor & 0x000000FF;
                    valor = valor << 24;
                    valor = valor >> 24;
                    break;

                case 2: // 3er byte del registro
                    valor  = mv.registros[codReg].valor & 0x0000FF00;
                    valor = valor << 16;
                    valor = valor >> 24;
                    break;

                case 3: //registro de 2 bytes
                    valor  = mv.registros[codReg].valor & 0x0000FFFF;
                    valor = valor << 16;
                    valor = valor >> 16;
                    break;
            }
            break;
        }   
    }
    return valor;
}

void setop(operando op, int valor, TMV* mv){
    switch(op.tipo){
        case 0: {
            char codReg = (op.valor >> 16) & 0x00000F;
            short int offset = op.valor & 0x0000FFFF;
            int dirFisica = mv->tabla_segmentos[mv->registros[codReg].valor>>16].base + offset + (mv->registros[codReg].valor & 0x0000FFFF);
            //int dirFisica = calculaDirFisica(op.valor, mv);
            int tamaniocelda = 4 - ((op.valor >> 22) & 0x0003);
            for(int i=dirFisica+tamaniocelda-1; i>=dirFisica; i--){
                mv->memoria[i] = valor & 0x000000FF;
                valor = (valor >> 8);
            }
        }
        break;
        case 2: { // registro
            char sectorReg = (op.valor & 0x30) >> 4;
            char codReg = op.valor & 0x0F;
            switch (sectorReg){
                case 0: // registro de 4 bytes
                    mv->registros[codReg].valor = valor;
                    break;
                case 1: // 4to byte del registro
                    mv->registros[codReg].valor = (mv->registros[codReg].valor & 0xFFFFFF00) | (valor & 0x000000FF);
                    break;               
                case 2: // 3er byte del registro
                    mv->registros[codReg].valor = (mv->registros[codReg].valor & 0xFFFF00FF) | ((valor << 8)& 0x0000FF00);
                    break;
                case 3: // registro de 2 bytes
                    mv->registros[codReg].valor = (mv->registros[codReg].valor & 0xFFFF0000) | (valor & 0x0000FFFF);
                    break;
            }
        }        
        break;
    }
}

int nroOperandosOperacion(char instruccion){
    if (((instruccion & 0x01F) > 0x1D) || (((instruccion & 0x01F) > 0x0C) && ((instruccion & 0x01F) < 0x10)))
        return 0;
    else
        if (((instruccion>>4) & 0x0001) == 0x001) 
            return 1;
        else 
            if (((instruccion>>4) & 0x0001) == 0x000) 
                return 2;       
}

void actualizaCC(int resultado, TMV* mv){
    if (resultado > 0)
        mv->registros[CC].valor = 0x00000000;
    else
        if (resultado < 0)
            mv->registros[CC].valor = 0x80000000;
        else
            mv->registros[CC].valor = 0x40000000;
}

void MOV (operando opA, operando opB, TMV* mv){
    int b = getop(opB, *mv);
    setop(opA, b, mv);
}

void ADD(operando opA, operando opB, TMV* mv){
    int a, b, resultado;
    a = getop(opA, *mv);
    b = getop(opB, *mv);
    resultado = a + b;
    setop(opA, resultado, mv);
    actualizaCC(resultado, mv);
}

void SUB(operando opA, operando opB, TMV* mv){
    int a, b, resultado;
    a = getop(opA, *mv);
    b = getop(opB, *mv);
    resultado = a - b;
    setop(opA, resultado, mv);
    actualizaCC(resultado, mv);
}

void MUL(operando opA, operando opB, TMV* mv){
    int a, b, resultado;
    a = getop(opA, *mv);
    b = getop(opB, *mv);
    resultado = a * b;
    setop(opA, resultado, mv);
    actualizaCC(resultado, mv);
}

void DIV(operando opA, operando opB, TMV* mv){
    int a, b, resultado;
    a = getop(opA, *mv);
    b = getop(opB, *mv);
    if (b == 0)
        encontroError(2, mv);
    resultado = a / b;
    mv -> registros[AC].valor = a % b;
    setop(opA, resultado, mv);
    actualizaCC(resultado, mv);
}

void AND(operando opA, operando opB, TMV* mv){
    int a, b, resultado;
    a = getop(opA, *mv);
    b = getop(opB, *mv);
    resultado = a & b;
    setop(opA, resultado, mv);
    actualizaCC(resultado, mv);
}

void OR(operando opA, operando opB, TMV* mv){
    int a, b, resultado;
    a = getop(opA, *mv);
    b = getop(opB, *mv);
    resultado = a | b;
    setop(opA, resultado, mv);
    actualizaCC(resultado, mv);
}

void XOR(operando opA, operando opB, TMV* mv){
    int a, b, resultado;
    a = getop(opA, *mv);
    b = getop(opB, *mv);
    resultado = a ^ b;
    setop(opA, resultado, mv);
    actualizaCC(resultado, mv);
}

void SWAP(operando opA, operando opB, TMV* mv){
    int a, b;
    a = getop(opA, *mv);
    b = getop(opB, *mv);
    setop(opA, b, mv);
    setop(opB, a, mv);
}

void CMP(operando opA, operando opB, TMV* mv){
    int a, b;
    a = getop(opA, *mv);
    b = getop(opB, *mv);
    actualizaCC(a-b, mv);
}

void SHL(operando opA, operando opB, TMV* mv){
    int a, b, resultado;
    a = getop(opA, *mv);
    b = getop(opB, *mv);
    resultado = a << b;
    setop(opA, resultado, mv);
    actualizaCC(resultado, mv);
}

void SHR(operando opA, operando opB, TMV* mv){
    int a, b, resultado;
    a = getop(opA, *mv);
    b = getop(opB, *mv);
    resultado = a >> b;
    setop(opA, resultado, mv);
    actualizaCC(resultado, mv);
}

void JMP(operando opA, operando opB, TMV* mv){
    int a = getop(opA, *mv);
    mv->registros[IP].valor = mv->registros[CS].valor | a;
}

void JZ(operando opA, operando opB, TMV* mv){
    int a = getop(opA, *mv);
    if(mv->registros[CC].valor == 0x40000000){
        mv->registros[IP].valor = mv->registros[CS].valor | a;
    }
}

void JP(operando opA, operando opB, TMV* mv){
    int a = getop(opA, *mv);
    if(mv->registros[CC].valor == 0x00000000){
        mv->registros[IP].valor = mv->registros[CS].valor | a;
    }
}

void JN(operando opA, operando opB, TMV* mv){
    int a = getop(opA, *mv);
    if(mv->registros[CC].valor == 0x80000000){
        mv->registros[IP].valor = mv->registros[CS].valor | a;
    }
}

void JNZ(operando opA, operando opB, TMV* mv){
    int a = getop(opA, *mv);
    if(mv->registros[CC].valor != 0x40000000){
        mv->registros[IP].valor = mv->registros[CS].valor | a;
    }
}

void JNP(operando opA, operando opB, TMV* mv){
    int a = getop(opA, *mv);
    if(mv->registros[CC].valor != 0x00000000){
        mv->registros[IP].valor = mv->registros[CS].valor | a;
    }
}

void JNN(operando opA, operando opB, TMV* mv){
    int a = getop(opA, *mv);
    if(mv->registros[CC].valor != 0x80000000){
        mv->registros[IP].valor = mv->registros[CS].valor | a;
    }
}

void NOT(operando opA, operando opB, TMV* mv){
    int a = ~getop(opA, *mv);
    setop(opA, a, mv);
    actualizaCC(a, mv);
}

void LDH(operando opA, operando opB, TMV* mv){
    int a = (getop(opA, *mv) << 16) & 0xFFFF0000;
    mv->registros[AC].valor = (mv->registros[AC].valor & 0x0000FFFF) | a;
}

void LDL(operando opA, operando opB, TMV* mv){
    int a = getop(opA, *mv) & 0x0000FFFF;
    mv->registros[AC].valor = (mv->registros[AC].valor & 0xFFFF0000) | a;
}

void STOP(operando opA, operando opB, TMV* mv){
    mv->registros[IP].valor = mv->registros[CS].valor | mv->tabla_segmentos[mv->registros[CS].valor>>16].size;
}
    
void PUSH(operando opA, operando opB, TMV* mv){
    int a;
    operando aux;
    int auxSS = (mv->registros[SS].valor >> 16);
    mv->registros[SP].valor-= 4;
    if ((mv->registros[SP].valor & 0x0000FFFF - 3) <= 0) 
       encontroError(5, mv);
    else {
        a = getop(opA, *mv);
        aux.tipo = 0; 
        aux.valor = 0x00030000 | (mv->registros[SP].valor & 0x0000FFFF);
        setop(aux, a, mv);
    }
}

void POP(operando opA, operando opB, TMV* mv){
    int a;
    operando aux;
    int auxSS = (mv->registros[SS].valor >> 16);
    if ((mv->registros[SP].valor & 0x0000FFFF) >= (mv->tabla_segmentos[auxSS].size)) // si el sp apunta al final del segmento
        encontroError(6, mv);
    else {
        aux.tipo = 0;
        aux.valor = 0x00030000 | (mv->registros[SP].valor & 0x0000FFFF);;
        a = getop(aux, *mv);
        setop(opA, a, mv);
        mv->registros[SP].valor += 4;
    }
}

void CALL(operando opA, operando opB, TMV* mv){
    operando aux;
    aux.valor = IP;
    aux.tipo = 2;
    PUSH(aux, opB, mv);
    JMP(opA, opB, mv);
}

void RET(operando opA, operando opB, TMV* mv){
    operando aux;
    aux.valor = IP;
    aux.tipo = 2;
    POP(aux, opB, mv);
}

void RND(operando opA, operando opB, TMV* mv){
    int b, aux;
    b = getop(opB,*mv);
    aux = rand() % b;
    setop(opA, aux, mv);
}

void SYS1(operando opA, operando opB, TMV* mv){ //READ
    int dirLogica = mv->registros[EDX].valor;
    int iTDS = (dirLogica >> 16) & 0x000F;
    short int offset = dirLogica & 0x00FFFF;
    int dirFisica = mv->tabla_segmentos[iTDS].base + offset;
    
    int k = dirFisica;
    int dato;

    int cantCeldas = mv->registros[ECX].valor & 0x000000FF;
    int tamanioCeldas =  (mv->registros[ECX].valor & 0x0000FF00)>>8;
    int modoLectura = mv->registros[EAX].valor & 0x000000FF;

    for (int i=0; i<cantCeldas; i++){
        printf("\n");
        printf("[%04X]:", dirFisica);
        switch (modoLectura){
            case 0x08: 
                scanf("%X", &dato);
                break;
            case 0x04: 
                scanf("%O", &dato);
                break;
            case 0x02: 
                scanf(" %c", &dato);
                break;
            case 0x01: 
                scanf("%d", &dato);
            break;
        }    
        printf("\n");
        int k = dirFisica;  
        for (int j=tamanioCeldas; j>0; j--){
            mv->memoria[k + j - 1] = dato & 0x000000FF;
            dato = dato >> 8;         
        }
        dirFisica += tamanioCeldas;
    }
}

void SYS2(operando opA, operando opB, TMV* mv){ // WRITE
    int dirFisica = calculaDirFisica(mv->registros[EDX].valor, mv);
    int k = dirFisica;
    int aux, h;

    int cantCeldas = mv->registros[ECX].valor & 0x000000FF;
    int tamanioCeldas =  (mv->registros[ECX].valor & 0x0000FF00)>>8;
    int modoEscritura = mv->registros[EAX].valor & 0x000000FF;
    printf("\n");
    for (int i=0; i<cantCeldas; i++){
        int dato = 0;
        for (int j = 0; j<tamanioCeldas; j++){
            aux = mv->memoria[dirFisica+tamanioCeldas-1-j];
            aux = aux & 0x000000FF;
            aux = aux << (8 * j);
            dato = aux | dato;
        }
        printf("[%04X]: ", dirFisica);
        if ((modoEscritura & 0x00000001) != 0){ //decimal
            printf(" #");
            printf("%d  ", dato);
        }
        if ((modoEscritura & 0x00000002) != 0){ //caracter
            h=0;
            while (h<tamanioCeldas){
                aux = (dato >> (8*(tamanioCeldas-1-h++))) & 0x000000FF;
                if ((aux<32) | (aux>126))
                    printf(". ");
                else
                    printf("%c", aux );
            }
        }
        if ((modoEscritura & 0x00000004) != 0){ //octal
            printf(" @");  
            printf("%o ", dato);
        }
        if ((modoEscritura & 0x00000008) != 0){ //hexadecimal
            printf(" %%");
            h=0;
            while (h<tamanioCeldas){
                printf("%02X", ((dato>>(8*(tamanioCeldas-1-h++))) & 0x000000FF));
            }
        }
        printf("\n");   
        dirFisica += tamanioCeldas;         
    }
}

void SYS3(operando opA, operando opB, TMV* mv){ // read string
    char* string;
    int iTDS = (mv->registros[EDX].valor >> 16) & 0x000F;
    int dirFisica = calculaDirFisica(mv -> registros[EDX].valor, mv);
    int cantChar = mv -> registros[ECX].valor & 0x0000FFFF;
    int i=0;
    if (cantChar == 0xFFFF){
        int tamaniomax = mv->tabla_segmentos[iTDS].base + mv->tabla_segmentos[iTDS].size;
        string = (char*)malloc(sizeof(char)*(tamaniomax-dirFisica));
    }
    else 
        string = (char*)malloc(sizeof(char)*cantChar);
    gets(string);
    while ((i < cantChar || cantChar == 0xFFFF) && (string[i] != 0x00)){
        mv->memoria[dirFisica + i] = string[i++];
    }
    mv->memoria[dirFisica + i] = 0x00;
    free(string);
}

void SYS4(operando opA, operando opB, TMV* mv){ // write string
    int dirFisica = calculaDirFisica(mv -> registros[EDX].valor, mv);
    int i = 0;
    while (mv ->memoria[dirFisica + i] != 0x00){
        printf("%c", mv->memoria[dirFisica + i++]);
    }
    printf("\n");
}

void SYS7(operando opA, operando opB, TMV* mv){  // clear
    system("cls"); //funciona solo para Windows
}

void generaVMI(TMV* mv){
    FILE *arch = fopen(mv->nombreVMI, "wb");
    if (arch != NULL){
        char memoria[2], aux;
        char version = 1;
        fwrite("VMI24", sizeof(char), 5, arch);
        fwrite(&version, sizeof(char), 1, arch);
        memoria[0] = (mv->memoriaTotal/1024) >> 8;
        memoria[1] = mv->memoriaTotal/1024;
        fwrite(memoria, 2*sizeof(char), 1, arch);
        for (int i= 0; i<16; i++){
            for (int j=3; j>=0; j--){
                aux = (mv->registros[i].valor >> (8*j)) & 0x000000FF;
                fwrite(&aux, sizeof(char), 1, arch);
            }
        }
        for (int i=0; i<8; i++){
            for (int j=1; j>=0; j--){
                aux = (mv->tabla_segmentos[i].base >> (8*j)) & 0x000000FF;
                fwrite(&aux, sizeof(char), 1, arch);
            }
            for (int j=1; j>=0; j--){
                aux = (mv->tabla_segmentos[i].size >> (8*j)) & 0x000000FF;
                fwrite(&aux, sizeof(char), 1, arch);
            }        
        }
        fwrite(mv->memoria, sizeof(char), mv->memoriaTotal, arch);
        //fclose(arch);
    }
}

void SYSF(operando opA, operando opB, TMV* mv){
    char comando;
    int tamanioCodigo = mv->tabla_segmentos[mv->registros[CS].valor>>16].base + mv->tabla_segmentos[mv->registros[CS].valor>>16].size;
    if (mv->nombreVMI != NULL){
        generaVMI(mv);
        scanf("%c", &comando);
        while ((comando!= 'g') && (comando != 'q') &&  (mv->registros[IP].valor < (tamanioCodigo))){
            if (comando == 10){
                ejecutaUnaInstruccion(mv);
                generaVMI(mv);    
            }
            scanf("%c", &comando);
        }
        if (comando == 'q')
            mv->registros[IP].valor = tamanioCodigo;
    }
}

void SYSNULL(operando opA, operando opB, TMV* mv){
}

void (*sys[16])(operando, operando, TMV*) = {SYSNULL, SYS1, SYS2, SYS3, SYS4, SYSNULL, SYSNULL, SYS7, SYSNULL, SYSNULL, SYSNULL, SYSNULL, SYSNULL, SYSNULL, SYSNULL, SYSF};

void SYS(operando opA, operando opB, TMV* mv){
    sys[opA.valor](opA, opB, mv);
}

void inicializarOperaciones(punteroFuncion operacion[]){
    operacion[0x000] = &MOV;
    operacion[0x001] = &ADD;
    operacion[0x002] = &SUB;
    operacion[0x003] = &SWAP;
    operacion[0x004] = &MUL;
    operacion[0x005] = &DIV;
    operacion[0x006] = &CMP;
    operacion[0x007] = &SHL;
    operacion[0x008] = &SHR;
    operacion[0x009] = &AND;
    operacion[0x00A] = &OR;
    operacion[0x00B] = &XOR;
    operacion[0x00C] = &RND;

    operacion[0x010] = &SYS;
    operacion[0x011] = &JMP;
    operacion[0x012] = &JZ;
    operacion[0x013] = &JP;
    operacion[0x014] = &JN;
    operacion[0x015] = &JNZ;
    operacion[0x016] = &JNP;
    operacion[0x017] = &JNN;
    operacion[0x018] = &LDL;
    operacion[0x019] = &LDH;
    operacion[0x01A] = &NOT;
    operacion[0x01B] = &PUSH;
    operacion[0x01C] = &POP;
    operacion[0x01D] = &CALL;
    operacion[0x01E] = &RET;
    operacion[0x01F] = &STOP;
}

void ejecutaUnaInstruccion(TMV* mv){
    char instruccion, codOperacion;
    operando opA, opB;
    punteroFuncion operacion[0x020];
    inicializarOperaciones(operacion);
    int dirFisicaIP;
    int tamanioCS = mv->tabla_segmentos[mv->registros[CS].valor>>16].size;
    if (mv->registros[IP].valor < tamanioCS) {
        dirFisicaIP = calculaDirFisica(mv->registros[IP].valor, mv);   
        instruccion = mv->memoria[dirFisicaIP];
        mv->registros[IP].valor += 1;
        opB.tipo = (instruccion >> 6) & 0x03;
        opA.tipo = (instruccion >> 4) & 0x03;      
        if (nroOperandosOperacion(instruccion) == 1){
            opA.tipo = opB.tipo;
            opB.tipo = 3;
        }
        codOperacion = instruccion & 0x1F;
        chequeaCodOpValido(codOperacion, *mv);
        opB.valor = leeOperando(&opB, *mv);
        mv->registros[IP].valor += (~(opB.tipo) & 0x03);
        opA.valor = leeOperando(&opA, *mv);
        mv->registros[IP].valor += (~(opA.tipo) & 0x03);
        operacion[codOperacion](opA, opB, mv); 
    }
}

void chequeaCodOpValido(char codOperacion, TMV mv){
    if ((codOperacion < 0x00) || ((codOperacion > 0x0C) && (codOperacion < 0x10)) || (codOperacion > 0x1F))
        encontroError(1, &mv);
}

void ejecuta(TMV* mv){
    char instruccion, codOperacion;
    operando opA, opB;
    punteroFuncion operacion[0x020];
    inicializarOperaciones(operacion);
    int dirFisicaIP;
    int tamanioCS = mv->tabla_segmentos[mv->registros[CS].valor>>16].size;
    while ((mv->registros[IP].valor & 0x0000FFFF) < tamanioCS){  
        dirFisicaIP = calculaDirFisica(mv->registros[IP].valor, mv);   
        instruccion = mv->memoria[dirFisicaIP];
        mv->registros[IP].valor += 1;
        opB.tipo = (instruccion >> 6) & 0x03;
        opA.tipo = (instruccion >> 4) & 0x03;
        
        if (nroOperandosOperacion(instruccion) == 1){
            opA.tipo = opB.tipo;
            opB.tipo = 3;
        }
        codOperacion = instruccion & 0x1F;
        chequeaCodOpValido(codOperacion, *mv);
        opB.valor = leeOperando(&opB, *mv);
        mv->registros[IP].valor += (~(opB.tipo) & 0x03);
        opA.valor = leeOperando(&opA, *mv);
        mv->registros[IP].valor += (~(opA.tipo) & 0x03);

        operacion[codOperacion](opA, opB, mv); 
    }
}
