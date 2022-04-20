#!/bin/bash
#
if (( $SHLVL < 3 )); then
  mkdir -p logs
  /usr/bin/script -q logs/install.log /bin/bash -c "$0 $*"
  exit 0
fi

# This install.sh script must be run from within the working
# directory in which Pin will be used.
#
# e.g. if your working directory is $HOME/card_p2
#
#  cd $HOME/card_p2
#  /group/project/card/p2/install.sh
#
cwd=`pwd`

/bin/echo "Installing Pin for CARD P2 in" $cwd "on" `date`

/bin/echo -n "1. Copying Pin files..."
cp -r /group/teaching/card/p2/user_files/* . && sed -i "s:XXXX:$cwd:" setup_pin.sh
if [ $? -eq 0 ]; then
  /bin/echo "OK"
else
  /bin/echo "failed!"
  exit 1
fi

/bin/echo "2. Testing the installation"
/bin/echo -n "   - setting up the environment..."
source setup_pin.sh
if [ $? -eq 0 ]; then
  /bin/echo "OK"
else
  /bin/echo "failed!"
  exit 1
fi

/bin/echo -n "   - compiling the BPExample Pin tool..."
cd BPExample && \
rm -f obj-intel64/*.so obj-intel64/*.o && \
make obj-intel64/branch_predictor_example.so TARGET=intel64 PIN_ROOT=$PIN_ROOT &>> $cwd/logs/install_make.log
if [ $? -eq 0 ]; then
  /bin/echo "OK"
else
  /bin/echo "failed!"
  exit 1
fi

/bin/echo -n "   - running the BPExample Pin tool on the gromacs benchmark..."
pin -t $BP_EXAMPLE/obj-intel64/branch_predictor_example.so \
    -BP_type always_taken -o stats_always_taken.out -- \
    $GROMACS_PATH/gromacs_base.amd64-m64-gcc41-nn -silent \
    -deffnm $GROMACS_DATA/gromacs -nice 0 > gromacs.out &>> $cwd/logs/install_pin.log
if [ $? -eq 0 ]; then
  /bin/echo "OK"
  /bin/echo "   Testing completed successfully"
else
  /bin/echo "failed!"
  exit 1
fi

/bin/echo "Environment has been set for using Pin"
/bin/echo "Your P2_WORK directory is" $P2_WORK



