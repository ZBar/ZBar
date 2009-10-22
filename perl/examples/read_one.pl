#!/usr/bin/env perl
use warnings;
use strict;
require Barcode::ZBar;

# create a Processor
my $proc = Barcode::ZBar::Processor->new();

# configure the Processor
$proc->parse_config("enable");

# initialize the Processor
$proc->init($ARGV[0] || '/dev/video0');

# enable the preview window
$proc->set_visible();

# read at least one barcode (or until window closed)
$proc->process_one();

# hide the preview window
$proc->set_visible(0);

# extract results
foreach my $symbol ($proc->get_results()) {
  # do something useful with results
  print('decoded ' . $symbol->get_type() .
        ' symbol "' . $symbol->get_data() . "\"\n");
}
