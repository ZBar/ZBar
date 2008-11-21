# Before `make install' is performed this script should be runnable with
# `make test'. After `make install' it should work as `perl Zebra.t'

use warnings;
use strict;
use Test::More tests => 3;

#########################

BEGIN { use_ok('Zebra') }

#########################

my $scanner = Zebra::Scanner->new();
isa_ok($scanner, 'Zebra::Scanner', 'scanner');

#########################

can_ok($scanner, qw(reset new_scan scan_y get_width get_color));

#########################

# FIXME more scanner tests
