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

#include "ep1.h"

int escalonador;                        /* 1 - FCFS || 2 - SRNT || 3 - com Prioridade */
int qtd_process = 0;
Process processos[MAX_PROCESSES];
pthread_t proc_threads[MAX_PROCESSES];

int cores = 0;                          /* Número de núcleos de processamento */

int fila_prontos[MAX_PROCESSES];        /* Fila de prontos por índice */
int prontos = 0;                        /* Número de prontos */
int preempcoes = 0;                     /* Número de preempções */


/* 
Mutexes para proteger esses valores: 

Um protege a fila e a quantidade de prontos
e o outro a quantidade de preempções
*/
pthread_mutex_t prontos_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t preempcoes_mutex = PTHREAD_MUTEX_INITIALIZER;

/* 
Vetores para armazenar qual CPU está rodando um processo
e qual processo está rodando em cada CPU

Limite de CPUs: 128 (Considerando que o máximo dos testes
é 50 processos, não deve restringir nada)
*/
int cpu_do_processo[MAX_PROCESSES];
int processo_na_cpu[128];

/*
Contador de tempo, protegido pelo mutex abaixo
*/
int tempo_atual = -1;
pthread_mutex_t tempo_mutex = PTHREAD_MUTEX_INITIALIZER;

/*
Funções para adicionar e remover um processo da fila de prontos:

Insere ordenado conforme o escalonador
*/
void adiciona_pronto(int index){
    pthread_mutex_lock(&prontos_mutex);
    if(escalonador == FCFS){
        int k = prontos;
        while(k > 0 && processos[fila_prontos[k-1]].t0 > processos[index].t0){
            fila_prontos[k] = fila_prontos[k-1];
            k--;
        }
        fila_prontos[k] = index;
        prontos++;
    }
    else if(escalonador == SRTN){
        int k = prontos;
        while(k > 0 && processos[fila_prontos[k-1]].dt > processos[index].dt){
            fila_prontos[k] = fila_prontos[k-1];
            k--;
        }
        fila_prontos[k] = index;
        prontos++;
    }
    else{
        fila_prontos[prontos++] = index;
    }
    pthread_mutex_unlock(&prontos_mutex);
}

void remove_pronto(int index){
    pthread_mutex_lock(&prontos_mutex);
    for(int i = index; i < prontos-1; i++){
        fila_prontos[i] = fila_prontos[i+1];
    }
    prontos--;
    pthread_mutex_unlock(&prontos_mutex);
}

/*
Cópias das funções, mas chamadas que assumem que o mutex já está travado 

Usar as anteriores com o mutex travado causa deadlock
*/
void adiciona_pronto_locked(int index){
    if(escalonador == FCFS){
        int k = prontos;
        while(k > 0 && processos[fila_prontos[k-1]].t0 > processos[index].t0){
            fila_prontos[k] = fila_prontos[k-1];
            k--;
        }
        fila_prontos[k] = index;
        prontos++;
    }
    else if(escalonador == SRTN){
        int k = prontos;
        while(k > 0 && processos[fila_prontos[k-1]].dt > processos[index].dt){
            fila_prontos[k] = fila_prontos[k-1];
            k--;
        }
        fila_prontos[k] = index;
        prontos++;
    }
    else{
        fila_prontos[prontos++] = index;
    }
}

void remove_pronto_locked(int index){
    for(int i = index; i < prontos-1; i++){
        fila_prontos[i] = fila_prontos[i+1];
    }
    prontos--;
}

/*
Função para selecionar o próximo processo da fila

Como as filas estão ordenadas, simplesmente retorna o primeiro
*/
int proximo_processo(){
    int index = -1;

    pthread_mutex_lock(&prontos_mutex);

    if(prontos == 0){
        pthread_mutex_unlock(&prontos_mutex);
        return -1;
    }

    index = fila_prontos[0];
    remove_pronto_locked(0);
    pthread_mutex_unlock(&prontos_mutex);
    return index;
}

/*Devolve o tempo, acesso protegido pelo mutex*/
int get_time(){
    int tempo;
    pthread_mutex_lock(&tempo_mutex);
    tempo = tempo_atual;
    pthread_mutex_unlock(&tempo_mutex);
    return tempo;
}


/*  
    Thread responsável por executar o escalonador

    Fica num loop até que todos os processos estejam 
    marcados como concluidos. 

    No inicio do loop, verifica se o numero de processos
    escalonados é menor que o número de núcleos disponíveis.
    Se for, escalona os próximos de acordo com a fila de 
    prontos.

    Por fim, se o escalonador é preemptivo, verifica se
    algum processo da fila de prontos tem mais prioridade
    que um processo que está rodando. E se tiver, troca o 
    estado dos dois.

*/
void *t_escalonador(void *arg){
    while(1){
        pthread_mutex_lock(&tempo_mutex);
        tempo_atual++;
        pthread_mutex_unlock(&tempo_mutex);
        usleep(1000);
        int rodando = 0;
        for(int i = 0; i < cores; i++){
            int proc = processo_na_cpu[i];
            if(proc != -1 && (processos[proc].escalonado == 0 || processos[proc].concluido == 1)){
                processo_na_cpu[i] = -1;
                cpu_do_processo[proc] = -1;
            }
        }
        int terminar = 1;
        for(int i = 0; i < qtd_process; i++){
            if(processos[i].escalonado == 1 && processos[i].concluido != 1){
                rodando++;
            }
            if(processos[i].concluido == 0){
                terminar = 0;
            }
        }
        if(terminar == 1) break;

        if(escalonador == FCFS){
            while(rodando < cores){
                int index = proximo_processo();
                if(index == -1) break;

                pthread_mutex_lock(&processos[index].mutex);
                processos[index].escalonado = 1;
                pthread_cond_signal(&processos[index].cond);
                pthread_mutex_unlock(&processos[index].mutex);
                rodando++;
            }
        }

        else if(escalonador == SRTN){
            while(rodando < cores){
                int index = proximo_processo();
                if(index == -1) break;

                int cpu_id = -1;
                for(int c = 0; c < cores; c++){
                    if(processo_na_cpu[c] == -1){
                        cpu_id = c;
                        break;
                    }
                }
                if(cpu_id == -1) break;

                processo_na_cpu[cpu_id] = index;
                cpu_do_processo[index] = cpu_id;

                cpu_set_t cp;
                CPU_ZERO(&cp);
                CPU_SET(cpu_id, &cp);
                pthread_setaffinity_np(proc_threads[index], sizeof(cpu_set_t), &cp);
                pthread_mutex_lock(&processos[index].mutex);
                processos[index].escalonado = 1;
                pthread_cond_signal(&processos[index].cond);
                pthread_mutex_unlock(&processos[index].mutex);
                rodando++;
            }

            pthread_mutex_lock(&prontos_mutex);
            for(int i = 0; i < prontos; i++){
                int candidato = fila_prontos[i];
                int pior = -1;
                int pcpu = -1;
                int prest = -1;
                for(int c = 0; c < cores; c++){
                    int proc = processo_na_cpu[c];
                    if(proc != -1 && processos[proc].concluido == 0 && processos[proc].restante > prest){
                        pior = proc;
                        pcpu = c;
                        prest = processos[proc].restante;
                    }
                }

                if(pior != -1 && processos[candidato].dt < prest){
                    pthread_mutex_lock(&processos[pior].mutex);
                    processos[pior].escalonado = 0;
                    pthread_mutex_unlock(&processos[pior].mutex);

                    adiciona_pronto_locked(pior);
                    processo_na_cpu[pcpu] = candidato;
                    cpu_do_processo[pior] = -1;
                    cpu_do_processo[candidato] = pcpu;

                    cpu_set_t cp2;
                    CPU_ZERO(&cp2);
                    CPU_SET(pcpu, &cp2);
                    pthread_setaffinity_np(proc_threads[candidato], sizeof(cpu_set_t), &cp2);
                    pthread_mutex_lock(&processos[candidato].mutex);
                    processos[candidato].escalonado = 1;
                    pthread_cond_signal(&processos[candidato].cond);
                    pthread_mutex_unlock(&processos[candidato].mutex);

                    remove_pronto_locked(i);
                    i--;
                    pthread_mutex_lock(&preempcoes_mutex);
                    preempcoes++;
                    pthread_mutex_unlock(&preempcoes_mutex);
                }
            }
            pthread_mutex_unlock(&prontos_mutex);
        }
        else if(escalonador == PRIORITY && rodando < cores){
            pthread_mutex_lock(&prontos_mutex);
            int qtd = prontos;
            int k = 0;
            while(k < qtd && rodando < cores){
                int idx = fila_prontos[0];
                remove_pronto_locked(0);

                int timer = get_time();
                int quantum = 0;
                double aux = (double) processos[idx].deadline - (timer + processos[idx].restante);
                if(aux < 0){
                    quantum = 1;
                }
                else if(aux < 0.1) quantum = 10;
                else{
                    if(processos[idx].deadline - timer < 2*processos[idx].dt){
                        quantum = processos[idx].dt;
                        if(quantum > 10) quantum = 10;
                    }
                    else quantum = 1;
                }

                if(aux >= 0){
                    if(qtd >= processos[idx].deadline - timer - quantum){
                        quantum = 10;
                    }
                }

                if(quantum > processos[idx].restante){
                    quantum = processos[idx].restante;
                }

                if(quantum <= 0){
                    adiciona_pronto_locked(idx);
                }
                else{
                    processos[idx].qcedido = quantum;
                    pthread_mutex_lock(&processos[idx].mutex);
                    processos[idx].escalonado = 1;
                    pthread_cond_signal(&processos[idx].cond);
                    pthread_mutex_unlock(&processos[idx].mutex);
                    rodando++;
                }
                k++;
            }
            pthread_mutex_unlock(&prontos_mutex);
        }
        sleep(1);
    }
    return NULL;
}

/*
Threads que simulam os processos:

Entram na fila de prontos em t0, 
tentam executar por dt

Foi usado o modificador volatile, para evitar
otimização do compilador e fazer com que o 
tempo real seja gasto.

REF: https://pt.stackoverflow.com/questions/10175/para-que-serve-o-modificador-volatile-do-c-c

No SRTN, o escalonador preempta o processo. No PRIORITY, o processo preempta a si mesmo
ao fim dos quantums cedidos.
*/
void *t_processos(void *arg){
    int index = *(int *)arg;
    free(arg);
    Process *processo = &processos[index];
    processo->restante = processo->dt;

    while(get_time() < processo->t0){
        sleep(1);
    }

    adiciona_pronto(index);

    if(escalonador == FCFS){
        pthread_mutex_lock(&processo->mutex);
        while(processo->escalonado == 0){
            pthread_cond_wait(&processo->cond, &processo->mutex);
        }
        pthread_mutex_unlock(&processo->mutex);

        int timer = get_time();
        while(processo->restante > 0){
            volatile int gastador = 0;
            while(get_time() - timer < 1) for(int i = 0; i < 100; i++){
                gastador = i;
            }
            int atual = get_time();

            processo->restante--;
            timer = atual;
        }
        processo->tf = get_time();
        processo->concluido = 1;   
    }

    else if(escalonador == SRTN){
        while(processo->restante > 0){
            pthread_mutex_lock(&processo->mutex);
            while(processo->escalonado == 0){
                pthread_cond_wait(&processo->cond, &processo->mutex);
            }
            pthread_mutex_unlock(&processo->mutex);

            int timer = get_time();

            volatile int gastador = 0;
            while(get_time() - timer < 1) for(int i = 0; i < 100; i++){
                gastador = i;
            }
            processo->restante--;
            if(processo->restante <= 0){
                processo->tf = get_time();
                processo->concluido = 1;
                break;
            }
        }
    }

    else{
        while(processo->restante > 0){
            pthread_mutex_lock(&processo->mutex);
            while(processo->escalonado == 0){
                pthread_cond_wait(&processo->cond, &processo->mutex);
            }
            pthread_mutex_unlock(&processo->mutex);

            int timer = get_time();

            while(processo->restante > 0){
                volatile int gastador = 0;
                while(get_time() - timer < 1) for(int i = 0; i < 100; i++){
                    int gastador = i;
                }
                timer = get_time();
                processo->restante--;
                if(processo->restante <= 0){
                    processo->tf = timer;
                    processo->concluido = 1;
                    break;
                }
                processo->qusado++;
                if(processo->qusado >= processo->qcedido){
                    processo->qusado = 0;
                    processo->qcedido = 0;
                    processo->escalonado = 0;
                    adiciona_pronto(index);
                    pthread_mutex_lock(&preempcoes_mutex);
                    preempcoes++;
                    pthread_mutex_unlock(&preempcoes_mutex);
                    break;
                }
            }
        }
    }
    return NULL;
}

/*
main(int argc, char *argv[]):
    Recebe tres argumentos de entrada:
        1 - Tipo de escalonador (1 - FCFS || 2 - SRTN || 3 - Prioridade)
        2 - Nome do arquivo trace
        3 - Nome do arquivo saida 
    Inicializa as threads referentes aos processos e ao escalonador
    Registra as informações desejadass na saida
*/
int main(int argc, char *argv[]){
    if(argc != 4){
        printf("Argumentos incorretos. Uso certo:\n ep1 <escalonador> <trace> <saida>\n");
        return 1;
    }

    escalonador = atoi(argv[1]);
    if(escalonador < 1 || escalonador > 3){
        printf("O escalonador deve ser um número inteiro entre 1 e 3!!\n");
        return 1;
    }

    char *trace = argv[2];
    char *saida = argv[3];

    FILE *fp = fopen(trace, "r");
    if(fp == NULL){
        printf("Erro ao abrir o arquivo trace.\n");
        return 1;
    }

    char linha[100];

    while(fgets(linha, sizeof(linha), fp) != NULL){
        if(qtd_process >= MAX_PROCESSES){
            printf("Numero de processos maior que o limite\n");
            fclose(fp);
            return 1;
        }
        if(strlen(linha) < 2) continue;
        Process proc;
        if(sscanf(linha, "%s %d %d %d", proc.name, &proc.t0, &proc.dt, &proc.deadline) != 4){
            printf("Linha inválida no trace! \nFormato correto: <nome> <t0> <dt> <deadline>\n");
            fclose(fp);
            return 1;
        }
        proc.concluido = 0;
        proc.escalonado = 0;
        proc.restante = proc.dt;
        proc.tf = 0;
        proc.qcedido = 0;
        proc.qusado = 0;
        pthread_mutex_init(&proc.mutex, NULL);
        pthread_cond_init(&proc.cond, NULL);
        processos[qtd_process++] = proc;
    }

    fclose(fp);

    cores = sysconf(_SC_NPROCESSORS_ONLN);

    /*
    Para teste com número definido de núcleos
    basta remover o comentário da linha a 
    seguir e modificar para o valor desejado.S
    (observe que inserir um valor maior
    que o número de cores disponíveis pode
    causar comportamento imprevisível)

    Número de cores é limitado a 128 para condizer
    com o vetor de processos nas cpus.
    */

    //cores = 8;

    if(cores > 128) cores = 128;

    for(int i = 0; i < cores; ++i){
        processo_na_cpu[i] = -1;
    }

    for(int i = 0; i < qtd_process; ++i){
        cpu_do_processo[i] = -1;
    }


    if(cores < 1){
        printf("Erro ao obter a quantidade de núcleos\n");
        return 1;
    }

    pthread_t roda_escalonador;
    pthread_create(&roda_escalonador, NULL, t_escalonador, NULL);

    for(int i = 0; i < qtd_process; i++){
        int *arg = malloc(sizeof(int));
        *arg = i;
        pthread_create(&proc_threads[i], NULL, t_processos, arg);
    }

    for(int i = 0; i < qtd_process; i++){
        pthread_join(proc_threads[i], NULL);
    }
    pthread_join(roda_escalonador, NULL);

    FILE *fp_saida = fopen(saida, "w");
    if(!fp_saida){
        printf("Nao foi possivel criar o arquivo de saida.\n");
        return 1;
    }

    int cumpridos = 0;

    for(int i = 0; i < qtd_process; i++){
        int tr = processos[i].tf - processos[i].t0;
        int cumpriu = 0;
        if(processos[i].tf - processos[i].deadline <= 0){
            cumpriu = 1;
            cumpridos++;
        }
        fprintf(fp_saida, "%s %d %d %d\n", processos[i].name, tr, processos[i].tf, cumpriu);
        pthread_mutex_destroy(&processos[i].mutex);
        pthread_cond_destroy(&processos[i].cond);
    }
    fprintf(fp_saida, "%d\n", preempcoes);
    fclose(fp_saida);
    pthread_mutex_destroy(&prontos_mutex);
    pthread_mutex_destroy(&preempcoes_mutex);

    return 0;
}
