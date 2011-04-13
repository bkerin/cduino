#!/usr/bin/perl -w

# Allow specification of fuse and lock bit setting using slightly more human
# readable (or at least search-forable) names (as opposed to the raw byte
# values the avrdude accepts).
#
# The first argument must be the name of processor being targeted in the form
# understood by avrdude (for a recent Arduino Duemilanove this will be 'm328p'
# unless the chip has been changed out).
#
# Any additional arguments should be a strings containing comma-seperated
# substrings of the form bit_name=value (where bit_name is one of the bit names
# from section 27.1 or 27.2 of the ATMega 328P datasheet).
#
# The output consists of avrdude -U options (and their arguments) to control
# avrdude such that:
#
#   * The bits mentioned as arguments are set as indicated.
#
#   * The unmentioned bits of bytes containing mentioned bits are set to their
#     default values.
#
#   * The bits of bytes containing no mentioned bits are unchanged.
#
# For example:
#
#   $ ./lock_and_fuse_bits_to_avrdude_options.perl -- m328p LB2=0 LB1=0
#   -U lock:w:0xFC:m 
#
#   $ ./lock_and_fuse_bits_to_avrdude_options.perl -- m328p LB2=0 LB1=0 DWEN=0
#   -U lock:w:0xFC:m -U hfuse:w:0x99:m
#
# If the --reverse option is given, this program instead takes the processor
# name and one or more argument like for example "-U efuse:w:0x05:m", and
# prints out the bit settngs represented by the hexadecimal values, one per
# line.  Note that the default bit values for bytes for which no corresponding
# -U option is supplied aren't printed.  Note also that when hexadecimal values
# which would set unused bits are encountered, the output doesn't capture this
# fact, possibly resulting in a (harmless) asymmetry if the output if fed back
# into a forward run of this program.  NOTE: at present we generate avrdude
# options that set unused bits to zero, since avrdude seems to expect this.
# The datasheets seem to expect one, but it apparently doesn't matter.  So at
# the moment, its feeding one values for unused bits into a reverse run that
# results in asymmetrical behavior if fed back to a forward run.
#
# For example:
#
#   $ ./lock_and_fuse_bits_to_avrdude_options.perl --reverse -- m328p \
#       -U efuse:w:0xFE:m
#   BODLEVEL0=0
#   BODLEVEL1=1
#   BODLEVEL2=1


use strict;

use Data::Dumper;
use Getopt::Long;
use POSIX qw( floor );

my $reverse = 0;
GetOptions('reverse' => \$reverse) or die "error parsing options";

@ARGV >= 1 or die "wrong number of arguments";

my $chip_model = shift(@ARGV);

# Data structure form of the datasheet tables that describe the layout of the
# bits in the lock and fuse bytes, and their default values.
my %bit_descriptions;

# FIXME: the next step would be to have some code words that would set
# associated groups of bits so you would actually have a clue from the call of
# this program what the fuse settings are specifying.

if ( $chip_model eq 'm328p' ) {
    %bit_descriptions = ( 
        lock_bits_byte => {
            BLB12 => { position => 5, default => 1 },
            BLB11 => { position => 4, default => 1 },
            BLB02 => { position => 3, default => 1 },
            BLB01 => { position => 2, default => 1 },
            LB2 => { position => 1, default => 1 },
            LB1 => { position => 0, default => 1 } },
        extended_fuse_byte => {
            BODLEVEL2 => { position => 2, default => 1 },
            BODLEVEL1 => { position => 1, default => 1 },
            BODLEVEL0 => { position => 0, default => 1 } },
        high_fuse_byte => {
            RSTDISBL => { position => 7, default => 1 },
            DWEN => { position => 6, default => 1 },
            SPIEN => { position => 5, default => 0 },
            WDTON => { position => 4, default => 1 },
            EESAVE => { position => 3, default => 1 },
            BOOTSZ1 => { position => 2, default => 0 },
            BOOTSZ0 => { position => 1, default => 0 },
            BOOTRST => { position => 0, default => 1 } },
        low_fuse_byte => {
            CKDIV8 => { position => 7, default => 0 },
            CKOUT => { position => 6, default => 1 },
            SUT1 => { position => 5, default => 1 },
            SUT0 => { position => 4, default => 0 },
            CKSEL3 => { position => 3, default => 0 },
            CKSEL2 => { position => 2, default => 0 },
            CKSEL1 => { position => 1, default => 1 },
            CKSEL0 => { position => 0, default => 0 } } );
}
else {
    die "wrong (or so far unknown) chip model '$chip_model'";
}

# Given a bit description hash reference and a bit position, return the bit
# name, or the string "unused position".
sub bit_position_to_name {
    @_ == 2 or die "wrong number of arguments";
    my ($bd, $pos) = @_;   # Bit description hash ref, bit position

    foreach ( keys(%{$bd}) ) {
        if ( $bd->{$_}{position} == $pos ) {
            return $_;
        }
    }

    return "unused position";
}

# NOTE: the non-reverse direction of this program is more clearly commented,
# this section just goes in the other direction.
if ( $reverse ) {

    # This allows either seperate argument strings or single strings.
    my @aos = map { split; } @ARGV;   # avrdude option strings

    foreach ( @aos ) {
        not m/-U/ or next;
        m/^(lock|efuse|hfuse|lfuse):w:0x([0-9A-Za-z]{2}):m$/
            or die "expected an avrdude -U option argument string, got '$_'";
        my ($abn, $bv) = ($1, $2);   # avrdude byte name, byte value.

        my $obn;   # Our byte name (as knonw in $bit_descriptions).
        if ( $abn eq 'lock' ) {
            $obn = "lock_bits_byte";
        }
        elsif ( $abn eq 'efuse' ) {
            $obn = "extended_fuse_byte";
        }
        elsif ( $abn eq 'hfuse' ) {
            $obn = "high_fuse_byte";
        }
        elsif ( $abn eq 'lfuse' ) {
            $obn = "low_fuse_byte";
        }
        else {
            die "unrecognized avrdude byte name '$abn'";
        }

        my $bd = $bit_descriptions{$obn};   # Bit descriptions for this byte

        # Get the byte value in decimal.
        my $bvd = 0; 
        for ( my $ii = 0 ; $ii < 2 ; $ii++ ) {
            my $nc = substr($bv, $ii, 1);   # Nibble value as hex character.
            my $cpdnv;   # Current position decimal nibble value.
            if ( $nc =~ /^[0-9]$/ ) {
                $cpdnv = $nc;
            }
            else {
                if ( $nc =~ m/[aA]/ ) {
                    $cpdnv = 10;
                }
                if ( $nc =~ m/[bB]/ ) {
                    $cpdnv = 11;
                }
                if ( $nc =~ m/[cC]/ ) {
                    $cpdnv = 12;
                }
                if ( $nc =~ m/[dD]/ ) {
                    $cpdnv = 13;
                }
                if ( $nc =~ m/[eE]/ ) {
                    $cpdnv = 14;
                }
                if ( $nc =~ m/[fF]/ ) {
                    $cpdnv = 15;
                }
            }
            if ( not defined($cpdnv) ) {
                print "\nnc: $nc\n";
            }

            $bvd += $cpdnv * 16 ** (1 - $ii);
        }

        # Compute and print out the bit names and values.
        for ( my $ii = 0 ; $ii < 8 ; $ii++ ) {
            my $cbit = $bvd % 2;
            my $bn = bit_position_to_name($bd, $ii);
            print ($bn ne "unused position" ? "$bn=$cbit\n" : "");
            $bvd = floor($bvd / 2.0);
        }
    }

    exit 0;
}

my @bit_settings_strings = @ARGV;

# Byte lookup table (map from bit names back to their containing bytes).
my %blt 
  = map { 
      my $byte = $_;
      map { 
        ($_ => $byte)
      } 
      keys(%{$bit_descriptions{$_}}) => $_
    } 
    keys(%bit_descriptions);

my @bit_settings;
foreach  ( @bit_settings_strings ) {
    push(@bit_settings, split(/\s*,\s*/, $_));
}

# Values of the bytes we're going to set.
my %byte_values = map { $_ => 0 } keys(%bit_descriptions);

# Keep track of explicitly set stuff so we can go through and apply defaults
# and 1 values for unused bits later.
my %ess;

# Add in the explicitly requested bit settings, and remember that we've done so.
foreach ( @bit_settings ) {
    m/([A-Z0-9]+)=([01])/ or die "malformed bit name setting string '$_'";
    my ($bit_name, $bit_value) = ($1, $2);
    exists($blt{$bit_name}) or die "unknown bit name '$bit_name'";
    my $byte_name = $blt{$bit_name};
    if ( $bit_value ) {
        $byte_values{$byte_name}
            += 2 ** $bit_descriptions{$byte_name}{$bit_name}{position};
    }
    $ess{$byte_name}{$bit_name} = 1;
}

# Add in the defaults for the unmentioned bit fields of bytes where at least
# some bit is getting set explicitly.
foreach my $byte_name ( keys(%byte_values) ) {
    exists($ess{$byte_name}) or next;
    foreach my $bit_name ( keys(%{$bit_descriptions{$byte_name}}) ) {
        if ( not exists($ess{$byte_name}{$bit_name}) ) {
            my $bit_desc = $bit_descriptions{$byte_name}{$bit_name};
            defined($bit_desc->{position}) or die "undef position";
            defined($bit_desc->{default}) or die "undef default";
            $byte_values{$byte_name}
                += $bit_desc->{default} * 2 ** $bit_desc->{position}; 
        }
    }
} 

# NOTE: for now we put in zero values for unused bits, to keep avrdude happy 
# (since that seems to be what it expects to see when it does its verification)
# The datasheets for the parts seem to think it should be one, but setting them
# to zero presumably doesn't cause any problems since avrdude expects it.
# Add in the values that are used for unused bit fields of bytes where at
# least one bit is getting set explicitly.
foreach my $byte_name ( keys(%byte_values) ) {
    exists($ess{$byte_name}) or next;
    my $cbyted = $bit_descriptions{$byte_name};
    my %unused_positions = map { $_ => 1 } (0 .. 7);
    foreach my $cbitd ( values(%{$cbyted}) ) {
        delete($unused_positions{$cbitd->{position}});
    }
    foreach ( keys(%unused_positions) ) {
        # NOTE: change this 0 to 1 make unused bit fields 1 (see note above).
        $byte_values{$byte_name} += 0 * 2 ** $_;
    }
}

if ( exists($ess{lock_bits_byte}) ) {
    print '-U lock:w:0x'.uc(sprintf('%02x', $byte_values{lock_bits_byte})).':m';
    print ' ';
}
if ( exists($ess{low_fuse_byte}) ) {
    print '-U lfuse:w:0x'.uc(sprintf('%02x', $byte_values{low_fuse_byte})).':m';
    print ' ';
}
if ( exists($ess{high_fuse_byte}) ) {
    print '-U hfuse:w:0x'.uc(sprintf('%02x', $byte_values{high_fuse_byte}));
    print ':m';
    print ' ';
}

if ( exists($ess{extended_fuse_byte}) ) {
    print '-U efuse:w:0x'.uc(sprintf('%02x', $byte_values{extended_fuse_byte}));
    print ':m';
    print ' ';
}

print "\n";

exit 0;

