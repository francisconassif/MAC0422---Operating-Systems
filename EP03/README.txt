EP03 - Sistemas Operacionais
Simulador de Memoria

Aluno: Francisco Nassif Membrive

Para executar, rodar make
Assim será criado os executável ep3
que pode ser executado com ./ep3

uso: 
    make && ./ep3 <algoritmo> <pgm de entrada> <txt de trace> <pgm de saida>
    <algoritmo>: inteiro de 1 a 4 em que
        1 - First fit
        2 - Next fit
        3 - Best fit
        4 - Worst Fit
    <pgm de entrada>: nome de um arquivo .pgm que contem as 65536 posicoes de memoria
    <txt de trace>: nome de um arquivo .txt que contem as requisicoes de memoria 
    <pgm de saida>: nome de um arquivo .pgm em que sera registrado o estado da memoria apos as alocacoes
    

Para rodar isso é necessário que a máquina tenha o GCC

O programa é deterministico, de modo que rodar outros programas nao deve afetar o resultado
final.

A saida corresponde a quantidade de falhas de memoria e as linhas do arquivo trace que nao 
foram satisfeitas

Sistema onde foi testado o programa:
    Ubuntu 22.04.5 LTS (WSL)
    Ryzen 5 3600 6-core AMD 
    32GB RAM DDR4
    gcc version 13.03
    glibc version 2.39
