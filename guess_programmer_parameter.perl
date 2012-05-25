#!/usr/bin/perl -w

# Use some black art knowledge of the /dev files and lsusb entries that the
# Arduinos create to guess what device file (of the form /dev/ttyWHATEVER)
# or baud rate we should use for the connected device, if we can find
# exactly one candidate device.  If we can't find anything that looks like
# an Arduino, or we think we find more than one, print an informative error
# message and exit with a non-zero exit code.

# vim: foldmethod=marker

use warnings;
use strict;

use Getopt::Long;

my ($device, $baud) = (0, 0);  # We require an option saying what we want.

GetOptions("device" => \$device, "baud" => \$baud)
    or die "option parsing error";

($device xor $baud)
    or die "didn't get exactly one of --device or --baud options";

sub find_usb_tty_devices # {{{1
{
    # Given a USB vendor ID and product ID as arguments, return a list
    # of device tty files (e.g. /dev/ttyACM0) found on the USB.  Blows up
    # somewhere ugly for non-tty things, though that could be fixed maybe.

    @_ == 2 or die "wrong number of arguments";

    my ($vid, $pid) = @_;   # Vendor and product IDs

    my $finder_command
        = 'find -H /sys -name "idProduct" -exec '.
              'bash -c '.
                  '"grep --quiet -e '.$pid.' {} && '.
                       'echo {}" \;';
    
    my $id_files = `$finder_command`;
    $? == 0 or die "find command failed";
    
    my @ifl = split("\n", $id_files);   # ID file lines

    if ( @ifl == 0 ) {
        return ();
    }
    
    my @results = ();

    foreach ( @ifl ) { 
    
        my $dpd = `dirname $_`;   # Device path directory.
        $? == 0 or die "dirname command failed";
        chomp($dpd);

        # Discovered vendor ID for this device
        my $dvid = `cat $dpd/idVendor`;
        $? == 0 or die "cat command failed";
        $dvid eq $vid
            or (print "discovered vendor ID is not that of an Uno\n" and next);
        
        # The tty finder command.
        my $ttyfc = 'basename $(find '.$dpd.' -name "tty:*" -print) | '.
                        'perl -p -e \'s/tty:(.*)/$1/\'';
        
        my $df = '/dev/'.`$ttyfc`;
        $? == 0 or die "command '$ttyfc' failed";
        chomp($df);

        -e $df
            or die "though we found device '$df', but that file doesn't exist";

        push(@results, $df);
    }
    
    # Cross-check the number of Unos we found with what lsusb shows.  It would
    # be nice if we could just use lsusb for all this, but so far as I can
    # tell it doesn't give us any way to figure out which /dev file we want.
    # The udevinfo program doesn't quite do it either, so we end up groping
    # around in /dev ourselves.
    my $le = `lsusb -d $vid:$pid`;   # lsusb entries
    my @lel = split("\n", $le);   # lsusb entry lines
    scalar(@lel) == scalar(@results)
        or die "unexpected number of Uno entries in lsusb output";

    return @results;
} # }}}1

sub find_uno_rev3_devices # {{{1
{
    # Search for any Arduino Uno rev.3 devices and return list of device
    # file names (which can be empty) under /dev that refer to the devices.
    # This hopefully finds other Uno devices besides rev. 3 as well, but I
    # haven't had a chance to test it.

    my ($ur3vid, $ur3pid) = ('2341', '0043');   # Uno vendor and product IDs

    return find_usb_tty_devices($ur3vid, $ur3pid);
} # }}}1

sub find_duemilanove_devices # {{{1
{
    # Search for any Arduino Uno rev.3 devices and return list of device
    # file names (which can be empty) under /dev that refer to the devices.
    # This hopefully finds other Uno devices besides rev. 3 as well, but I
    # haven't had a chance to test it.

    # Duemilanove vendor and product IDs
    my ($duevid, $duepid) = ('0403', '6001');  

    return find_usb_tty_devices($duevid, $duepid);
} # }}}1

my @uno_rev3_devs = find_uno_rev3_devices();
my @duemilanove_devs = find_duemilanove_devices();

# For some message we have strings with one discovered device per line.
my $uno_rev3_devs_multiline = join("\n", @uno_rev3_devs);
my $duemilanove_devs_multiline = join("\n", @duemilanove_devs);

# The speeds that we believe are used by some different Arduino versions.
my ($uno_rev3_baud, $duemilanove_baud) = (115200, 57600);

if ( @uno_rev3_devs == 1 and @duemilanove_devs == 0 ) {
    if ( $device ) {
        print $uno_rev3_devs[0]."\n";
    }
    elsif ( $baud ) {
        print $uno_rev3_baud."\n";
    }
    else {
        die "shouldn't be here";
    }
}
elsif ( @uno_rev3_devs == 0 and @duemilanove_devs == 1 ) {
    if ( $device ) {
        print $duemilanove_devs[0]."\n";
    }
    elsif ( $baud ) {
        print $duemilanove_baud."\n";
    }
    else {
        die "shouldn't be here";
    }
}
elsif ( @uno_rev3_devs == 0 and @duemilanove_devs == 0 ) {
    # FIXME: Here we should put some informative messages for when nothing
    # is detected,
    die "didn't detect any Arduinos :(";
}
else {
    # FIXME: here we should discriminate more duemilanove case and give
    # useful messages
    print STDERR <<END_MULTIPLE_ARDUINO_MESSAGE;

ERROR: Mutiple things that look like Arduinos were detected, including:

$uno_rev3_devs_multiline
$duemilanove_devs_multiline

This script doesn't know how to decide which one you want.

END_MULTIPLE_ARDUINO_MESSAGE
    exit 1;
}

exit 0;
