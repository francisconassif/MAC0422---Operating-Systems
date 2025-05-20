/***********************************************************************
EP02 - Simulador de Corrida - Header

Disciplina: MAC0422 - Sistemas Operacionais
Professor: Daniel Batista
Aluno: Francisco Membrive

Data: 18.05.2025

Headers do programa ep2.c, necessario para que a compilacao ocorra 
corretamente.

***********************************************************************/

#ifndef EP2_H
#define EP2_H

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <pthread.h>
#include <stdbool.h>

#define RAIAS 10

#define V30KMH 1
#define V60KMH 2
 
#define ATIVO 0
#define ELIMINADO 1
#define QUEBRADO 2

/*
Em casos extremos e improvaveis, e possivel que o numero de voltas 
chegue a ate 50000, mas o tempo de execucao seria tao longo que 
considerei alem do escopo deste trabalho uma corrida com mais de 
25000 voltas.
*/
#define MAX_VOLTAS 25000


/****************************************************************

Parametros do programa:

./ep2 <tamanho_pista> <total_ciclistas> <i|e> <opcional:"-debug>"

****************************************************************/

/*
int tamanho_pista: numero de posicoes horizontais da pista,
valor deve ser um inteiro com valor de 100 a 2500
*/
extern int tamanho_pista;

/*
int total_ciclistas: numero de ciclistas concorrendo na
corrida, deve ser um valor entre 5 e 5 vezes o tamanho
da pista
*/
extern int total_ciclistas;

/*
bool debug: true se inserido um quarto argumento na linha
de execucao do ep2 "-debug"
    - execucao com debug: imprime em stderr, a cada iteracao,
    todas as posicoes da pista, com um '.' se estiver vazia
    e a id do ciclista se estiver ocupada
    - execucao sem debug: imprime o ranking final de cada volta, 
    diretamente em stdout

Ambas as versoes imprimem um relatorio final com a classificacao 
dos ciclistas que nao quebraram, com o tempo em que cruzaram a linha
de chegada pela ultima vez. Alem disso, imprime, para cada ciclista
que quebrou, a ultima volta em que cruzou a linha de chegada e o 
tempo em que cruzou. O relatorio final sempre e impresso em stdout.
*/
extern bool debug;

/*
char abordagem: 'i' para abordagem ineficiente, que trava o mutex da 
pista inteira a cada movimento e 'e' para abordagem eficiente, que 
trava somente o mutex da coluna para qual o ciclista quer se mover.
*/
extern char abordagem;

/*
Struct que representa cada ciclista:

int id: identificador do ciclista, a ser utilizado
para acessar o ciclista no vetor de ciclistas e para
as impressoes. e 0-index para o programa e 1-index 
nas impressoes.

int pos: posicao linear do ciclista na pista

int lane: posicao vertical do ciclista na pista

int lap: ultima volta em que o ciclista cruzou a
linha de chegada

int status: assume os tres valores abaixo
    - ATIVO - ainda esta na corrida
    - QUEBRADO - quebrou e sai espontaneamente
    - ELIMINADO - foi eliminado pela thread central
    por estar em ultimo numa volta par

int vel: velocidade atual do ciclista, pode ser V30KMH 
(1 posicao a cada 2 iteracoes) ou V60KMH (1 posicao a
cada 1 iteracao)

int sorte: criterio aleatorio de desempate, muda a cada
clock

int tempo_atual: relogio proprio do ciclista, mede em 
quantidade de iteracoes, de modo que na impressao precisa
ser multiplicado por 60 ms

int tempo_volta: registra o tempo_atual no instante em que 
cruzou a linha de chegada pela ultima vez

bool moveu: sinaliza se o ciclista esta esperando que as posicoes 
à sua frente se desocupem ou se ja fez seu movimento naquela ite-
racao

pthread_t thread: thread que representa o ciclista. Ativa ate 
que o ciclista quebre e saia espontaneamente da corrida ou ate
que a thread central decida que o ciclista foi eliminado.
*/
typedef struct {
    int id;
    int pos;     
    int lane;         
    int lap;          
    int status;       
    int vel;
    int sorte;
    bool mudou_de_volta;
    long unsigned int tempo_volta;
    long unsigned int tempo_atual;
    pthread_t thread; 
} Ciclista;

/*
Matriz com os ids dos ciclistas em cada posicao,
-1 para as posicoes vazias
*/
extern int **pista;

/*
Vetor de ciclistas.
*/
extern Ciclista *ciclistas;

/*
Mutexes:
    mutex_posicao: trava a coluna (abordagem eficiente)
    mutex_pista: trava todas as posicoes (aordagem ineficiente)
*/
extern pthread_mutex_t *mutex_posicao;
extern pthread_mutex_t mutex_pista;

/*
Contadores de tempo e de ciclistas_ativos
*/
extern unsigned long int global_time;
extern int ciclistas_ativos;

/*
Vetor com o ranking atual de ciclistas ativos
ranking[0] = algum_id
significa que o ciclista algum_id estava em
primeiro lugar no instante da ultima verificacao
*/
extern int *rankings;

/*
Vetor que contem os indices dos ciclistas em ordem
aleatoria para insercao nos rankings das voltas
*/
extern int *ordem_aleatoria;

/*
Flag booleana para sinalizar que as threads nao devem executar 
mais. Especialmente para o ciclista que vence a corrida e nao
sai do loop normal que verifica se esta ativo.
*/
extern bool corrida_acabou;

/*
Barreiras
    barreira_movimento: aguarda todos os ciclistas se movimentarem
    barreira_central: aguarda a entidade central atualizar os rankings
    e eliminar o ciclista em ultimo lugar se necessario
*/
extern pthread_barrier_t barreira_movimento, barreira_central;

/*
Matriz de voltas X ranking das voltas.

Para cada volta, inicializa quando o primeiro ciclista
chega naquela volta. A partir dai, os demais ciclistas se
inserem, ate o ultimo. Quando o ultimo chega, o ranking 
e impresso e a memoria liberada.
*/
extern int* lap_rankings[MAX_VOLTAS+1];

/*
lap_qtd eh um vetor auxiliar que armazena a quantidade de
ciclistas que ja chegaram, que tambem eh o indice do proximo
a ser inserido no ranking.
*/
extern int lap_qtd[MAX_VOLTAS+1];

/*
volta_impressa eh outro vetor auxiliar que evita impressoes
duplicadas de uma mesma volta e tentativas de imprimir uma
volta que ja nao esta na memoria.
*/
extern bool volta_impressa[MAX_VOLTAS+1];

/*
int ultima_impressa: ultima volta cujo ranking foi impresso

int proxima_eliminada: proxima volta em que os ciclistas 
devem ser eliminados
*/
extern int ultima_impressa;
extern int proxima_eliminada;

/*
bool volta_concluida

tentativa de otimizar, fazendo reordenacoes somente em momentos em que
alguma thread termina uma volta
*/
extern bool volta_concluida;

/*
void inicializador(int d, int k, char modo)
    recebe como parametros
    d - tamanho da pista
    k - quantidade de ciclistas
    modo - 'i' ou 'e'

    e inicializa as variaveis necessarias para 
    o programa rodar.
*/
void inicializador(int d, int k, char modo);

/*
void finalizador()
    destroi as variaveis e libera a memoria
*/
void finalizador();

/*
int define_velocidade(Ciclista *ciclista)

define a velocidade do ciclista baseada nos 
criterios propostos:
    30KMH se esta na primeira volta
    a partir da segunda volta:
    75% de chance de 60KHM se fez a volta anterior a 30KMH
    25% de chance de 30KMH se fez a volta anterior a 30KMH
    45% de chance de 60MKH se fez a volta anterior a 60KMH
    55% de chance de 30KMH se fez a volta anterior a 30KMH
*/
int define_velocidade(Ciclista *ciclista);

/*
void movimento(Ciclista *ciclista, int velocidade)

Recebe o ciclista e sua velocidade e faz a maior parte
do trabalho da thread ciclista:

incrementa o tempo atual da thread
se o tempo for par, move somente se a velocidade for 60KMH

tenta andar para a posicao seguinte, se estiver ocupada,
tenta ultrapassar

se todos os ciclistas da posicao a frente ja tiverem se movimentado,
nao se movimenta (fica com a velocidade dos ciclistas a frente ate
aparecer oportunidade de ultrapassar num clock futuro)

depois de se mover, tenta descer para raias menores caso haja espaco

se completar uma volta, sorteia a possibilidade de quebra e atualiza o
ranking daquela volta
*/
void movimento(Ciclista *ciclista, int velocidade);

/*
void *t_ciclista(void *arg)

funcao que as threads ciclistas rodam
inicializa os valores e depois entra num loop
que movimenta o ciclista ate que ele nao tenha
mais status ATIVO
*/
void *t_ciclista(void *arg);

/*
bool elimina_volta(int volta)

retorna true se todos os ciclistas ja concluiram aquela
volta e define o status do ciclista que terminou em 
ultimo como ELIMINADO
*/
bool elimina_volta(int volta);

/*
retorna o valor da ultima volta
que o ultimo ciclista do ranking
completou

(nao ha nenhum ciclista em voltas menores)
*/
int last_lap_completed();

/*
funcao de comparacao para usar no qsort

ordenacao primaria por volta
e secundaria por posicao (invertida pois
posicoes menores sao mais proximas da linha
de chegada)
*/
int compara_ciclistas(const void *a, const void *b);

/*
int atualiza_ranking()

ordena os ciclistas ativos, atualizando o vetor rankings
e retorna a quantidade de ciclistas ativos
*/
int atualiza_ranking();

/*
imprime todas as posicoes da pista em stderr
*/
void imprime_debug();

/*
void imprime_volta(int lap)

imprime a volta lap se nao estiver em modo de debug
e se nao tiver imprimido ela antes
*/
void imprime_volta(int lap);

/*
void imprime_relatorio()

imprime o relatorio final conforme esplicado
no cabecalho
*/
void imprime_relatorio();

/*
main: contem a thread principal que usa
todas as funcoes acima para administrar 
a corrida conforme as condicoes definidas
*/
int main(int argc, char *argv[]);

#endif
