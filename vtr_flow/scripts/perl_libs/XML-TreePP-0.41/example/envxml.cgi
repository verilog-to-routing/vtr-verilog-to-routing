#!/usr/bin/env perl

use strict;
use XML::TreePP;

my $tree = { env => \%ENV };
my $tpp = XML::TreePP->new();

print "Content-Type: text/xml\n\n";
print $tpp->write( $tree ), "\n";
