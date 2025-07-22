EP04 - Sistemas Operacionais
Avaliacao de Desempenho de Servidores

Aluno: Francisco Nassif Membrive

Para executar, dar permissao com 
chmod +x ep4.sh

E rodar: 
./ep4.sh <numero de clientes> <tamanhos dos arquivos>

Argumentos: numero de clientes 
Tamanhos dos arquivos de testes: tamanhos, em MB, dos arquivos a serem usados para testar os servers. 
Os tamanhos devem ser escritos em formato decimal e separados por espaço

Saida: cria um arquivo ep4-resultados-{numero de clientes}.pdf, 
que contem um gráfico com o tempo que cada servidor demorou para
atender aquela quantidade de clientes naquele tamanho de arquivo.
Eixo x: tamanho do arquivo
Eixo y: tempo para atender todos os clientes

Exemplo de uso:
  ./ep4.sh 100 1 2 3 4 5 

  Marca o tempo que cada servidor demora para atender 100 clientes em arquivos de 
1MB, 2MB, 3MB, 4MB e 5MB.

Saida do exemplo: ep4-resultados-100.pdf

Requisitos:

    - Para o codigo funcionar, eh necessario que o endereco usado pelos servidores 
    esteja inicialmente livre. 

    - O codigo tenta eliminar processos que tenham os nomes iguais que possam ter restado
    de outra execucao, mas isso nao garante que o endereco estara livre

    - Seria possivel fazer isso atraves de programas adicionais que obtivessem
    o PID a partir do endereco, mas esta alem do escopo deste trabalho


    - O script deve ser rodado em um shell linux em um sistema com suporte
    a journalctl

    - Alem disso, a maquina deve ter instalado o gcc, o gnuplot e o moreutils
    que podem ser instalados no Ubuntu com:

        sudo apt install gcc gnuplot moreutils
    
    - O usuario que executar o script precisa ter acesso a pasta /tmp

    - E o script espera a existencia de um diretorio
        /ep4-clientes+servidores, contendo os seguintes arquivos:
            ep4-servidor-inet_muxes.c
            ep4-servidor-inet_processos.c
            ep4-servidor-inet_threads.c
            ep4-servidor-unix_threads.c
            ep4-cliente-inet.c
            ep4-cliente.unix.c

Link para o vídeo demonstrando a execução:
    https://youtu.be/Za-88xKxunw

Sistema onde foi testado o programa:
    Ubuntu 24.04.2 LTS 
    Ryzen 7 4800HS 8-core AMD 
    16GB RAM DDR4
    gcc version 13.3.0
    glibc version 2.39

Sistema onde foram feitos testes adicionais (não entregues):
    Ubuntu 22.04.5 LTS (WSL)
    Ryzen 5 3600 6-core AMD
    32GB RAM DDR4
    gcc version 11.4.0
    glibc version 2.35
