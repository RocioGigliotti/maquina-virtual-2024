#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TAM_MEMORIA 16384
#define salto 3

#define CS 0                                        
#define DS 1
#define IP 5
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
    char memoria[TAM_MEMORIA];
    registro registros[16];
    tds tabla_segmentos[8];
} TMV;

typedef struct {
    int valor;
    char tipo;
} operando;

typedef void (*punteroFuncion)(operando, operando, TMV*);

void inicializaReg(TMV* mv);
void guardaCodigo(TMV* mv, const char *nombreArch);
int nroOperandosOperacion(char instruccion);
void disassembly(TMV mv);
void ejecuta(TMV* mv);

int getop(operando opA, TMV mv);
void setop(operando op, int valor, TMV* mv);

void MOV(operando opA, operando opB, TMV* mv);
void ADD(operando opA, operando opB, TMV* mv);

int main(int argc, char const *argv[]){
    TMV mv;
    const char *nombreArch;
    if (argc < 2){
        printf("Uso del programa = ./vmx.exe filename.vmx [-d]\n");
        printf("El campo filename es obligatorio\n");
    }
    else {
        nombreArch = argv[1];
        inicializaReg(&mv);
        guardaCodigo(&mv, nombreArch);
        if ((argc == 3) && strcmp(argv[2], "-d") == 0)
                disassembly(mv);
        ejecuta(&mv);
    }
    return 0;
}

void inicializaReg(TMV* mv){ 
    strcpy(mv->registros[DS].nombre, "DS");
    strcpy(mv->registros[CS].nombre, "CS");
    strcpy(mv->registros[IP].nombre, "IP");
    strcpy(mv->registros[CC].nombre, "CC");
    strcpy(mv->registros[AC].nombre, "AC");
    strcpy(mv->registros[EAX].nombre, "EAX");
    strcpy(mv->registros[EBX].nombre, "EBX");
    strcpy(mv->registros[ECX].nombre, "ECX");
    strcpy(mv->registros[EDX].nombre, "EDX");
    strcpy(mv->registros[EEX].nombre, "EEX");
    strcpy(mv->registros[EFX].nombre, "EFX");
    mv->registros[DS].valor = 0x00010000;
    mv->registros[CS].valor = 0;
    mv->registros[IP].valor = 0;
    mv->registros[CC].valor = 0;
    mv->registros[AC].valor = 0;
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
            printf("La instruccion es invalida");
            mv->registros[IP].valor =  mv->tabla_segmentos[CS].size;
            break;
        case 2: // division por cero
            printf("Se intento dividir por cero");
            mv->registros[IP].valor =  mv->tabla_segmentos[CS].size;
            break;
        case 3: //fallo de segmento
            printf("SegFault");
            mv->registros[IP].valor =  mv->tabla_segmentos[CS].size;
            break;
    
    }
}

void guardaCodigo(TMV* mv, const char *nombreArch){
    char codEjecucion[6];
    char version;
    int i = 0;
    char instruccion, aux;
    unsigned short int tamanio;
    FILE *arch = fopen(nombreArch, "rb");
    if (arch == NULL){
        printf("No existe el archivo");
    }
    else{
        fread (codEjecucion, sizeof(char), 5, arch);
        codEjecucion[5] = '\0';
        fread(&version, sizeof(char), 1, arch);
        if ((strcmp (codEjecucion, "VMX24") != 0) || (version != 1)){
            printf("El archivo no es un archivo ejecutable por esta maquina virtual");
            fclose(arch);
        }
        else{
            printf("El archivo es valido, leyendo...");
            fread(&aux, sizeof(char), 1, arch);
            tamanio = aux;
            tamanio = tamanio << 8;
            fread(&aux, sizeof(char), 1, arch);
            tamanio = tamanio | (aux & 0x00FF);
    
            mv->tabla_segmentos[CS].base = 0;
            mv->tabla_segmentos[CS].size = tamanio;
            mv->tabla_segmentos[DS].base = tamanio;
            mv->tabla_segmentos[DS].size = TAM_MEMORIA-tamanio;
            mv->registros[IP].valor = mv->tabla_segmentos[CS].base;
            
            fread(mv->memoria, sizeof(char), tamanio, arch);
        
            printf("El codigo ya fue guardado \n"); 
            //printf("El codigo es: \n"); 
            //for ( int i = 0; i<tamanio; i++){
            //    printf("%02X  ", mv->memoria[i] & 0x000000FF);
            //}
            fclose(arch); 
        }
    }
}

int leeOperando(operando* op, TMV mv){
    int valor = 0;
    switch (op->tipo) { 
        case 0: // memoria
            valor = mv.memoria[mv.registros[IP].valor] & 0x00FF;
            valor = valor << 8;
            valor = valor | (mv.memoria[mv.registros[IP].valor + 1] & 0x00FF);
            valor = valor << 8;
            valor = valor | (mv.memoria[mv.registros[IP].valor + 2] & 0x00FF);
            break;

        case 1: // inmediato
            valor = mv.memoria[mv.registros[IP].valor] & 0x00FF;
            valor = valor << 8;
            valor = valor | (mv.memoria[mv.registros[IP].valor + 1] & 0x00FF);
            break;

        case 2: // registro

            valor = mv.memoria[mv.registros[IP].valor] & 0x00FF;
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
    vecCodOp[0x1F] = "STOP";
}

void escribeOperando(operando op, TMV mv){
    switch(op.tipo){ //memoria
        case 0: {
            char nroReg = (op.valor >> 16) & 0x00000F;
            int offset = op.valor & 0x00FFFF;
            printf("[%s + %d]", mv.registros[nroReg].nombre, offset);
            
        }
        break;
        case 1: //inmediato
            op.valor = op.valor << 16;
            op.valor = op.valor >> 16;
            printf("%d ", op.valor);  
            break;
        case 2: { //registro
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
        break;  
    }
}

void disassembly(TMV mv){
    char instruccion, codOperacion;
    operando opA, opB;
    int tamanio;
    char *vecCodOp[0X20];
    inicializaVecCodOp(vecCodOp);
    mv.registros[IP].valor = 0;
    printf("\n");
    while(mv.registros[IP].valor < mv.tabla_segmentos[CS].size){
        printf("[%04X] ", mv.registros[IP].valor & 0x0000FFFF);
        tamanio = 0;
        instruccion = mv.memoria[mv.registros[IP].valor];
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

        int j = mv.registros[IP].valor - 1 ;
        for (j; j < (mv.registros[IP].valor + tamanio); j++)
            printf("%02X\t", mv.memoria[j] & 0x000000FF);

        for (tamanio; tamanio <= 5; tamanio++)
            printf("\t");
        
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
    mv.registros[IP].valor = mv.tabla_segmentos[CS].base;
}
int getop(operando opA, TMV mv){
    int valor = 0;
    switch (opA.tipo){
        case 0: {// memoria
            int dirLogica = opA.valor;
            char codReg = (dirLogica >> 16) & 0x00000F;
            short int offset = dirLogica & 0x00FFFF;
            int dirFisica = mv.tabla_segmentos[(mv.registros[codReg].valor>> 16) & 0x00000F].base + ((mv.registros[codReg].valor) & 0x0000FFFF) + offset;
            for (int i = 0; i < 4; i++){ // lee 4 bytes de memoria a partir de la direccion fisica
                valor = valor << 8;
                valor += mv.memoria[dirFisica + i] & 0x000000FF;
            }
            break;
        }
        case 1: // inmediato
            valor = opA.valor;
            valor = valor << 16;
            valor = valor >> 16;
            break;

        case 2: {//registro 
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
            int dirLogica = op.valor;
            char codReg = (dirLogica >> 16) & 0x00000F;
            short int offset = dirLogica & 0x00FFFF;
            int dirFisica = mv->tabla_segmentos[(mv->registros[codReg].valor>> 16) & 0x00000F].base + ((mv->registros[codReg].valor) & 0x0000FFFF) + offset; 
            if ((dirFisica + salto) > (mv->tabla_segmentos[(mv->registros[codReg].valor>> 16) & 0x00000F].base + mv->tabla_segmentos[(mv->registros[codReg].valor>> 16) & 0x00000F].size))
                encontroError(3, mv);
            if (dirFisica < mv->tabla_segmentos[(mv->registros[codReg].valor>> 16) & 0x00000F].base)
                encontroError(3, mv);

            mv->memoria[dirFisica] = (valor>>24) & 0x000000FF;
            mv->memoria[dirFisica + 1] = (valor>>16) & 0x000000FF;
            mv->memoria[dirFisica + 2] = (valor>>8) & 0x000000FF;
            mv->memoria[dirFisica + 3] = valor & 0x000000FF;   
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
    if (((instruccion & 0x01F) > 0x1A) || (((instruccion & 0x01F) > 0x0C) && ((instruccion & 0x01F) < 0x10)))
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
    //printf("MOViendo...");
    int b = getop(opB, *mv);
    setop(opA, b, mv);
}

void ADD(operando opA, operando opB, TMV* mv){
    //printf("ADDeando numeros...");
    int a, b, resultado;
    a = getop(opA, *mv);
    b = getop(opB, *mv);
    resultado = a + b;
    setop(opA, resultado, mv);
    actualizaCC(resultado, mv);
}

void SUB(operando opA, operando opB, TMV* mv){
    //printf("SUBeando numeros...");
    int a, b, resultado;
    a = getop(opA, *mv);
    b = getop(opB, *mv);
    resultado = a - b;
    setop(opA, resultado, mv);
    actualizaCC(resultado, mv);
}

void MUL(operando opA, operando opB, TMV* mv){
    //printf("MULeando numeros...");
    int a, b, resultado;
    a = getop(opA, *mv);
    b = getop(opB, *mv);
    resultado = a * b;
    setop(opA, resultado, mv);
    actualizaCC(resultado, mv);
}

void DIV(operando opA, operando opB, TMV* mv){
    //printf("DIVeando numeros...");
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
    //printf("ANDeando numeros...");
    int a, b, resultado;
    a = getop(opA, *mv);
    b = getop(opB, *mv);
    resultado = a & b;
    setop(opA, resultado, mv);
    actualizaCC(resultado, mv);
}

void OR(operando opA, operando opB, TMV* mv){
    //printf("OReando numeros...");
    int a, b, resultado;
    a = getop(opA, *mv);
    b = getop(opB, *mv);
    resultado = a | b;
    setop(opA, resultado, mv);
    actualizaCC(resultado, mv);
}

void XOR(operando opA, operando opB, TMV* mv){
    //printf("XOReando numeros...");
    int a, b, resultado;
    a = getop(opA, *mv);
    b = getop(opB, *mv);
    resultado = a ^ b;
    setop(opA, resultado, mv);
    actualizaCC(resultado, mv);
}

void SWAP(operando opA, operando opB, TMV* mv){
    //printf("SWAPeando numeros...");
    int a, b;
    a = getop(opA, *mv);
    b = getop(opB, *mv);
    setop(opA, b, mv);
    setop(opB, a, mv);
}

void CMP(operando opA, operando opB, TMV* mv){
    //printf("CMPeando numeros...");
    int a, b;
    a = getop(opA, *mv);
    b = getop(opB, *mv);
    actualizaCC(a-b, mv);
}

void SHL(operando opA, operando opB, TMV* mv){
    //printf("SHLeando numeros...");
    int a, b, resultado;
    a = getop(opA, *mv);
    b = getop(opB, *mv);
    resultado = a << b;
    setop(opA, resultado, mv);

    actualizaCC(resultado, mv);
}

void SHR(operando opA, operando opB, TMV* mv){
    //printf("SHReando numeros...");
    int a, b, resultado;
    a = getop(opA, *mv);
    b = getop(opB, *mv);
    resultado = a >> b;
    setop(opA, resultado, mv);
    actualizaCC(resultado, mv);
}

void JMP(operando opA, operando opB, TMV* mv){
    //printf("JMPeando hasta...");
    int a = getop(opA, *mv);
    //printf("%%");
    //printf("%X\n ", a);
    mv->registros[IP].valor = a;
}

void JZ(operando opA, operando opB, TMV* mv){
    int a = getop(opA, *mv);
    if(mv->registros[CC].valor == 0x40000000){
        //printf("JZeando hasta...");
        //printf("%%");
        //printf("%X\n ", a);
        mv->registros[IP].valor = a;
    }
}

void JP(operando opA, operando opB, TMV* mv){  
    int a = getop(opA, *mv);
    if(mv->registros[CC].valor == 0x00000000){
        //printf("JPeando hasta...");
        //printf("%%");
        //printf("%X\n ", a);
        mv->registros[IP].valor = a;
    }
}

void JN(operando opA, operando opB, TMV* mv){    
    int a = getop(opA, *mv);
    if(mv->registros[CC].valor == 0x80000000){
        //printf("JNeando hasta...");
        //printf("%%");
        //printf("%X\n ", a);
        mv->registros[IP].valor = a;
    }
}

void JNZ(operando opA, operando opB, TMV* mv){
    int a = getop(opA, *mv);
    if(mv->registros[CC].valor != 0x40000000){
        //printf("JNZeando hasta...");
        //printf("%%");
        //printf("%X\n ", a);
        mv->registros[IP].valor = a;
    }
}

void JNP(operando opA, operando opB, TMV* mv){   
    int a = getop(opA, *mv);
    if(mv->registros[CC].valor != 0x00000000){
        //printf("JNPeando hasta...");
        //printf("%%");
        //printf("%X\n ", a);
        mv->registros[IP].valor = a;
    }
}

void JNN(operando opA, operando opB, TMV* mv){
    int a = getop(opA, *mv);
    if(mv->registros[CC].valor != 0x80000000){   
        //printf("JNNeando hasta...");
        //printf("%%");
        //printf("%X\n ", a);
        mv->registros[IP].valor = a;
    }
}

void NOT(operando opA, operando opB, TMV* mv){
    //printf("NOTeando numeros...");
    int a = ~getop(opA, *mv);
    setop(opA, a, mv);
    actualizaCC(a, mv);
}

void LDH(operando opA, operando opB, TMV* mv){
    //printf("LDHeando numeros...");
    int a = (getop(opA, *mv) << 16) & 0xFFFF0000;
    mv->registros[AC].valor = (mv->registros[AC].valor & 0x0000FFFF) | a;
}

void LDL(operando opA, operando opB, TMV* mv){
    //printf("LDLeando numeros...");
    int a = getop(opA, *mv) & 0x0000FFFF;
    mv->registros[AC].valor = (mv->registros[AC].valor & 0xFFFF0000) | a;
}

void STOP(operando opA, operando opB, TMV* mv){
    //printf("STOPeando numeros...\n");
    mv->registros[IP].valor = mv->tabla_segmentos[CS].size;
}

void RND(operando opA, operando opB, TMV* mv){
    //printf("RNDeando numeros...");
    int b, aux;
    b = getop(opB,*mv);
    aux = rand() % b;
    setop(opA, aux, mv);
}

void SYS1(operando opA, operando opB, TMV* mv){ //READ
    //printf("SYS1eando numeros...");
    int dirLogica = mv -> registros[EDX].valor;
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
        for (int j=tamanioCeldas; j > 0; j--){
            mv -> memoria[k + j - 1] = dato & 0x000000FF;
            dato = dato >> 8;         
        }
        dirFisica += tamanioCeldas;
    }

}

void SYS2(operando opA, operando opB, TMV* mv){ // WRITE
    //printf("SYS2eando numeros...");
    int dirLogica = mv -> registros[EDX].valor;
    int iTDS = (dirLogica >> 16) & 0x000F;
    short int offset = dirLogica & 0x00FFFF;
    int dirFisica = mv->tabla_segmentos[iTDS].base + offset;
    int k = dirFisica;
    int aux, h;

    int cantCeldas = mv->registros[ECX].valor & 0x000000FF;
    int tamanioCeldas =  (mv->registros[ECX].valor & 0x0000FF00)>>8;
    int modoEscritura = mv->registros[EAX].valor & 0x000000FF;
    printf("\n");
    for (int i=0; i<cantCeldas; i++){
        int dato = 0;
        for (int j = 0; j < tamanioCeldas; j++){
            aux = mv->memoria[dirFisica + tamanioCeldas - 1 - j];
            aux = aux & 0x000000FF;
            aux = aux << (8 * j);
            dato = aux | dato;
        }

        printf("[%04X]: ", dirFisica);
        if ((modoEscritura & 0x00000001) != 0){ //decimal
            printf(" ");
            printf("%d  ", dato);
        }
        if ((modoEscritura & 0x00000002) != 0){ //caracter
            h=0;
            while (h<tamanioCeldas){
                aux = (dato >> (8*(tamanioCeldas-1-h++))) & 0x000000FF;
                if ((aux<33) | (aux>126))
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

void (*sys[3])(operando, operando, TMV*) = {SYS1, SYS1, SYS2};

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

    operacion[0x01F] = &STOP;
}

void chequeaCodOpValido(char codOperacion, TMV mv){
    if ((codOperacion < 0x00) || ((codOperacion > 0x0C) && (codOperacion < 0x10)) || (((codOperacion > 0x1A) && (codOperacion != 0x1F))))
        encontroError(1, &mv);
}

void ejecuta(TMV* mv){
    char instruccion, codOperacion;
    operando opA, opB;
    punteroFuncion operacion[0x020];
    inicializarOperaciones(operacion);

    while(mv->registros[IP].valor < mv->tabla_segmentos[CS].size){
        instruccion = mv->memoria[mv->registros[IP].valor];
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

