/***********************************************************************
EP03 - Simulador de memoria - Header

Disciplina: MAC0422 - Sistemas Operacionais
Professor: Daniel Batista
Aluno: Francisco Membrive

Data: 10.06.2025

Header do programa ep3.c, necessario para que a compilacao ocorra 
corretamente.

***********************************************************************/

#ifndef EP3_H
#define EP3_H

#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>

#define FIRSTFIT 1
#define NEXTFIT 2
#define BESTFIT 3
#define WORSTFIT 4

#define MAX_ALOCATION 65536
#define MAX_LINES 2001

/****************************************************************

Parametros do programa:

./ep3 <algoritmo> <pgm de entrada> <txt de trace> <pgm de saida>

****************************************************************/

/*
int algoritmo

1 - First Fit
2 - Next Fit
3 - Best Fit
4 - Worst Fit
*/
extern int algoritmo;

/*
char *pgm_entrada: nome do arquivo de entrada
*/
extern char *pgm_entrada;

/*
char *trace: nome do arquivo de trace
*/
extern char *trace;

/*
char *pgm_saida: nome do arquivo de saida
*/
extern char *pgm_saida;

/*
indice da ultima posicao acessada, para ser utilizado 
pelo algoritmo next fit
*/
extern int last_position;

/*
guarda as falhas, com o numero da linha nas posicoes pares
e o valor requisitado nas posicoes impares
*/
extern int falhas[2*MAX_LINES];

/*
proxima posicao a ser preenchida no vetor de falhas
*/
extern int falha_count;

/*
deslocamento para pular as linhas iniciais
dos arquivos pgm
*/
extern int desloc;

/*
verifica se o algoritmo eh first fit
ou next fit e retorna true se for
*/
bool dois_primeiros();

/*
define se o caractere a ser escrito depois daquela
posicao de memoria eh um espaco ou uma quebra de linha
*/
char caractere_final(int idx);

/*
le a posicao idx de memoria do arquivo f e escreve em 
buffer (idx vai de 0 a 65535)
*/
void le_posicao(FILE *f, int idx, char buffer[4]);

/*
altera a posicao idx do arquivo f para valor
no geral, so eh utilizada para alterar de 
"255" (livre) para "  0" (ocupado)

ou para trocar duas posicoes durante a compactacao
*/
void altera(FILE *f, int idx, char valor[3]);

/*
troca o valor de memoria das posicoes indice i e j
*/
void unit_swap(FILE *f, int i, int j);

/*
aloca o valor de memoria m conforme o algoritmo definido

se nao houver m posicoes consecutivas livres, adiciona ao
vetor de falhas
*/
bool aloca(int linha, int m, FILE *saida_file);

/*
compacta a memoria usando two pointers, concentrando
posicoes ocupadas no final
*/
void compacta(FILE *saida_file);

/*
laco principal, abre o arquivo de saida e de trace
e roda o algoritmo solicitado
*/
void principal();

/*
main: copia entrada para saida e chama a funcao principal
*/
int main(int argc, char *argv[]);


#endif
