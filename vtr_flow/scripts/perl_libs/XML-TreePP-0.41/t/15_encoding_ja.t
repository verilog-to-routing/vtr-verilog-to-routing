# ----------------------------------------------------------------
    use strict;
    use Test::More tests => 7;
# ----------------------------------------------------------------
    my $FILES = [qw(
        t/example/hello-ja-utf8.xml
        t/example/hello-ja-sjis.xml
        t/example/hello-ja-euc.xml
    )];
# ----------------------------------------------------------------
SKIP: {
    if ( $] < 5.008 ) {
        eval { require Jcode; } unless defined $Jcode::VERSION;
        if ( ! defined $Jcode::VERSION ) {
            skip( "Jcode.pm is not loaded.", 7 );
        }
    }
    use_ok('XML::TreePP');
    &test_main();
}
# ----------------------------------------------------------------
sub test_main {
    my $tpp = XML::TreePP->new();
    my $prev;
    foreach my $file ( @$FILES ) {
        my $tree = $tpp->parsefile( $file );
        if ( defined $prev ) {
            is( $tree->{root}->{text}, $prev, $file );
        } else {
            like( $tree->{root}->{text}, qr/\S\!/, $file );
            $prev = $tree->{root}->{text};
        }
        my $xml = $tpp->write( $tree );
        like( $xml, qr/^\s*<\?xml[^<>]+encoding="UTF-8"/is, "write encoding" );
    }
}
# ----------------------------------------------------------------
;1;
# ----------------------------------------------------------------
