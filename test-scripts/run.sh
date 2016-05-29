#!/bin/bash

#############################################
###### A Combination of RCP's incluced ######
###### examples for RCP and TCP, based ######
######   on the RCP source examples    ######
#############################################

set -e
set -o nounset

capacity=2.4      
rtt=0.1
load=0.9 
shape=1.2
mean=25

rcp_in=../test-output/rcp-flow.tr
rcp_out=../test-output/rcp-flow-vs-delay.out
rcp_log=../test-output/rcp.log

tcp_in=../test-output/tcp-flow.tr
tcp_out=../test-output/tcp-flow-vs-delay.out
tcp_log=../test-output/tcp.log

# Run experiment and scripts.
echo
echo "Running RCP test"
perl -w run-rcp.pl $rcp_in $rcp_log $capacity $rtt $load $shape $mean
echo "Processing RCP test output"
perl -w average.pl $rcp_in $rcp_out

echo
echo "Running TCP test"
perl -w run-tcp.pl $tcp_in $tcp_log $capacity $rtt $load $shape $mean
echo "Processing TCP test output"
perl -w average.pl $tcp_in $tcp_out

echo
echo "Generating Chart"
perl plot.pl $rcp_out $tcp_out "plot.png" $capacity $rtt $load $shape $mean
mv *.png ../test-output/

echo
echo "Done. Raw data and generated charts can be found in the 'test-output' directory"
