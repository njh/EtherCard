#!/usr/bin/env perl

use strict;

my @examples = split(/\n/, `git ls-files ../examples/*/*.ino`);
foreach my $example (@examples) {
    $example =~ s|\.\./examples/\w+/||i;
    print "/** \@example $example */\n";
    print "/** \@file $example */\n";
}
