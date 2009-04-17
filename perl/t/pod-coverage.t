# Before `make install' is performed this script should be runnable with
# `make test'. After `make install' it should work as `perl pod.t'

use warnings;
use strict;
use Test::More;

eval "use Test::Pod::Coverage";
plan skip_all => "Test::Pod::Coverage required for testing pod coverage"
  if $@;

all_pod_coverage_ok();
