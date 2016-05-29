#!/usr/bin/perl -w

#######################################
#   Modified to accept trace file as  #
#   a command line argument, and an   #
# argument for where to store results #
#######################################

$inputFile = $ARGV[0];
$outputFile = $ARGV[1];

for ($j = 0; $j < 10000000; $j++) {
  $sumDur[$j] = 0;
  $avgDur[$j] = 0;
  $maxDur[$j] = 0;
  $numFlows[$j] = 0;
}

open(fileOut, ">$outputFile") or die;
open(fileIn, "$inputFile") or die;

$maximum = 0;
$simTime = 65;
while (<fileIn>) {
  chomp;
  @items = split;
  if ($items[1] <= $simTime) {
    if ($items[7] > $maximum) {
      $maximum = $items[7];
    }
    $sumDur[$items[7]] += $items[9];
    if ($items[9] > $maxDur[$items[7]]) {
      $maxDur[$items[7]] = $items[9];
    }
    $numFlows[$items[7]] += 1;
    $avgDur[$items[7]] = $sumDur[$items[7]] / $numFlows[$items[7]];
  }
}

for ($j = 1; $j <= $maximum; $j++) {
  if ($avgDur[$j] != 0) {
    printf fileOut "$j $avgDur[$j]\n";
  }
}

close(fileIn);
close(fileOut);
