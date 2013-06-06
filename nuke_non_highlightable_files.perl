#!/usr/bin/perl -w

# We end up with some files in our directory of things that we want to
# highlight that actually cause a bunch of trouble when we hightlight them
# (stupid double links due to multiple possible definitions, etc.), so we
# use this script to get rid of them.  These mostly come from the Arduino
# libs which we often having lying around during adaptation.

use strict;

@ARGV == 1 or die "wrong number of arguments";

my $xld = $ARGV[0];   # xlinked sources dir

my @nuke_pats = ( '[A-Z]',
                  'binary\.h',
                  'wiring.*\.[ch]',
                  'new\.h',
                  'pins_arduino\.h',
                  'sd_card_info\.h' );

chdir($xld) or die "cd to '$xld' failed: $!";

opendir(XLD, ".") or die;

sub matches_a_nuke_pattern
{
    # Remove file named arg2 if it matches any of the (anchored) pattens in
    # the array referenced by arg2.

    @_ == 2 or die "wrong number of arguments";
    my ($file, $nps) = @_;   # File, nuke patterns

    foreach ( @{$nps} ) {
        if ( $file =~ m/^$_/ ) {
            return 1;
        }
    } 

    return 0;
}

while ( my $cf = readdir(XLD) ) {
    if ( matches_a_nuke_pattern($cf, \@nuke_pats) ) {
        unlink($cf) == 1 or die "unlink failed on '$cf': $!";
    }
}

closedir(XLD) or die "close failed on '$xld': $!";

exit 0;
