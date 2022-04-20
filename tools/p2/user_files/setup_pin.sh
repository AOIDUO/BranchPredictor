#=============== CARD PRACTICAL 2 ENVIRONMENT SETUP SCRIPT =====================

# !!! THE ENVIRONMENT VARIABLE P2_WORK WAS SET AUTOMATICALLY TO POINT TO THE 
# !!! DIRECTORY WHERE THE CARD P2 USER FILES WERE INSTALLED.
# !!! YOU SHOULD NOT NEED TO EDIT IT MANUALLY.
#
export P2_WORK=XXXX

#================= DO NOT CHANGE ANYTHING BELOW THIS LINE ======================

export CARD=/afs/inf.ed.ac.uk/group/teaching/card
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
