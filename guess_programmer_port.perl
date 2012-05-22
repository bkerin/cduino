#!/usr/bin/perl -w

# Use some black art knowledge of the device files and lsusb entries that
# the arduinos create to guess what device value (port is a misnomer now
# I think) we should use for the avrdude device option and such.

use warnings;
use strict;

my $fr = not system("file /dev/ttyACM0 >/dev/null");
my $lr = not system("lsusb -d :0043 >/dev/null");
if ( $fr and $lr ) { print "/dev/ttyACM0\n"; exit 0; }
$fr = not system("file /dev/ttyUSB0 >/dev/null");
$lr = not system("lsusb -d :6001 >/dev/null");
if ( $fr and $lr ) { print "/dev/ttyUSB0\n"; exit 0; }
die "failed to guess value for avrdude programmer port";
