use 5.006;
use ExtUtils::MakeMaker;

use lib qw(inc);
use Devel::CheckLib;

check_lib_or_exit(
    lib    => 'zbar',
    header => 'zbar.h',
    LIBS   => join(' ', map({ /^LIBS=(.*)/; $1 } grep(/^LIBS=/, @ARGV))),
    INC    => join(' ', map({ /^INC=(.*)/; $1 }  grep(/^INC=/, @ARGV))),
);

WriteMakefile(
    NAME              => 'Barcode::ZBar',
    VERSION_FROM      => "ZBar.pm",
    ABSTRACT_FROM     => "ZBar.pm",
    AUTHOR            => 'Jeff Brown <spadix@users.sourceforge.net>',
    LICENSE           => 'lgpl',
    LIBS              => ['-lzbar'],
    META_MERGE        => {
        build_requires => {
            'Test::More'    => 0,
        }
    },
);
