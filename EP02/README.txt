EP02 - Sistemas Operacionais
Simulador de Corrida

Aluno: Francisco Nassif Membrive

Para executar, rodar make
Assim será criado os executável ep2
que pode ser executado com ./ep2

uso: 
    make && ./ep2 <tamanho_pista> <qtd_ciclistas> <i|e> <opcional: -debug>
    <tamanho_pista>: inteiro de 100 a 2500
    <qtd_ciclistas>: inteiro de 5 a 5*tamanho_pista
    <i|e>: caractere
        - i para abordagem ingenua
        - e para abordagem eficiente
    <-debug>: flag opcional para debug

Para rodar isso é necessário que a máquina tenha o GCC bem como 
a biblioteca pthread

É recomendado que ep2 rode sozinho, para maior precisão dos resultados.

Sistema onde foi testado o programa:
    Ubuntu 22.04.5 LTS
    Ryzen 5 3600 6-core AMD 
    16GB RAM DDR4
    gcc version 11.04
    glibc version 2.35
