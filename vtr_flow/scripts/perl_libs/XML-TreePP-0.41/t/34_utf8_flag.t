# ----------------------------------------------------------------
#   this test script is written in utf8 but does not "use utf8" for 5.005-compatibility
# ----------------------------------------------------------------
    use strict;
    use Test::More;
# ----------------------------------------------------------------
{
    local $@;
    eval { require 5.008001; };
    plan skip_all => 'Perl 5.8.1 is required.' if $@;
}
# ----------------------------------------------------------------
{
    plan tests => 66;
    use_ok('XML::TreePP');
    &test_utf8();
}
# ----------------------------------------------------------------
sub test_utf8 {

    my $octxml = <<"EOT";
<root>
    <one>一</one>
    <two>二２</two>
    <three>三３参</three>
    <four>四４Ⅳⅳ</four>
    <five>5</five>
    <six>±6÷6</six>
</root>
EOT

    my $strxml = $octxml;
    utf8::decode( $strxml );

    my $strtpp = XML::TreePP->new( utf8_flag => 1 );
    my $octtpp = XML::TreePP->new();

    ok( ! utf8::is_utf8($octxml), '[source] XML: octets' );
    ok(   utf8::is_utf8($strxml), '[source] XML: string' );

    my $treeA = $strtpp->parse( $octxml );
    my $treeB = $strtpp->parse( $strxml );
    my $treeC = $octtpp->parse( $octxml );
    my $treeD = $octtpp->parse( $strxml );

    ok( ! utf8::is_utf8($octxml), "[source] XML: octets (no damaged)" );
    ok(   utf8::is_utf8($strxml), "[source] XML: string (no damaged)" );

    &check_string( 'A', $treeA );
    &check_string( 'B', $treeB );
    &check_octest( 'C', $treeC );
    &check_string( 'D', $treeD );

    &check_same( 'A-B', $treeA, $treeB );
    &check_same( 'B-D', $treeB, $treeB );
    &check_diff( 'A-C', $treeA, $treeC );

    foreach my $hash ( $treeA, $treeB, $treeD ) {
        my $root = $hash->{root};
        foreach my $key ( sort keys %$root ) {
            ok( utf8::is_utf8($root->{$key}), 'XML: string '.$key );
        }
    }

    foreach my $hash ( $treeC ) {
        my $root = $hash->{root};
        foreach my $key ( sort keys %$root ) {
            ok( ! utf8::is_utf8($root->{$key}), 'XML: octets '.$key );
        }
    }

    my $xmlH = $octtpp->write( $treeC );
    my $xmlE = $strtpp->write( $treeA );
    my $xmlF = $strtpp->write( $treeB );
    my $xmlG = $octtpp->write( $treeD );

    ok(   utf8::is_utf8($xmlE), '[E] XML: string' );
    ok(   utf8::is_utf8($xmlF), '[F] XML: string' );
    ok(   utf8::is_utf8($xmlG), '[G] XML: string' );
    ok( ! utf8::is_utf8($xmlH), '[H] XML: octets' );
}
# ----------------------------------------------------------------
sub check_string {
    my $name = shift;
    my $tree = shift;

    my $oct1 = '一';
    my $oct2 = "二２";
    my $str2 = $oct2;
    utf8::decode( $str2 );

    my $four = $tree->{root}->{four};
    ok( utf8::is_utf8($four), "[$name] 4: string" );

    my $five = $tree->{root}->{five};
    ok( utf8::is_utf8($five), "[$name] 5: string" );

    my $six = $tree->{root}->{six};
    ok( utf8::is_utf8($six), "[$name] 6: string" );

    my $one = "".$tree->{root}->{one};
    isnt( $one, $oct1, "[$name] 1: string != octets" );
    utf8::encode( $one );
    is( $one, $oct1, "[$name] 2: octets == octets" );

    my $two = "".$tree->{root}->{two};
    isnt( $two, $oct2, "[$name] 3: string != octets" );
    is( $two, $str2, "[$name] 4: string == string" );
}
# ----------------------------------------------------------------
sub check_octest {
    my $name = shift;
    my $tree = shift;

    my $oct1 = '一';
    my $oct2 = "二２";
    my $str2 = $oct2;
    utf8::decode( $str2 );

    my $four = $tree->{root}->{four};
    ok( ! utf8::is_utf8($four), "[$name] 4: octets" );

    my $five = $tree->{root}->{five};
    ok( ! utf8::is_utf8($five), "[$name] 5: octets" );

    my $six = $tree->{root}->{six};
    ok( ! utf8::is_utf8($six), "[$name] 6: octets" );

    my $one = $tree->{root}->{one};
    is( $one, $oct1, "[$name] 1: octets == octets" );

    my $two = "".$tree->{root}->{two};
    isnt( $two, $str2, "[$name] 2: octets != string" );
    utf8::decode( $two );
    is( $two, $str2, "[$name] 2: string == string" );
}
# ----------------------------------------------------------------
sub check_same {
    my $name = shift;
    my $tree1 = shift;
    my $tree2 = shift;

    my $three1 = $tree1->{root}->{three};
    my $three2 = $tree2->{root}->{three};
    is( $three1, $three2, "[$name] 4: same" );

#   octets' latin-1 and string's latin-1 are equal
#   my $five1 = $tree1->{root}->{five};
#   my $five2 = $tree2->{root}->{five};
#   is( $five1, $five2, "[$name] 5: same" );

    my $six1 = $tree1->{root}->{six};
    my $six2 = $tree2->{root}->{six};
    is( $six1, $six2, "[$name] 6: same" );
}
# ----------------------------------------------------------------
sub check_diff {
    my $name = shift;
    my $tree1 = shift;
    my $tree2 = shift;

    my $three1 = $tree1->{root}->{three};
    my $three2 = $tree2->{root}->{three};
    isnt( $three1, $three2, "[$name] 4: diff" );

#   octets' latin-1 and string's latin-1 are equal
#   my $five1 = $tree1->{root}->{five};
#   my $five2 = $tree2->{root}->{five};
#   isnt( $five1, $five2, "[$name] 5: diff" );

    my $six1 = $tree1->{root}->{six};
    my $six2 = $tree2->{root}->{six};
    isnt( $six1, $six2, "[$name] 6: diff" );
}
# ----------------------------------------------------------------
;1;
# ----------------------------------------------------------------
