#!/usr/bin/perl

# Quick hack script to generate the .DS_Store for the DMG, which
#   * allows us to precisely position the window and icons
#   * is more usefully versioned
#   * avoids references to my local HD(!?)

use warnings;
use strict;

BEGIN {
  use File::Spec::Functions qw(rel2abs splitpath);
  use lib (splitpath(rel2abs($0)))[1];
}

use Data::Plist::BinaryWriter;
use Mac::Finder::DSStore qw(writeDSDBEntries makeEntries);
use Mac::Finder::AliasRecord;

$Mac::Finder::DSStore::Entry::types{bwsp} = 'blob';
$Mac::Finder::DSStore::Entry::types{icvp} = 'blob';

writeDSDBEntries($ARGV[0] || "DS_Store",
    makeEntries(".",
        bwsp => Data::Plist::BinaryWriter->new(serialize => 0)->write([
            dict => {
                WindowBounds => [
                    string => sprintf('{{%d, %d}, {%d, %d}}',
                                      512, 128, 512, 608 + 22)
                ],
                SidebarWidth => [integer => 0],
                ShowToolbar => [false => 0],
                ShowSidebar => [false => 0],
                ShowPathbar => [false => 0],
                ShowStatusBar => [false => 0],
            }
        ]),
        icvp => Data::Plist::BinaryWriter->new(serialize => 0)->write([
            dict => {
                viewOptionsVersion => [integer => 0],
                arrangeBy => [string => "none"],
                iconSize => [real => 64],
                textSize => [real => 12],
                labelOnBottom => [true => 1],
                gridSpacing => [real => 100],
                gridOffsetX => [real => 0],
                gridOffsetY => [real => 0],
                showItemInfo => [false => 0],
                showIconPreview => [false => 0],
                backgroundType => [integer => 2],
                backgroundColorRed => [real => 0],
                backgroundColorGreen => [real => 0],
                backgroundColorBlue => [real => .5],
                backgroundImageAlias => [
                    data => Mac::Finder::AliasRecord->new(
                        path => 'ZBarSDK:.background:ZBarSDK-bg.png',
                        volumeFS => 'HFS+')->write()
                ],
            },
        ]),
        vstl => "icnv",
    ),
    makeEntries("README",       Iloc_xy => [ 4.5 * 32,  2.5  * 32 ]),
    makeEntries("ZBarSDK",      Iloc_xy => [ 4.5 * 32,  7.5  * 32 ]),
    makeEntries("ChangeLog",    Iloc_xy => [  4  * 32, 12.5  * 32 ]),
    makeEntries("Documentation.html",
                                Iloc_xy => [  8  * 32, 12.5  * 32 ]),
    makeEntries("Examples",     Iloc_xy => [ 12  * 32, 12.5  * 32 ]),
    makeEntries("COPYING",      Iloc_xy => [  4  * 32,  16   * 32 ]),
    makeEntries("LICENSE",      Iloc_xy => [  8  * 32,  16   * 32 ]),
    makeEntries("Documentation",Iloc_xy => [ 12  * 32,  16   * 32 ]),
);
