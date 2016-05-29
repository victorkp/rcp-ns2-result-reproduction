#!/bin/perl

use strict;
use warnings;
use Chart::Gnuplot;

my $file_rcp = $ARGV[0];
my $file_tcp = $ARGV[1];
my $file_out = $ARGV[2];

# Link capacity in bits/second
my $link_capacity = $ARGV[3] * 1000000000;
my $link_rtt = $ARGV[4];

# Used for processor sharing ideal line generation
my $pareto_rho = $ARGV[5];

# Segment size in bits
my $segment_size = 8000;

my @flow_sizes = (1..100000);


# Generates what TCP slow start looks like,
# if all flows never left slow start
sub slowstart_line() {
    my @line;
    foreach my $flow (@flow_sizes) {
        # RTT * number of needed round trips + propagation delay
        my $time = $link_rtt * (1.0 * log($flow + 1)/log(2) + 0.5)
                      + $segment_size / $link_capacity;
        push (@line, $time);
    }

    return @line;
}

# Generates ideal processor sharing line
sub processor_sharing_line() {
    my @line;
    foreach my $flow (@flow_sizes) {
        my $time = (($flow * $segment_size) / ($link_capacity * (1 - $pareto_rho))
                      + (1.5 * $link_rtt));
        push (@line, $time);
    }

    return @line;
}

# Given filename, returns references to size and duration lists
sub line_from_file($) {
    my $file = $_[0];

    my @sizes;
    my @durations;

    open(my $fh, '<:encoding(UTF-8)', $file)
            or die "Could not open file '$file' $!";
   
    # Data files contain <size, duration> pairs on 
    # a single line, separated by whitespace
    while (<$fh>) {
        chomp;
        my @values = split(/\s/);
        push(@sizes, $values[0]);
        push(@durations, $values[1]);
    }

    return (\@sizes, \@durations);
}

my ($rcp_sizes, $rcp_durations) = line_from_file($file_rcp);
my ($tcp_sizes, $tcp_durations) = line_from_file($file_tcp);
my @ps_line = processor_sharing_line();
my @ss_line = slowstart_line();

########## LOG SCALE CHART GENERATION ############
my $chart = Chart::Gnuplot->new(
        output => "log-$file_out",
        imagesize => "1.3, 1.3",
        title  => "Average Flow Completion Times (Log scale in X and Y)",
        xlabel => "Flow Size (in packets)",
        ylabel => "Average Completion Time (in seconds)",
        yrange => [0.1, 10],
        xrange => [1, 100000],
        logscale => 'xy',
    );

my $ps_data = Chart::Gnuplot::DataSet->new(
        xdata => \@flow_sizes,
        ydata => \@ps_line,
        title => "Processor Sharing",
        style => "line",
        color => "red",
        linetype => "dash",
    );

my $ss_data = Chart::Gnuplot::DataSet->new(
        xdata => \@flow_sizes,
        ydata => \@ss_line,
        title => "Slow Start",
        style => "line",
        color => "dark-red",
    );

my $rcp_data = Chart::Gnuplot::DataSet->new(
        xdata => $rcp_sizes,
        ydata => $rcp_durations,
        title => "RCP",
        style => "linespoints",
        pointsize => 1,
        pointtype => 'cross',
        color => "blue",
    );

my $tcp_data = Chart::Gnuplot::DataSet->new(
        xdata => \@{$tcp_sizes},
        ydata => \@{$tcp_durations},
        title => "TCP",
        style => "linespoints",
        pointsize => 1,
        pointtype => 'cross',
        color => "green",
    );


$chart->plot2d($rcp_data, $tcp_data, $ps_data, $ss_data);

########## NORMAL SCALE CHART GENERATION ############
#
# Edit data to be only flows <= 2000 packets long
@flow_sizes = (1..2000);
my @truncated_ps_line = @ps_line[0..1999];
my @truncated_ss_line = @ss_line[0..1999];

my $index = -1;
for my $d (@{$rcp_durations}) {
    if($d > 2000) {
        last;
    } else {
        $index++;
    }
}
my @truncated_rcp_sizes = @{$rcp_sizes}[0..$index];
my @truncated_rcp_durations = @{$rcp_durations}[0..$index];

$index = -1;
for my $d (@{$tcp_durations}) {
    if($d >= 2000) {
        last;
    } else {
        $index++;
    }
}
my @truncated_tcp_sizes = @{$tcp_sizes}[0..$index];
my @truncated_tcp_durations = @{$tcp_durations}[0..$index];

$chart = Chart::Gnuplot->new(
        output => "normal-$file_out",
        imagesize => "1.3, 1.3",
        title  => "Average Flow Completion Times (Log scale in Y)",
        xlabel => "Flow Size (in packets)",
        ylabel => "Average Completion Time (in seconds)",
        yrange => [0.1, 10],
        xrange => [1, 2000],
        logscale => 'y',
    );

$ps_data = Chart::Gnuplot::DataSet->new(
        xdata => \@flow_sizes,
        ydata => \@truncated_ps_line,
        title => "Processor Sharing",
        style => "line",
        color => "red",
        linetype => "dash",
    );

$ss_data = Chart::Gnuplot::DataSet->new(
        xdata => \@flow_sizes,
        ydata => \@truncated_ss_line,
        title => "Slow Start",
        style => "line",
        color => "dark-red",
    );

$rcp_data = Chart::Gnuplot::DataSet->new(
        xdata => \@truncated_rcp_sizes,
        ydata => \@truncated_rcp_durations,
        title => "RCP",
        style => "linespoints",
        pointsize => 1,
        pointtype => 'cross',
        color => "blue",
    );

$tcp_data = Chart::Gnuplot::DataSet->new(
        xdata => \@truncated_tcp_sizes,
        ydata => \@truncated_tcp_durations,
        title => "TCP",
        style => "linespoints",
        pointsize => 1,
        pointtype => 'cross',
        color => "green",
    );


$chart->plot2d($rcp_data, $tcp_data, $ps_data, $ss_data);
