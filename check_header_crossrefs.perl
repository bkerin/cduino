#!/usr/bin/perl -w

# vim:foldmethod=marker

# Given an HTML file name including the xlinked_source_html/ directory part,
# veryify that if it's a test driver, interface, or implementation file, it
# has the crosslinks that we like to support in the header.  This is a helper
# script for a recipe in Makefile, and should be understood in that context.

use strict;

my $sf = $ARGV[0];   # Source HTML file to process

# Skip exempt files {{{1

# The lesson files don't follow the test driver/interface/implementation
# pattern.
$sf !~ m/\/lesson.+\./ or exit 0;

# The blink.c file is an exceptional intro/demo file.
$sf !~ m/\/blink\.c/ or exit 0;

# The util.h file is an exceptional interface file.
$sf !~ m/\/util\.h/ or exit 0;

# Headers that aren't full-fledged interfaces:
$sf !~ m/ds18b20_commands\.h/ or exit 0;

# Headers and sources ending in _private (before the dot suffix)
$sf !~ m/_private\.[hc]/ or exit 0;

# This header isn't a full-fledged interface.
$sf !~ m/one_wire_common\.h/ or exit 0;

# This program is for a module that tests/demos generic.mk functionality, so
# doesn't have a header or implementation file and doesn't follow the pattern.
$sf !~ m/random_id_test\.c/ or exit 0;

# We have a stupid test program called main.c in a test module at the moment
# FIXME: try removing this exception to see if its gone, I think it was
# pcinttest/main.c or so.
$sf !~ m/main\.c/ or exit 0;

# }}}1

open(SF, "<$sf") or die "couldn't open $sf for reading";

# Determine which source type we have, and the root name of the module.
my ($st, $mrn);   # Source type, Module Root Name
if ( $sf =~ m/(\w+)_test\.c\.html$/ ) {
    $st = 'test driver';
    $mrn = $1;
}
elsif ( $sf =~ m/(\w+)\.h\.html$/ ) {
    $st = 'interface';
    $mrn = $1;
}
elsif ( $sf =~ m/(\w+)\.c\.html$/ ) {
    $st = 'implementation';
    $mrn = $1;
}

my @sfl = <SF>;   # Source file lines

foreach ( @sfl ) {
    if ( $st eq 'test driver' ) {
        # Require the interface file to be mentioned in our usual sentence.
        if ( m/\sTest\/demo\sfor\sthe\s
               .*
               href="$mrn\.h\.html".*\sinterface\./x ) {
            exit 0;
        }
    }

    if ( $st eq 'interface' ) {
        # Require the test driver and implementation (or this file message)
        # to be mentioned on the same line in this particular format.
        if ( m/\sTest\sdriver:\s.*href="${mrn}_test\.c\.html"
               .*
               \sImplementation:\s.*(?:href="${mrn}\.c\.html"|This\sfile)/x ) {
            exit 0;
        }
    }

    if ( $st eq 'implementation' ) {
        if ( m/\sImplementation\sof\sthe\sinterface\sdescribed\sin\s+
               .*
               href="$mrn\.h\.html"/x ) {
            exit 0;
        }
    }
}

die "didn't find the expected cross-reference lines for a(n) '$st' type file ".
    "in file '$sf', module root name '$mrn'";
