/***********************************************************************
EP01 - Simulador de Processsos

Disciplina: MAC0422 - Sistemas Operacionais
Professor: Daniel Batista
Aluno: Francisco Membrive

Data: 22.04.2025

uso: 
    make ep1 && ./ep1 <escalonador> <trace.txt> <saida.txt>
    <escalonador> - 1 pra FCFS, 2 pra SRTN, 3 pra PRIORITY

***********************************************************************/

#ifndef EP1_H
#define EP1_H

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>

#define MAX_NAME 33
#define MAX_PROCESSES 51
#define TIME_QUANTUM 1
#define FCFS 1
#define SRTN 2
#define PRIORITY 3

/*
Struct para armazenar os dados do processo

Dados de entrada:
    char name[MAX_NAME]: nome
    int t0: tempo de chegada
    int dt: tempo necessário para executar
    int deadline: tempo limite para terminar e cumprir o prazo

Informações e flags:
    int restante: tempo que falta para concluir, inicia valendo dt
    int tf: tempo em que foi concluído
    int concluido: 1 se já foi concluído, 0 senão
    int escalonado: 1 se está rodando, 0 senão

Para operação com threads:
    pthread_mutex_t mutex: para proteger o acesso ao processo
    pthread_cond_t cond: para sinalizar quando o processo está pronto

Para o escalonamento com prioridade:
    int qcedido: quantums recebidos pelo processo naquela rodada
    int qusado: quantums usados pelo processo naquela rodada

*/
typedef struct{
    char name[MAX_NAME];
    int t0;
    int dt;
    int deadline;
    int restante;
    int tf;
    int concluido;
    int escalonado;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    int qcedido;
    int qusado;
} Process;

extern int escalonador;                        /* 1 - FCFS || 2 - SRNT || 3 - com Prioridade */
extern int qtd_process;
extern Process processos[MAX_PROCESSES];
extern pthread_t proc_threads[MAX_PROCESSES];

extern int cores;                              /* Número de núcleos de processamento */

extern int fila_prontos[MAX_PROCESSES];        /* Fila de prontos por índice */
extern int prontos;                            /* Número de prontos */
extern int preempcoes;                         /* Número de preempções */


/* 
Mutexes para proteger esses valores: 

Um protege a fila e a quantidade de prontos
e o outro a quantidade de preempções
*/
extern pthread_mutex_t prontos_mutex;
extern pthread_mutex_t preempcoes_mutex;

/* 
Vetores para armazenar qual CPU está rodando um processo
e qual processo está rodando em cada CPU

Limite de CPUs: 128 (Considerando que o máximo dos testes
é 50 processos, não deve restringir nada)
*/
extern int cpu_do_processo[MAX_PROCESSES];
extern int processo_na_cpu[128];

/*
Contador de tempo, protegido pelo mutex abaixo
*/
extern int tempo_atual;
extern pthread_mutex_t tempo_mutex;

/* prototipos */
void adiciona_pronto(int index);
void remove_pronto(int index);
void adiciona_pronto_locked(int index);
void remove_pronto_locked(int index);
int proximo_processo(void);
int get_time(void);
void *t_escalonador(void *arg);
void *t_processos(void *arg);

#endif 
