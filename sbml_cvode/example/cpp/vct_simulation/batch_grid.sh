#!/bin/bash

BIN="./QSP_vct"
FILES=./grid_param/*.xml
OUTDIR="./out_grid"

for file in $FILES
do
    i="$(echo $file | rev | cut -d'/' -f1 | rev | cut -d'.' -f1)"
    cmd="${BIN} -i ${file} -o ${OUTDIR} -n solution_${i}.csv"  
    echo $cmd
    eval $cmd
done
