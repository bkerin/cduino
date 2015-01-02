#!/usr/bin/perl -w

# The source-highlight program doesn't understand syntax or our module
# layout scheme, so it sometimes adds a bunch of extra reference targets.
# This script goes through and scrapes them back out.

# vim:foldmethod=marker

use strict;
use warnings FATAL => 'all';

use Data::Dumper;
use Fcntl qw( SEEK_SET );

# X-Ref RegeX as it appears in source-highlight-generated HTML.  Note that
# this has a capture paren set embedded in it.
my $xrgx = '>\s*(\w+)\s*->\s*(\w+\.\w+)\s*:\s*(\d+)\s*<';

# Tag Sequence regex.  Matches one or more HTML tags in a row.
my $ts = '(?:(?:<[^<>]+>)+)';

sub disambiguated_chunk ($$) # {{{1
{
    # Given a chunk of HTML consisting of a code line followed by some
    # reference lines as produced by source-highlight, try to resolve or in
    # some cases just remove the ambiguous references, and put the result into
    # a single in-line reference.  Return the hopefully disambiguated lines.
    # This is where the real intelligence in this module lives (such as
    # it is).  The horrible thing is we have to recognize the code features
    # of interest in their HTML marked-up form.  Its not hard but somewhat
    # painful and ugly.  A lot of these are just hard-coded cases where we
    # know from our knowledge of the code what's going on.

    # Source File Name, Html Chunk Lines
    my ($sfn, $hcls) = @_;

    # Html Code Line (this is the one with the actual C code).
    my $hcl = shift(@$hcls);

    # Re-create the original C code line by removing all HTML tags.
    my $ccl = $hcl;
    $ccl =~ s/$ts//g;

    my @refs = @$hcls;

    my @nr = ();   # New (filtered) Refs

    # Filter out refs that can be ruled out from an examination of the
    # individual reference-target pair itself (without comparing it to other
    # targets for the same ref).

    # Filter refs that can be see bad by intrinsic characteristics # {{{2
    foreach ( @refs ) {

        m/$xrgx/;
        my $rn = $1;   # Reference Name (actual var, macro, etc. name)
        my $rt = $2;   # Reference Target (file)

        # FIXXME: well, I guess it would be nice to nuke even non-ambiguous
        # references in this case, but things would have to change elsewhere...
        # If the reference appears to be a function definition parameter
        # name...
        if ( $ccl =~ m/^\w+\s*\(.*$rn\s*(?=,|\)).*\)\s*$/ ) {
            # ...don't treat it as a reference at all.
            ;
        }

        elsif ( $sfn =~ m/one_wire_master\.c\.html$/ ) {
            if ( $rt eq 'sd_card_private.h' ) {
                ;
            }
            elsif ( $rn eq 'rom_id' and $rt eq 'one_wire_slave.c' ) {
                ;
            }
            else {
                push(@nr, $_);
            }
        }

        elsif ( $sfn =~ m/one_wire_slave\.c\.html$/ ) {
            if ( $rt eq 'sd_card_private.h' ) {
                ;
            }
            elsif ( $rn eq 'rom_id' and $rt eq 'one_wire_master.c' ) {
                ;
            }
            else {
                push(@nr, $_);
            }
        }

        else {
            push(@nr, $_);
        }
    } # }}}2

    my %rbn = ();   # References By Name (hash of array refs)
    foreach ( @nr ) {
        m/$xrgx/;
        my $rn = $1;   # Reference Name (actual var, macro, etc. name)
        $rbn{$rn} ||= [ ];
        push(@{$rbn{$rn}}, $_);
    }

    # Filter refs by considering their relative merrits # {{{2
    foreach my $kw ( keys %rbn ) {

        my @rftk = @{$rbn{$kw}};   # Refs For This Keyword

        # Parse each reference line and save in Reference Descriptions.
        my @rd = ();
        foreach ( @rftk ) {
            m/$xrgx/;
            my $rn = $1;   # Reference Name (actual var, macro, etc. name)
            $rn eq $kw or die "bug";   # Name is the same for all these refs
            my $rt = $2;   # Reference Target (file)
            push(@rd, { rhtml => $_, name => $1, target => $2, line => $3 });
        }

        # All caps means macro, and if we redefine a macro in a file we mean
        # it to apply for entire file, so the local definition should be
        # used as the reference target.  Because the first reference found
        # is usually the undef for the macro, we want to use the one with
        # the higher line number.  X macros might be an exception but are
        # handled specially elsewhere anyway.
        my $ldt = undef;   # Local Definition Reference
        if ( $kw =~ m/^[A-Z0-9_]{2,}$/ ) {
            my $hlrlnsf = -1;   # Highest Local Reference Line Number So Far
            my $rtu = undef;    # Reference To Use
            foreach ( @rd ) {
                if ( $sfn =~ m/\Q$_->{target}\E\.html$/ ) {
                    if ( $_->{line} > $hlrlnsf ) {
                        $hlrlnsf = $_->{line};
                        $rtu = $_;
                    }
                }
            }
            if ( defined($rtu) ) {
                $rbn{$kw} = [ $rtu->{rhtml} ];
            }
        }

    } # }}}2

    # Replace now-non-ambiguous post-line references with inline references
    foreach ( keys %rbn ) {
        if ( @{$rbn{$_}} == 1 ) {
            my $frl = $rbn{$_}[0];   # Full Reference Line
            $frl =~ m/$xrgx/;
            my $rn = $1;   # Reference Name (actual var, macro, etc. name)
            my $mrl = $frl;   # Modified Reference Line (for inline use)
            $mrl =~ s/\s*->\s*\w+\.\w+:\d+//;
            $mrl =~ s/\n$//;
            $hcl =~ s/$rn/$mrl/g;
            delete($rbn{$_});
        }
    }

    # Return the new HTMLized code line and any remaining HTML reference lines.
    return ($hcl, map { (@$_) } values(%rbn));

} # }}}1

sub disambiguate_crossrefs_in_file ($) # {{{1
{
    # Disambiguate cross-references in a given file.

    my ($ftm) = @_;   # File To Modify

    open(FTM, "+<$ftm") or die "couldn't open file '$ftm' for read/write: $!";

    my @oc = <FTM>;   # Old Contents
    my @nc = ();      # New Contents

    my @cwax = ();   # Chunk With Ambiguous X-ref(s)
    my $pcl = undef;   # Previous Code Line (containing the actual code ref)
    foreach ( @oc ) {
        if ( m/>\s*\w+\s*->\s*(\w+\.\w+):\d+</ ) {
            if ( defined($pcl) ) {
                push(@cwax, $pcl);
                pop(@nc);
                $pcl = undef;
            }
            push(@cwax, $_);
        }
        else {
            if ( @cwax ) {
                push(@nc, disambiguated_chunk($ftm, [ @cwax ]));
            }
            @cwax = ();
            push(@nc, $_);
            $pcl = $_;
        }
    }

    truncate(FTM, 0) or die;
    seek(FTM, 0, SEEK_SET) or die;
    print FTM @nc or die;
    close(FTM) or die;

} # }}}1

my $xsd = "xlinked_source_html";   # X-linked Source Directory

opendir(XSD, $xsd) or die;

foreach ( readdir(XSD) ) {
    m/.*\.[ch]\.html/ or next;
    disambiguate_crossrefs_in_file("$xsd/$_");
}

closedir(XSD) or die;

exit 0;
