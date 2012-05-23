# ----------------------------------------------------------------
    use strict;
    use Test::More tests => 9;
    BEGIN { use_ok('XML::TreePP') };
# ----------------------------------------------------------------
    my $tpp = XML::TreePP->new();
    my $space0   = '   ';
    my $space1   = '&#32;&#x20; ';
    my $entref0  = '&amp;quot;&amp;apos;&amp;lt;&amp;gt;&amp;amp;';
    my $entref1  = '&quot;&apos;&lt;&gt;&amp;';
    my $charref0 = "\x21\x22";
    my $charref1 = '&#33;&#x22;';
    my $invalid0 = '&amp;#32&amp;#x20&amp;#foo;&amp;#ZZ&amp;#&amp;32;&amp;x20;&amp;bar';
    my $invalid1 = '&#32&#x20&#foo;&#ZZ&#&32;&x20;&bar';
    my $tree = {
        root    =>  {
            space   =>  $space1,
            entref  =>  $entref1,
            charref =>  $charref1,
            invalid =>  $invalid1,
        },
    };
    my $write = $tpp->write( $tree );

    my $space2   = ( $write =~ m#<space>(.*)</space># )[0];         # through
    is( $space2,   $space1, 'write space' );
    my $entref2  = ( $write =~ m#<entref>(.*)</entref># )[0];       # escaped
    is( $entref2,  $entref0, 'write entref' );
    my $charref2 = ( $write =~ m#<charref>(.*)</charref># )[0];     # through
    is( $charref2, $charref1, 'write charref' );
    my $invalid2 = ( $write =~ m#<invalid>(.*)</invalid># )[0];     # escaped
    is( $invalid2, $invalid0, 'write invalid' );

    my $parse = $tpp->parse( $write );
    is( $parse->{root}->{space},   $space0,   'write space' );      # unescaped
    is( $parse->{root}->{entref},  $entref1,  'write entref' );     # unescaped
    is( $parse->{root}->{charref}, $charref0, 'write charref' );    # unescaped
    is( $parse->{root}->{invalid}, $invalid1, 'write invalid' );
# ----------------------------------------------------------------
;1;
# ----------------------------------------------------------------
