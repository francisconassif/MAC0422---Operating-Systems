EP01 - Sistemas Operacionais
Simulador de Processos e Shell

Aluno: Francisco Nassif Membrive

Para executar, rodar make
Assim serão criados os executáveis uspsh e ep1
que podem ser executados com ./ep1 e ./uspsh

./uspsh inicializa o shell, dentro do qual podem ser rodados os 
comandos ls, whoami, chmod, top, cd e ep1

./ep1 recebe os argumentos
./ep1 <escalonador> <trace> <saida>
escalonador: valor de 1 a 3 
    1 - FCFS
    2 - SRTN
    3 - PRIORITY

trace: arquivo de texto com o seguinte formato de linha
    processo t0 dt deadline

processo - string de até 32 caracteres
t0, dt e deadline - inteiros

saida: arquivo onde serão imprimidos os resultados no seguinte
formato de linha:

    processo tr tf cumpriu

processo - nome do processo
tr - tempo de relógio (tf - t0)
tf - tempo de conclusao
cumpriu - 0 se tf > deadline e 1 caso contrario

Para rodar isso é necessário que a máquina tenha o GCC bem como 
as bibliotecas pthread e readline

É recomendado que ep1 rode sozinho, para maior precisão dos resultados.
