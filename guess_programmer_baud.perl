#!/usr/bin/perl -w

# Use some black art knowledge of the device files and lsusb entries that
# the arduinos cause to exist to guess what baud rate we should tell avrdude
# to use.

use warnings;
use strict;

my $fr = not system("file /dev/ttyACM0 >/dev/null");
my $lr = not system("lsusb -d :0043 >/dev/null");
if ( $fr and $lr ) { print "115200\n"; exit 0; }
$fr = not system("file /dev/ttyUSB0 >/dev/null");
$lr = not system("lsusb -d :6001 >/dev/null");
if ( $fr and $lr ) { print "57600\n"; exit 0; }
die "failed to guess value for avrdude programmer baud rate";
