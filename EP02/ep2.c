/***********************************************************************
EP02 - Simulador de Corrida

Disciplina: MAC0422 - Sistemas Operacionais
Professor: Daniel Batista
Aluno: Francisco Membrive

Data: 18.05.2025

uso: 
    make && ./ep2 <tamanho_pista> <qtd_ciclistas> <i|e> <opcional: -debug>
    <tamanho_pista>: inteiro de 100 a 2500
    <qtd_ciclistas>: inteiro de 5 a 5*tamanho_pista
    <i|e>: caractere
        - i para abordagem ingenua
        - e para abordagem eficiente
    <-debug>: flag opcional para debug

***********************************************************************/

#include "ep2.h"

int tamanho_pista;
int total_ciclistas;
bool debug = false;
char abordagem;

int **pista;
pthread_mutex_t *mutex_posicao;
pthread_mutex_t mutex_pista;

Ciclista *ciclistas;
long unsigned int global_time = 0;
int ciclistas_ativos;
int *rankings;
int *ordem_aleatoria;
bool corrida_acabou = false;

pthread_barrier_t barreira_movimento, barreira_central;

int* lap_rankings[MAX_VOLTAS+1];
int lap_qtd[MAX_VOLTAS+1];
bool volta_impressa[MAX_VOLTAS+1];
int ultima_impressa = 0;
int proxima_eliminada = 2;

bool volta_concluida = false;

void inicializador(int d, int k, char modo){
    tamanho_pista = d;
    total_ciclistas = k;
    abordagem = modo;

    ciclistas = calloc(total_ciclistas, sizeof(Ciclista));
    rankings = calloc(total_ciclistas, sizeof(int));
    ordem_aleatoria = calloc(total_ciclistas, sizeof(int));
    pista = calloc(RAIAS, sizeof(int *));

    for (int i = 0; i < RAIAS; i++) {
        pista[i] = calloc(tamanho_pista, sizeof(int));
        for (int j = 0; j < tamanho_pista; j++) {
            pista[i][j] = -1;
        }
    }

    if(abordagem == 'e'){
        mutex_posicao = calloc(tamanho_pista, sizeof(pthread_mutex_t));
        for (int i = 0; i < tamanho_pista; i++) {
            pthread_mutex_init(&mutex_posicao[i], NULL);
        }
    }
    else{
        pthread_mutex_init(&mutex_pista, NULL);
    }

    for(int i = 0; i < total_ciclistas; i++){
        rankings[i] = i;
    }

    for(int i = total_ciclistas-1; i > 0; i--){
        int j = rand() % (i + 1);
        int temp = rankings[i];
        rankings[i] = rankings[j];
        rankings[j] = temp;
    }

    int posicao_atual = 0;

    for(int i = 0; i < total_ciclistas; i++){
        ordem_aleatoria[i] = rankings[i];
        int raia_atual = i % 5;
        if(i > 0 && i % 5 == 0) posicao_atual++;
        ciclistas[rankings[i]].pos = posicao_atual;
        ciclistas[rankings[i]].lane = raia_atual;
        ciclistas[rankings[i]].id = rankings[i];
        pista[raia_atual][posicao_atual] = rankings[i];
    }

    for(int volta = 0; volta <= MAX_VOLTAS; volta++){
        lap_rankings[volta] = NULL;
        lap_qtd[volta] = 0;
        volta_impressa[volta] = false;
    }
}

void finalizador(){
    for(int i = 0; i < RAIAS; i++){
        free(pista[i]);
    }
    free(pista);

    if(abordagem == 'i'){
        pthread_mutex_destroy(&mutex_pista);
    }
    else{
        for(int i = 0; i < tamanho_pista; i++){
            pthread_mutex_destroy(&mutex_posicao[i]);
        }
        free(mutex_posicao);
    }

    for(int i = 0; i < total_ciclistas; i++){
        pthread_join(ciclistas[i].thread, NULL);
    }

    for(int i = 0; i <= MAX_VOLTAS; i++){
        if(lap_rankings[i]) free(lap_rankings[i]);
    }

    pthread_barrier_destroy(&barreira_movimento);
    pthread_barrier_destroy(&barreira_central);

    free(ciclistas);
    free(rankings);
    free(ordem_aleatoria);
}

int define_velocidade(Ciclista *ciclista){
    if(ciclista->lap < 2) return ciclista->vel = V30KMH;

    int aleatorio = rand() % 100;

    if(ciclista->vel == V30KMH){
        if(aleatorio < 75) return ciclista->vel = V60KMH;
        else return ciclista->vel = V30KMH;
    }

    if(aleatorio < 45) return ciclista->vel = V60KMH;
    return ciclista->vel = V30KMH;
}

void movimento(Ciclista *ciclista, int velocidade){
    ciclista->tempo_atual++;
    if(velocidade == V30KMH && ciclista->tempo_atual % 2 == 0){
        return;
    }
    int posicao_atual = ciclista->pos;
    int proxima_posicao = (posicao_atual - 1 + tamanho_pista) % tamanho_pista;
    int raia_atual = ciclista->lane;
    int outra_raia = raia_atual;
    bool moved = false;

    if(abordagem == 'i') pthread_mutex_lock(&mutex_pista);
    else pthread_mutex_lock(&mutex_posicao[proxima_posicao]);

    if(pista[outra_raia][proxima_posicao] == -1){
        /*
        tenta so andar reto
        */
        pista[ciclista->lane][ciclista->pos] = -1;
        pista[outra_raia][proxima_posicao] = ciclista->id;
        moved = true;
        ciclista->pos = proxima_posicao;
        ciclista->lane = outra_raia;
    }
    else{
        for(int possivel_raia = outra_raia; possivel_raia < RAIAS; possivel_raia++){
            /* tenta ultrapassar por raias mais externas */
            if(pista[possivel_raia][proxima_posicao] == -1){
                pista[ciclista->lane][ciclista->pos] = -1;
                pista[possivel_raia][proxima_posicao] = ciclista->id;
                ciclista->pos = proxima_posicao;
                ciclista->lane = possivel_raia;
                moved = true;
            }
        }
    }

    if(moved && posicao_atual < proxima_posicao){
        ciclista->lap++;
        ciclista->mudou_de_volta = true;
        /*
        se completou uma volta multipla de 5, tenta quebrar
        */
        if(ciclista->lap % 5 == 0 && (rand() % 100) < 10){
            ciclista->status = QUEBRADO;

            pista[ciclista->lane][ciclista->pos] = -1;

            if(abordagem == 'i') pthread_mutex_unlock(&mutex_pista);
            else{
                pthread_mutex_unlock(&mutex_posicao[ciclista->pos]);
            }
        }
        else{
            /*
            tenta descer na nova posicao
            */
            if(ciclista->lane > 0){
                for(int possivel_raia = ciclista->lane - 1; possivel_raia >= 0; possivel_raia--){
                    if(pista[possivel_raia][ciclista->pos] == -1){
                        pista[ciclista->lane][ciclista->pos] = -1;
                        pista[possivel_raia][ciclista->pos] = ciclista->id;
                        ciclista->lane = possivel_raia;
                    }
                }
            }
    
            if(abordagem == 'i') pthread_mutex_unlock(&mutex_pista);
            else{
                pthread_mutex_unlock(&mutex_posicao[ciclista->pos]);
            }
        }

        /*
        sinaliza para a thread central que deve verificar eliminacao 
        valor global e desprotegido, mas as threads nunca tentam flipar pra 0
        somente a thread central flipa pra 0 quando eh garantido que as demais 
        estao na barreira
        */
        volta_concluida = true;
        define_velocidade(ciclista);
        ciclista->tempo_volta = ciclista->tempo_atual;
    }
    else{
        if(abordagem == 'e'){
            pthread_mutex_unlock(&mutex_posicao[proxima_posicao]);
            pthread_mutex_lock(&mutex_posicao[ciclista->pos]);
        }

        /*
        tenta descer na posicao que esta
        */

        if(ciclista->lane > 0){
            for(int possivel_raia = ciclista->lane - 1; possivel_raia >= 0; possivel_raia--){
                if(pista[possivel_raia][ciclista->pos] == -1){
                    pista[ciclista->lane][ciclista->pos] = -1;
                    pista[possivel_raia][ciclista->pos] = ciclista->id;
                    ciclista->lane = possivel_raia;
                }
            }
        }

        if(abordagem == 'i') pthread_mutex_unlock(&mutex_pista);
        else{
            pthread_mutex_unlock(&mutex_posicao[ciclista->pos]);
        }

    }
}

void aleatoriza(){
    for(int i = total_ciclistas-1; i > 0; i--){
        int j = rand() % (i + 1);
        int temp = ordem_aleatoria[i];
        ordem_aleatoria[i] = ordem_aleatoria[j];
        ordem_aleatoria[j] = temp;
    }
}

void *t_ciclista(void *arg){
    Ciclista *ciclista = (Ciclista *)arg;
    ciclista->vel = V30KMH;
    ciclista->lap = 0;
    ciclista->status = ATIVO;
    ciclista->mudou_de_volta = false;
    while(ciclista->status == ATIVO){
        ciclista->sorte = rand() % 100;
        /*
        esse if nao serve pra nada, mas vou manter pois nos testes 
        ele estava presente
        */
        if(ciclista->status == ATIVO) movimento(ciclista, ciclista->vel);
        pthread_barrier_wait(&barreira_movimento);
        pthread_barrier_wait(&barreira_central);
        if(corrida_acabou) break;
    }
    return NULL;
}

bool elimina_volta(int volta){
    if(lap_qtd[volta] <= 0 || lap_rankings[volta] == NULL){
        return false;
    }
    int last_rank = ciclistas_ativos - 1;
    int last_cyclist_id = rankings[last_rank];
    /*evita que o ciclista seja eliminado 2 vezes:*/
    while(last_rank > 0 && ciclistas[last_cyclist_id].status != ATIVO){
        if(last_rank < 2) return false;
        last_rank--;
        last_cyclist_id = rankings[last_rank];
    }
    if(ciclistas[last_cyclist_id].lap >= volta){
        int indice = lap_qtd[volta] - 1;
        int eliminado_id = lap_rankings[volta][indice];
        ciclistas[eliminado_id].status = ELIMINADO;
        pista[ciclistas[eliminado_id].lane][ciclistas[eliminado_id].pos] = -1;
        ciclistas[eliminado_id].lap = volta;
        ciclistas[eliminado_id].pos = tamanho_pista - 1;
        /*printf("Ciclista %d eliminado na volta %d\n", eliminado_id + 1, volta);*/
        return true;
    }
    return false;
}

int last_lap_completed(){
    int last_rank = ciclistas_ativos -1;
    int last_cyclist_id = rankings[last_rank];
    return ciclistas[last_cyclist_id].lap;
}

int compara_ciclistas(const void *a, const void *b){
    Ciclista *ciclista1 = *(Ciclista **)a;
    Ciclista *ciclista2 = *(Ciclista **)b;

    if(ciclista1->lap != ciclista2->lap){
        return ciclista2->lap - ciclista1->lap;
    }

    if(ciclista1->pos != ciclista2->pos) 
        return ciclista1->pos - ciclista2->pos;

    if(ciclista1->status != ciclista2->status){
        return ciclista1->status - ciclista2->status;
    }
    /* se empatam, decide na sorte */
    return ciclista2->sorte - ciclista1->sorte;
}

int atualiza_ranking(){
    aleatoriza();
    Ciclista **ativos = calloc(total_ciclistas, sizeof(Ciclista*));
    int index = 0;

    for(int j = 0; j < total_ciclistas; j++){
        int i = ordem_aleatoria[j];
        if(ciclistas[i].status == ATIVO) ativos[index++] = &ciclistas[i];
        if(ciclistas[i].mudou_de_volta){
            ciclistas[i].mudou_de_volta = false;
            if(ciclistas[i].status == QUEBRADO && !debug){
                printf("Ciclista %d quebrou na volta %d\n", ciclistas[i].id + 1, ciclistas[i].lap);
            }
            if(lap_qtd[ciclistas[i].lap] == 0){
                lap_rankings[ciclistas[i].lap] = malloc(sizeof(int) * total_ciclistas);
            }
            int indice = lap_qtd[ciclistas[i].lap]++;
            lap_rankings[ciclistas[i].lap][indice] = ciclistas[i].id;
        }
    }

    qsort(ativos, index, sizeof(Ciclista*), compara_ciclistas);

    for(int i = 0; i < index; i++){
        rankings[i] = ativos[i]->id;
    }

    free(ativos);

    return index;
}

void imprime_debug(){
    /*
    As linhas comentadas imprimiam a saida tabulada pelo numero maximo
    de caracteres que os ids de ciclistas tinham.

    Como o modo de debug eh mais interessante com menos ciclistas, removi isso.

    Sleep opcional, para poder enxergar:
    */
    /*usleep(100000);*/
    for (int raia = RAIAS - 1; raia >= 0; raia--){
        for(int posicao = 0; posicao < tamanho_pista; posicao++){
            if(pista[raia][posicao] == -1){
                /*fprintf(stderr, "     .");*/
                fprintf(stderr, " .");
            }
            else{
                int aux = pista[raia][posicao] + 1;
                fprintf(stderr, " %d", aux);
                /*
                if(aux < 10){
                    fprintf(stderr, "     %d", aux);
                }
                else if(aux < 100){
                    fprintf(stderr, "    %d", aux);
                }
                else if(aux < 1000){
                    fprintf(stderr, "   %d", aux);
                }
                else if(aux < 10000){
                    fprintf(stderr, "  %d", aux);
                }
                else{
                    fprintf(stderr, " %d", aux);
                }
                */
            }
        }
        fprintf(stderr,"\n");
    }
}

void imprime_volta(int lap){
    if(debug) return;
    if(volta_impressa[lap]) return;
    int qtd_ciclistas = lap_qtd[lap];
    if(qtd_ciclistas > 1){
        printf("Ranking da volta %d:\n\n", lap);
        for(int i = 0; i < qtd_ciclistas; i++){
            printf("Ciclista %d\n", lap_rankings[lap][i] + 1);
        }
        printf("\n");
    }

    volta_impressa[lap] = true;
}

void imprime_relatorio(){
    Ciclista **nao_quebrados = calloc(total_ciclistas, sizeof(Ciclista*));
    int index = 0;

    for(int i = 0; i < total_ciclistas; i++){
        if(ciclistas[i].status != QUEBRADO) nao_quebrados[index++] = &ciclistas[i]; 
    }

    qsort(nao_quebrados, index, sizeof(Ciclista*), compara_ciclistas);

    printf("RANKING FINAL:\n");

    for(int i = 0; i < index; i++){
        printf("#%d:\tCiclista %d\t", i + 1, nao_quebrados[i]->id+1);
        printf("Tempo final:\t%ld0 ms\n", nao_quebrados[i]->tempo_volta*6);
    }

    free(nao_quebrados);

    Ciclista **quebrados = calloc(total_ciclistas, sizeof(Ciclista*));
    index = 0;

    for(int i = 0; i < total_ciclistas; i++){
        if(ciclistas[i].status == QUEBRADO) quebrados[index++] = &ciclistas[i];
    }

    qsort(quebrados, index, sizeof(Ciclista*), compara_ciclistas);

    for(int i = 0; i < index; i++){
        printf("Ciclista %d quebrou na volta %d ao tempo %ld0 ms\n", 
            quebrados[i]->id +1, quebrados[i]->lap, 6*quebrados[i]->tempo_volta);
    }

    free(quebrados);
}

int main(int argc, char *argv[]) {
    if (argc < 4) {
        fprintf(stderr, "Uso correto: %s <tamanho_da_pista> <quantidade_de_ciclistas> <i|e> <-debug|>\n", argv[0]);
        return 1;
    }

    tamanho_pista = atoi(argv[1]);
    total_ciclistas = atoi(argv[2]);
    abordagem = argv[3][0];

    if(argc > 4){
        if(strcmp(argv[4], "-debug") == 0) debug = true;
    }

    if (abordagem != 'i' && abordagem != 'e') {
        fprintf(stderr, "O terceiro parametro deve ser o caractere 'i' para abordagem ingenua ou o caractere 'e' para abordagem eficiente\n");
        return 1;
    }

    if(tamanho_pista < 100 || tamanho_pista > 2500) {
        fprintf(stderr, "O comprimento da pista deve ser um inteiro de 100 a 2500.\n");
        return 1;
    }
    if(total_ciclistas < 5 || total_ciclistas > 5 * tamanho_pista) {
        fprintf(stderr, "O numero de ciclistas deve ser de 5 a %d para este tamanho de pista.\n", 5 * tamanho_pista);
        return 1;
    }

    srand(time(NULL));

    inicializador(tamanho_pista, total_ciclistas, abordagem);

    pthread_barrier_init(&barreira_movimento, NULL, total_ciclistas + 1);
    pthread_barrier_init(&barreira_central, NULL, total_ciclistas+1);

    for(int i = 0; i < total_ciclistas; i++){
        pthread_create(&ciclistas[i].thread, NULL, t_ciclista, &ciclistas[i]);
    }

    ciclistas_ativos = total_ciclistas;

    int ultima_terminada = 0;
    int barreira_atual = total_ciclistas;


    while(ciclistas_ativos > 1){
        if(debug) imprime_debug();
        global_time++;
        pthread_barrier_wait(&barreira_movimento);

        if(volta_concluida){
            ciclistas_ativos = atualiza_ranking();

            if(ciclistas_ativos == 1) corrida_acabou = true;
            ultima_terminada = last_lap_completed();

            while(ultima_terminada > ultima_impressa){
                if(ciclistas_ativos < 2) break;
                while(ultima_terminada >= proxima_eliminada && elimina_volta(proxima_eliminada)){
                    proxima_eliminada+=2;
                    ciclistas_ativos--;
                    ultima_terminada = last_lap_completed();
                    if(ciclistas_ativos < 2) break;
                }
                while(ultima_terminada > ultima_impressa){
                    ultima_impressa++;
                    imprime_volta(ultima_impressa);
                }
                ciclistas_ativos = atualiza_ranking();
                if(ciclistas_ativos < 2) break;
                ultima_terminada = last_lap_completed();
            }
            if(ciclistas_ativos < 2) corrida_acabou = true;
        }
        volta_concluida = false;

        if(barreira_atual != ciclistas_ativos){
            barreira_atual = ciclistas_ativos;
            pthread_barrier_destroy(&barreira_movimento);
            pthread_barrier_init(&barreira_movimento, NULL, ciclistas_ativos+1);

            pthread_barrier_wait(&barreira_central);
            pthread_barrier_destroy(&barreira_central);
            pthread_barrier_init(&barreira_central, NULL, ciclistas_ativos + 1);
        }

        else{
            pthread_barrier_wait(&barreira_central);
        }
    }

    corrida_acabou = true;

    imprime_relatorio();

    finalizador();

    return 0;

}
