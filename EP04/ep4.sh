#!/bin/bash

# EP04 - Script para desempenho de servidores

# Disciplina: MAC0422 - Sistemas Operacionais
# Professor: Daniel Batista
# Aluno: Francisco Membrive
#
# Data: 01.07.2025
#
# Script ep4.sh
#
# Argumentos: numero de clientes 
# Tamanhos dos arquivos de testes: tamanhos, em MB, dos arquivos a serem usados para testar os servers. 
# Os tamanhos devem ser escritos em formato decimal e separados por espaço
#
# Saida: cria um arquivo ep4-resultados-{numero de clientes}.pdf, 
# que contem um gráfico com o tempo que cada servidor demorou para
# atender aquela quantidade de clientes naquele tamanho de arquivo.
# Eixo x: tamanho do arquivo
# Eixo y: tempo para atender todos os clientes
#
# Permissao 
#   chmod +x ep4.sh
#
# Uso
#   ./ep4.sh <numero de clientes> <tamanhos dos arquivos de testes>
#
# Exemplo:
#   ./ep4.sh 100 1 2 3 4 5 
#   Marca o tempo que cada servidor demora para atender 100 clientes em arquivos de 
#   1MB, 2MB, 3MB, 4MB e 5MB.
#
# Para o codigo funcionar, eh necessario que o endereco usado pelos servidores 
# esteja inicialmente livre. 
# 
# O codigo tenta eliminar processos que tenham os nomes iguais que possam ter restado
# de outra execucao, mas isso nao garante que o endereco estara livre
#
# Seria possivel fazer isso atraves de programas adicionais que obtivessem
# o PID a partir do endereco, mas esta alem do escopo deste trabalho




# Como eh necessario pelo menos um tamanho para os testes, 
# sao necessarios pelo menos dois argumentos:
if [ $# -lt 2 ]; then
    echo "Uso: $0 <clientes> <tamanho1> [<tamanhoI> ...]"
    exit 1
fi

QTD_CLIENTS=$1
shift
SIZES=("$@")

SERVERS=("ep4-servidor-inet_processos" "ep4-servidor-inet_threads" "ep4-servidor-inet_muxes" "ep4-servidor-unix_threads")
CLIENTS=("ep4-cliente-inet" "ep4-cliente-inet" "ep4-cliente-inet" "ep4-cliente-unix")

# Define o diretorio onde os arquivos temporarios serao criados:
TEMP_DIR="/tmp"

# Mata possíveis processos que já estejam rodando (pelo nome) os servidores e compila em /tmp
echo "Compilando ep4-servidor-inet_processos"
echo "Compilando ep4-servidor-inet_threads"
echo "Compilando ep4-servidor-inet_muxes"
echo "Compilando ep4-servidor-unix_threads"
echo "Compilando ep4-cliente-inet"
echo "Compilando ep4-cliente-unix"

for i in {0..3}; do
    pkill -f "${SERVERS[$i]}" 2>/dev/null || true
    gcc -o "$TEMP_DIR/${SERVERS[$i]}" "ep4-clientes+servidores/${SERVERS[$i]}.c"
    gcc -o "$TEMP_DIR/${CLIENTS[$i]}" "ep4-clientes+servidores/${CLIENTS[$i]}.c"
done

# Cria o arquivo de plot:
PLOT_FILE="/tmp/ep4-resultados-${QTD_CLIENTS}.data"
: > "$PLOT_FILE"

# Laco principal, para cada tamanho passado como argumento, 
# executa os 4 servidores para o numero de clientes desejado
for SIZE in "${SIZES[@]}"; do
    FILE="/tmp/${SIZE}MB.txt"

    echo ">>>>>>> Gerando um arquivo texto de: ${SIZE}MB..."

    base64 /dev/urandom | head -c "$((SIZE * 1024 * 1024))" > "$FILE"
    echo "exit" >> "$FILE"

    TIMES=()

    for i in {0..3}; do
        echo "Subindo o servidor ${SERVERS[$i]}"

        START_TIME=$(/bin/date '+%Y-%m-%d %H:%M:%S')
        "$TEMP_DIR/${SERVERS[$i]}" &> /dev/null
        sleep 1

        # Pega o PID do server para poder mandar o kill depois
        SPID=$(journalctl -o short --grep="Servidor de echo no ar!" | tail -n1 | awk -F'[][]' '{print $2}')


        # Argumento de execucao dos clientes, 127.0.0.1 para inet e vazio para unix
        adit=""

        if [ "$i" -lt 3 ]; then
            adit="127.0.0.1"
        fi

        echo ">>>>>>> Fazendo ${QTD_CLIENTS} clientes ecoarem um arquivo de: ${SIZE}MB..."

        # Armazena o PID de cada cliente para poder aguardar que todos sejam concluidos
        CLIENT_PIDS=()
        for ((j=0; j<QTD_CLIENTS; j++)); do
            "$TEMP_DIR/${CLIENTS[$i]}" $adit < "$FILE" &>/dev/null &
            CLIENT_PIDS+=($!)
        done

        echo "Esperando os clientes terminarem..."

        # Aguarda todos os clientes:
        for pid in "${CLIENT_PIDS[@]}"; do
            wait "$pid"
        done

        echo ">>>>>>> ${QTD_CLIENTS} clientes encerraram a conexão"

        echo "Verificando os instantes de tempo no journald..."

        # Calcula o tempo de execucao:
        #   Aqui, medi o tempo em HH:MM:SS, diferente do enunciado, pois
        #   a execucao no WSL estava demorando mais de 100 minutos para o 
        #   MUXES e o gnuplot nao lidava bem com o formato MMM:SS

        LOGS=$(journalctl -q --since "$START_TIME" -o short-iso | grep "$SERVER")
        FIRST=$(echo "$LOGS" | head -1 | awk '{print $1" "$2}')
        LAST=$(echo "$LOGS" | tail -1 | awk '{print $1" "$2}')
        DURATION=$(dateutils.ddiff "$FIRST" "$LAST" -f "%0H:%0M:%0S")

        TIMES+=("$DURATION")

        echo ">>>>>>> Tempo para servir os ${QTD_CLIENTS} clientes com o ${SERVERS[$i]}: ${DURATION}"


        echo "Enviando um sinal 15 para o servidor ${SERVERS[$i]}..."

        # Aqui, mata o servidor. 

        # Inicialmente, utilizava somente pkill
        # No entanto, por orientacao do monitor, passei a usar 
        # kill com o pid (mais confiavel) e mantive o 
        # pkill pelo nome como uma segunda garantia
        if [ -n "$SPID" ]; then
            kill -15 "$SPID"
        else 
            pkill -f "${SERVERS[$i]}"
        fi
    done

    # Escreve os tempos e o tamanho no arquivo de plot
    echo "$SIZE ${TIMES[@]}" >> "$PLOT_FILE"
    
done

# Cria a gpi file usando formato HH:MM:SS
echo ">>>>>>> Gerando o gráfico de ${QTD_CLIENTS} clientes com arquivos de: $(printf '%sMB ' "${SIZES[@]}")"
GPI_FILE="/tmp/ep4-resultados-${QTD_CLIENTS}.gpi"
cat <<EOF > "$GPI_FILE"
set ydata time
set timefmt "%H:%M:%S"
set format y "%H:%M:%S"
set xlabel 'Dados transferidos por cliente (MB)'
set ylabel 'Tempo para atender ${QTD_CLIENTS} clientes concorrentes'
set term pdfcairo
set output "ep4-resultados-${QTD_CLIENTS}.pdf"
set grid
set key top left
plot "$PLOT_FILE" using 1:2 with linespoints title "Sockets da Internet: Processos", \
     "$PLOT_FILE" using 1:3 with linespoints title "Sockets da Internet: Threads", \
     "$PLOT_FILE" using 1:4 with linespoints title "Sockets da Internet: Mux de E/S", \
     "$PLOT_FILE" using 1:5 with linespoints title "Sockets Unix: Threads"
EOF

# Plota o grafico em pdf
gnuplot "$GPI_FILE"

# Limpa os arquivos em /tmp
rm -f "$TEMP_DIR"/*MB.txt 
rm -f "$PLOT_FILE"
rm -f "$GPI_FILE"
rm -f "$TEMP_DIR"/ep4-servidor-*
rm -f "$TEMP_DIR"/ep4-cliente-*

exit 0


