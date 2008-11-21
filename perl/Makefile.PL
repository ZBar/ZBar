use 5.006;
use ExtUtils::MakeMaker;

$ENV{srcdir} ||= '..';

WriteMakefile(
    NAME              => 'Zebra',
    VERSION_FROM      => "$ENV{srcdir}/perl/Zebra.pm",
    ABSTRACT_FROM     => "$ENV{srcdir}/perl/Zebra.pm",
    AUTHOR            => 'Jeff Brown <spadix@users.sourceforge.net>',
    LIBS              => ['-lzebra '],
);
