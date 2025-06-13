#include "ep3.h"

int algoritmo;
char *pgm_entrada;
char *trace;
char *pgm_saida;

int last_position = 0;

int falhas[2*MAX_LINES];
int falha_count = 0;
int desloc = 0;

bool dois_primeiros(){
    return algoritmo == NEXTFIT || algoritmo == FIRSTFIT;
}

char caractere_final(int idx){
    return (idx % 16 == 15) ? '\n' : ' ';
}

void le_posicao(FILE *f, int idx, char buffer[4]){
    fseek(f, desloc + 4*idx, SEEK_SET);
    fread(buffer, 1, 4, f);
}

void altera(FILE *f, int idx, char valor[3]){
    fseek(f, desloc + 4*idx, SEEK_SET);
    fwrite(valor, 1, 3, f);
    fputc(caractere_final(idx), f);
}

void unit_swap(FILE *f, int i, int j){
    char bloco_i[4];
    char bloco_j[4];

    le_posicao(f, i, bloco_i);
    le_posicao(f, j, bloco_j);

    altera(f, i, bloco_j);
    altera(f, j, bloco_i);
}

bool aloca(int linha, int m, FILE *saida_file){
    int it = 0;

    /*
    posicao inicial: continua da ultima posicao alocada 
    se o algoritmo for nextfit

    ou comeca da posicao 0 para os outros
    */
    int posicao = (algoritmo == NEXTFIT) ? last_position : 0; 

    /* variaveis pra todos */
    int tam = 0;
    int inicio = -1;

    /* variaveis para bestfit */
    int best_tam = 65536;
    int best_inicio = -1;

    /* variaveis para worstfit */
    int worst_tam = 0;
    int worst_inicio = -1;

    /*
    loop principal 

    percorre as 65536 posicoes de memoria
    no caso de first e nextfit, pode 
    terminar antes ao encontrar espaco
    */
    while(it++ < MAX_ALOCATION){

        char buf[4];
        le_posicao(saida_file, posicao, buf);

        if(buf[2] != '0'){
            if(tam == 0) inicio = posicao;
            tam++;
        }

        if(buf[2] == '0' || posicao == MAX_ALOCATION - 1 || it == MAX_ALOCATION - 1){
            if(tam >= m){
                if(dois_primeiros()){
                    for(int i = 0; i < m; i++){
                        altera(saida_file, inicio+i, "  0");
                    }
                    last_position = (inicio + m) % MAX_ALOCATION;
                    return true;
                }
                if(tam < best_tam){
                    best_tam = tam;
                    best_inicio = inicio;
                }
                if(tam > worst_tam){
                    worst_tam = tam;
                    worst_inicio = inicio;
                }
            }
            tam = 0;
            inicio = -1;
        }
        posicao = (posicao + 1) % MAX_ALOCATION;
    }

    if(algoritmo == BESTFIT && best_inicio != -1){
        for(int i = 0; i < m; i++){
            altera(saida_file, best_inicio+i, "  0");
        }
        return true;
    }

    if(algoritmo == WORSTFIT && worst_inicio != -1){
        for(int i = 0; i < m; i++){
            altera(saida_file, worst_inicio+i, "  0");
        }
        return true;
    }

    falhas[falha_count++] = linha;
    falhas[falha_count++] = m;

    return false;
}

void compacta(FILE *saida_file){
    int i = 0;
    int j = 65536 - 1;

    while(i < j){
        char bloco_i[4];
        char bloco_j[4];

        le_posicao(saida_file, i, bloco_i);
        le_posicao(saida_file, j, bloco_j);

        if(bloco_i[2] == '5' && bloco_j[2] != '5'){
            unit_swap(saida_file, i, j);
            i++;
            j--;
        }
        else if(bloco_i[2] == '5'){
            j--;
        }
        else if(bloco_j[2] != '5'){
            i++;
        }
        else{
            i++;
            j--;
        }
    }   
}

void principal(){
    FILE *trace_file = fopen(trace, "r");
    if (trace_file == NULL){
        fprintf(stderr, "Erro ao abrir o arquivo %s\n", trace);
        return;
    }

    FILE *saida_file = fopen(pgm_saida, "r+");
    if (saida_file == NULL){
        fprintf(stderr, "Erro ao abrir o arquivo %s\n", pgm_saida);
        fclose(trace_file);
        return;
    }

    char cabecalho[20];
    for(int i = 0; i < 3; i++){
        if(!fgets(cabecalho, sizeof(cabecalho), saida_file)){
            fprintf(stderr, "Erro na leitura do cabecalho\n");
            fclose(trace_file);
            fclose(saida_file);
            return;
        }
    }

    desloc = ftell(saida_file);

    int l, m;
    char segundo_argumento[20];

    while (fscanf(trace_file, "%d %s", &l, segundo_argumento) != EOF){
        if(strcmp(segundo_argumento, "COMPACTAR") == 0){
            compacta(saida_file);
        }
        else{
            m = atoi(segundo_argumento);
            aloca(l, m, saida_file);
        }
    }

    fclose(trace_file);
    fclose(saida_file);

    for(int i = 0; i < falha_count; i+=2){
        printf("%d %d\n", falhas[i], falhas[i+1]);
    }

    printf("%d\n", falha_count/2);
}




int main(int argc, char *argv[]){
    if (argc != 5){
        fprintf(stderr, "Uso: %s <algoritmo de alocacao> <nome do PGM de entrada> <arquivo de trace> <PGM de saida>\n", argv[0]);
        return 1;
    }

    algoritmo = atoi(argv[1]);
    pgm_entrada = argv[2];
    trace = argv[3];
    pgm_saida = argv[4];

    if(algoritmo < 1 || algoritmo > 4){
        fprintf(stderr, "O algoritmo de alocação deve ser um inteiro de 1 a 4!\n");
        return 1;
    }

    FILE *entrada = fopen(pgm_entrada, "r");
    if (entrada == NULL){
        fprintf(stderr, "Erro ao abrir o arquivo %s\n", pgm_entrada);
        return 1;
    }

    FILE *saida = fopen(pgm_saida, "w");
    if (saida == NULL){
        fprintf(stderr, "Erro ao abrir o arquivo %s\n", pgm_saida);
        fclose(entrada);
        return 1;
    }

    int c;
    while ((c = fgetc(entrada)) != EOF) {
        fputc(c, saida);
    }

    fclose(entrada);
    fclose(saida);

    principal();

    return 0;
}
