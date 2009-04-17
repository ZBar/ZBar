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

# setup a callback
sub my_handler {
    my ($proc, $image, $closure) = @_;

    # extract results
    foreach my $symbol ($image->get_symbols()) {
        # do something useful with results
        print('decoded ' . $symbol->get_type() .
              ' symbol "' . $symbol->get_data() . "\"\n")
          if(!$symbol->get_count());
    }
}
$proc->set_data_handler(\&my_handler);

# enable the preview window
$proc->set_visible();

# initiate scanning
$proc->set_active();
$proc->user_wait();
