package Mac::Finder::AliasRecord;

# Generate(/Parse) a Mac "alias record" binary string/file.
#
# Currently just enough is implemented to satisfy immediate requirements
# (ie, write backgroundImageAlias to .DS_Store for DMG)
#
# based on these documents:
# http://www.geocities.com/xhelmboyx/quicktime/formats/alias-layout.txt
# http://sebastien.kirche.free.fr/python_stuff/MacOS-aliases.txt
#
# FIXME interface is very poor...

use warnings;
use strict;
use DateTime;
use File::Spec;
use File::Spec::Mac;
use Encode qw(encode);
require Exporter;

our $VERSION = '0.1';
our @ISA = qw(Exporter);

my %FSEncodings = (
  MacFS   => ['RW', ''],
  MFS     => ['RW', ''],
  HFS     => ['BD', ''],
  'HFS+'  => ['H+', ''],

  AudioCD => ['', 'JH'],
  ISO9660 => ['', 'AG'],
  FAT     => ['', 'IS'],
  Joliet  => ['', 'Jo'],
  'ISO9660+Joliet' => ['', 'Jo'],
);

my %DiskEncodings = (
  HD          => 0,
  FixedHD     => 0,
  Network     => 1,
  NetworkDisk => 1,
  Floppy      => 4,
  Floppy1440  => 4,
  Other       => 5,
  OtherDisk   => 5,
);

my %RecordEncodings = (
  parentDir     => 0x00,
  absolutePath  => 0x02,
  unicodeFile   => 0x0e,
  unicodeVolume => 0x0f,
  volumePath    => 0x12,
);

sub new {
  my $class = shift || __PACKAGE__;
  my $self = {
    aliasCreator => '',
    aliasVersion => 2,
    aliasType => 'file',
    volume => '',
    volumeCreated => 0,
    volumeFS => 'HFS',
    volumeDisk => undef,
    volumeAttrs => 0,
    directoryID => 0,
    file => '',
    fileID => 0,
    fileCreated => 0,
    fileType => '',
    fileCreator => '',
    nlvlFrom => -1,
    nlvlTo => -1,
    records => { },
    @_
  };
  if(exists($self->{path})) {
    my $path = $self->{path};
    my ($vol, $dir, $file) = File::Spec::Mac->splitpath($path);
    $vol =~ s/:$//;
    my @dir = File::Spec::Mac->splitdir($dir);
    while(@dir && !$dir[0]) {
      shift(@dir);
    }
    while(@dir && !$dir[-1]) {
      pop(@dir);
    }
    $self->{volume} ||= $vol;
    $self->{records}{unicodeVolume} ||=
      pack('na*', length($vol), encode('utf-16be', $vol));

    $self->{file} ||= $file;
    $self->{records}{parentDir} ||= $dir[-1]
      if(@dir);
    $self->{records}{absolutePath} ||= $path;
    $self->{records}{volumePath} ||= File::Spec->catfile('', @dir, $file);
    $self->{records}{unicodeFile} ||=
      pack('na*', length($file), encode('utf-16be', $file));
  }
  return(bless($self, ref($class) || $class));
}

sub toFSTime {
  my $val = shift;
  if(ref($val) && $val->isa("DateTime")) {
    $val = $val->epoch - DateTime->new(year => 1904)->epoch();
  }
  return($val);
}

sub write {
  my ($self, $out) = @_;

  my $aliasType = $self->{aliasType};
  $aliasType = (($aliasType =~ /^d(ir(ectory)?)?$/i && 1) ||
                ($aliasType !~ /^f(ile)?$/ && $aliasType) || 0);

  my $volumeCreated = toFSTime($self->{volumeCreated});
  my $volumeFS = $self->{volumeFS};
  if(ref($volumeFS) ne 'ARRAY') {
    $volumeFS = $FSEncodings{$volumeFS} || ['', ''];
  }

  my $volumeDisk = $self->{volumeDisk};
  if(!defined($volumeDisk)) {
    if($volumeFS->[0] eq 'H+') {
      $volumeDisk = 'Floppy';
    }
    elsif($volumeFS->[0]) {
      $volumeDisk = 'HD';
    }
    else {
      $volumeDisk = 'Other';
    }
  }
  $volumeDisk = (exists($DiskEncodings{$volumeDisk})
                 ? $DiskEncodings{$volumeDisk}
                 : $volumeDisk);

  my $fileCreated = toFSTime($self->{fileCreated});

  my $buf =
    pack('nn (C/a @28)Na2n N(C/a @64)NNa4a4 n!n!Na2 x10 (n!n/ax!2)*',
         $self->{aliasVersion}, $aliasType,
         $self->{volume}, $volumeCreated, $volumeFS->[0], $volumeDisk,
         $self->{directoryID}, $self->{file}, $self->{fileID}, $fileCreated,
         $self->{fileType}, $self->{fileCreator}, $self->{nlvlFrom},
         $self->{nlvlTo}, $self->{volumeAttrs}, $volumeFS->[1],
         map(((exists($RecordEncodings{$_}) ? $RecordEncodings{$_} : $_)
              => $self->{records}{$_}),
             keys(%{$self->{records}})),
         (-1, ''));
  $buf = pack('a4n', $self->{aliasCreator}, length($buf) + 6) . $buf;

  if(!$out) {
    return($buf);
  }
  elsif(ref($out) eq 'GLOB') {
    print $out $buf;
  }
  else {
    open(my $outfh, '>', $out) || die;
    print $outfh $buf;
  }
}

1;
