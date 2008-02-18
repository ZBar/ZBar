#!/usr/bin/perl
use warnings;
use strict;
use Frontier::Client;
use Data::Dumper;
my $s = Frontier::Client->new('url' => 'http://dev.upcdatabase.com/rpc');

$| = 1; # autoflush

foreach (@ARGV) {
  lookup($_);
}
if(!-t) {
  while(1) {
    my $decode = <STDIN>;
    last unless(defined($decode));
    chomp($decode);
    lookup($decode);
  }
}

sub lookup {
  my $decode = shift;
  if($decode =~ m[^(EAN-13:)?(\d{12,13})$]) {
    my $ean = $2;
    $ean = $s->call('calculateCheckDigit', $ean . "C")
      if(length($ean) == 12);
    print("[$decode] ");
    my $result = $s->call('lookupEAN', $s->string($ean));
    if(ref($result)) {
      print((!$result->{found} ||
            (ref($result->{found} && !$result->{found}->value())))
            ? "not found\n"
            : "$result->{description}\n")
    }
    else {
      print("$result\n");
    }
  }
  else {
    print("$decode\n");
  }
}
