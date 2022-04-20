#=============== ENVIRONMENT SETUP SCRIPT =====================

# export P2_WORK=/home/aoiduo/Downloads/card/cw/2/card2
export P2_WORK=$(pwd)

export CARD=$(pwd)"/tools"

export P2_ROOT=$CARD/p2
export P2_LIB=$P2_ROOT/lib
export P2_BENCHMARKS=$P2_ROOT/benchmarks
export PIN=pin-3.20-98437-gf02b61307-gcc-linux
export PIN_ROOT=$P2_ROOT/tools/$PIN

export PATH=$PIN_ROOT:$PATH
export LD_LIBRARY_PATH=$P2_LIB:$LD_LIBRARY_PATH

export GROMACS_PATH=$P2_BENCHMARKS/435.gromacs/run_base_ref_amd64-m64-gcc41-nn.0000
export GOBMK_PATH=$P2_BENCHMARKS/445.gobmk/run_base_ref_amd64-m64-gcc41-nn.0000
export SJENG_PATH=$P2_BENCHMARKS/458.sjeng/run_base_ref_amd64-m64-gcc41-nn.0000
export GROMACS_DATA=$P2_WORK/input
