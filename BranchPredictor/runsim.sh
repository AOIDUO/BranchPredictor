#!/bin/bash

mkdir obj-intel64/
make obj-intel64/branch_predictor.so TARGET=intel64 PIN_ROOT=$PIN_ROOT;

if [[ $1 == 'all' ]] ; then 
    for bench in sjeng gobmk gromacs ; do
        for bp_type in 'local' 'gshare' 'tournament' ; do
            for num_bp_entry in 128 1024 4096 ; do
                printf '%-12.12s ' $bp_type $num_bp_entry $bench
                ./runsim.sh $bp_type $num_bp_entry $bench 2>&1 1>/dev/null | grep "Prediction accuracy"
                # printf "\n"
            done
        done
    done
else 
    if [[ $3 == 'sjeng' ]] ; then 
    	benchmark="$SJENG_PATH/sjeng_base.amd64-m64-gcc41-nn $SJENG_PATH/ref.txt"
        pin -t $BP_EXAMPLE/obj-intel64/branch_predictor.so -BP_type $1 -o "$1.out" -num_BP_entries $2 -- $benchmark
    elif [[ $3 == 'gobmk' ]] ; then 
        pin -t $BP_EXAMPLE/obj-intel64/branch_predictor.so -BP_type $1 -o "$1.out" -num_BP_entries $2 -- $GOBMK_PATH/gobmk_base.amd64-m64-gcc41-nn --quiet --mode gtp < $GOBMK_PATH/13x13.tst 
    elif [[ $3 == 'gromacs' ]] ; then
    	benchmark="$GROMACS_PATH/gromacs_base.amd64-m64-gcc41-nn -silent -deffnm $GROMACS_DATA/gromacs"
        pin -t $BP_EXAMPLE/obj-intel64/branch_predictor.so -BP_type $1 -o "$1.out" -num_BP_entries $2 -- $benchmark
    elif [[ $3 == 'test' ]] ; then
        pin -t $BP_EXAMPLE/obj-intel64/branch_predictor.so -BP_type $1 -o "$1.out" -num_BP_entries $2 -- ../tests/test.out
    fi
fi

# pin -t $BP_EXAMPLE/obj-intel64/branch_predictor.so -BP_type $bp_type -o "$bp_type.out" -num_BP_entries $num_bp_entry -- ../tests/test.out
