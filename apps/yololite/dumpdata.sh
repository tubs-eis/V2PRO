# Copyright (c) 2024 Chair for Chip Design for Embedded Computing,
#                    Technische Universitaet Braunschweig, Germany
#                    www.tu-braunschweig.de/en/eis
# 
# Use of this source code is governed by an MIT-style
# license that can be found in the LICENSE file or at
# https://opensource.org/licenses/MIT
# 
#!/bin/bash

# dump binary data files (input, weights, results) to humand-readable format
# commandline parameters: list of net dirs (e.g. nets/nnqtest_add)

CATCMD="cat" # whole file
CATCMD="head -n 10"

cmp_files () {
    [ -f "$1" ] && [ -f "$2" ] && cmp "$1" "$2"
}

shopt -s nullglob
for net in $@; do

    # subdirs with signed 16-bit data
    for subdir in weights input ref_fixed sim_results emu_results; do
        files=${net}/${subdir}/*.bin
        for f in ${files}; do
            hexdump -x "$f" > "${f/.bin/.hex}"
            od --endian=little -Ax -t d2 --width=16 "$f" > "${f/.bin/.dec}"
        done
    done

    # 32-bit float data
    files=${net}/ref_float/*.bin
    for f in ${files}; do
#        hexdump -e '%x' "$f" > "${f/.bin/.hex}" # hex dump of float not too useful
        od --endian=little -Ax -t f4 "$f" --width=32 > "${f/.bin/.dec}"
    done

    # layerwise: dump data to console
    echo "============ NET ${net} ============"
    for layer in l000 l001 l002 l003 l004; do
        echo "====== LAYER ${layer}"
        for subdir in input weights ref_float ref_fixed sim_results emu_results; do
            files=${net}/${subdir}/${layer}*.dec
            for f in ${files}; do
                echo " ${subdir}: ($f)"
                ${CATCMD} $f
            done
            # dump quantization parameters (only existing in weights/)
            files=${net}/${subdir}/${layer}*.txt
            for f in ${files}; do
                cat "$f"
                echo
            done
        done
        echo "Comparing input, ref_fixed, sim_results and sim_results..."
        i=${net}/input/${layer}.bin
        r=${net}/ref_fixed/${layer}.bin
        s=${net}/sim_results/${layer}.bin
        e=${net}/emu_results/${layer}.bin

        cmp_files $i $r
        cmp_files $i $s
        cmp_files $i $e
        cmp_files $r $s
        cmp_files $r $e
        cmp_files $s $e
        
#        [ -f "$r" ] && [ -f "$s" ] && ! cmp "$r" "$s"
#        [ -f "$r" ] && [ -f "$e" ] && ! cmp "$r" "$e"
#        [ -f "$s" ] && [ -f "$e" ] && ! cmp "$s" "$e"
    done

done
shopt -u nullglob
