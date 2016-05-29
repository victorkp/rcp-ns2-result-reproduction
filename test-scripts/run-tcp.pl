#!/usr/bin/perl -w

# Parse command-line arguments.
$in_file = $ARGV[0];
$log_file = $ARGV[1];
$capacity = $ARGV[2];
$rtt = $ARGV[3];
$load = $ARGV[4];
$num_bottleneck_links = 1;
$pareto_shape = $ARGV[5];
$mean_flow_size = $ARGV[6];
$init_num_flows = 10000;
$num_flows = 100000;

`nice -n +20 ns tcp.tcl $num_flows $capacity $rtt $load $num_bottleneck_links $init_num_flows $mean_flow_size $pareto_shape $in_file > $log_file`;
