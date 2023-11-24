#!/bin/bash

# Loop para executar o comando principal para x de 1 a 20
for ((x = 1; x <= 20; x++)); do
    make
    echo "Executando para x = $x"
    
    # Comando time para medir o tempo de execução e salvar a saída em um arquivo
    { time ./ems ./jobs $x; } 2>> benchmark_output.txt
    
    echo "--------------------------------------"
    clear
    make clean
done