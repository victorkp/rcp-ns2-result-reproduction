#!/usr/bin/perl -w

$in_file = $ARGV[0];
$log_file = $ARGV[1];
$capacity = $ARGV[2];
$rtt = $ARGV[3];
$load = $ARGV[4];
$num_bottleneck_links = 1;
$pareto_shape = $ARGV[5];
$mean_flow_size = $ARGV[6];
$init_num_flows = 5000;
$sim_end = 300;
$alpha = 0.1;
$beta = 1;

`nice ns rcp.tcl $sim_end $capacity $rtt $load $num_bottleneck_links $alpha $beta $init_num_flows $mean_flow_size $pareto_shape $in_file > $log_file`;
