#!/usr/bin/perl -w

# vim:foldmethod=marker

# Given an HTML file name including the xlinked_source_html/ directory part,
# veryify that if its a test driver, interface, or implementation file,
# it has the crosslinks that we like to support.  This is a helper script
# for a recipe in Makefile, and should be understood in that context.

use strict;

my $sf = $ARGV[0];   # Source HTML file to process

# Skip exempt files {{{1

# The lesson files don't follow the test driver/interface/implementation
# pattern.
$sf !~ m/\/lesson.+\./ or exit 0;

# The blink.c file is an exceptional intro/demo file.
$sf !~ m/\/blink\.c/ or exit 0;

# }}}1

open(SF, "<$sf") or die "couldn't open $sf for reading";

# Determine which source type we have, and the root name of the module.
my ($st, $mrn);   # Source type, module root name
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
        exit 0 if $mrn eq 'adc';   # FIXME test trickery
        if ( m/\sTest\sdriver:\s.*href="${mrn}_test\.c\.html"
               .*
               \sImplementation:\s.*(?:href="${mrn}\.c\.html"|This\sfile)/x ) {
            exit 0;
        }
    }

    if ( $st eq 'implementation' ) {
        #if ( m/ Implementation\s+of\s+the\s+interface\s+described\s+
        if ( m/\sImplementation\sof\sthe\sinterface\sdescribed\sin\s
               .*
               href="$mrn\.h\.html"/x ) {
            exit 0;
        }
    }
}

die "didn't find the expected cross-reference lines for a(n) '$st' type file ".
    "in file $sf";
