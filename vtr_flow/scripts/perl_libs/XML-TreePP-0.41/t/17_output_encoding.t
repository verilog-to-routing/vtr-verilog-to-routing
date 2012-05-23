# ----------------------------------------------------------------
    use strict;
    use Test::More tests => 43;
    BEGIN { use_ok('XML::TreePP') };
# ----------------------------------------------------------------
    my $DIVISION_SIGN = {
        'Shift_JIS' =>  "\x81\x80",
        'EUC-JP'    =>  "\xA1\xE0",
        'GB2312'    =>  "\xA1\xC2",
        'EUC-KR'    =>  "\xA1\xC0",
        'BIG5'      =>  "\xA1\xD2",
        'UTF-8'     =>  "\xC3\xB7",
        'Latin-1'   =>  "\xF7",
    };
    my $PLUSMINUS_SIGN = {
        'Shift_JIS' =>  "\x81\x7D",
        'EUC-JP'    =>  "\xA1\xDE",
        'GB2312'    =>  "\xA1\xC0",
        'EUC-KR'    =>  "\xA1\xBE",
        'BIG5'      =>  "\xA1\xD3",
        'UTF-8'     =>  "\xC2\xB1",
        'Latin-1'   =>  "\xB1",
    };
# ----------------------------------------------------------------
SKIP: {
    &test_main('UTF-8');
    if ( $] < 5.008 ) {
        eval { require Jcode; } unless defined $Jcode::VERSION;
        if ( ! defined $Jcode::VERSION ) {
            skip( "Jcode.pm is not loaded.", 36 );
        }
    }
    &test_main('Shift_JIS');
    &test_main('EUC-JP');
    skip( "Perl $]", 24 ) if ( $] < 5.008 );
    &test_main('Latin-1');
    &test_main('EUC-KR');
    &test_main('GB2312');
    &test_main('BIG5');
}
# ----------------------------------------------------------------
sub test_main {
    my $code = shift;
    my $tpp = XML::TreePP->new();
    my $tree = {
        root => {
            division    => $DIVISION_SIGN->{'UTF-8'},
            plusminus   => $PLUSMINUS_SIGN->{'UTF-8'},
        },
    };

    my $xml1 = $tpp->write( $tree, $code );
    $tpp->set( output_encoding => $code );
    my $xml2 = $tpp->write( $tree );

    like( $xml1, qr/^\s*<\?xml[^<>]+encoding="\Q$code\E"/is, "encoding $code 1" );
    like( $xml2, qr/^\s*<\?xml[^<>]+encoding="\Q$code\E"/is, "encoding $code 2" );

    my $div1 = ( $xml1 =~ m/<division>([^<>]+)</ )[0];
    my $div2 = ( $xml2 =~ m/<division>([^<>]+)</ )[0];
    is( $div1, $DIVISION_SIGN->{$code},  "division $code 1" );
    is( $div2, $DIVISION_SIGN->{$code},  "division $code 2" );

    my $plm1 = ( $xml1 =~ m/<plusminus>([^<>]+)</ )[0];
    my $plm2 = ( $xml2 =~ m/<plusminus>([^<>]+)</ )[0];
    is( $plm1, $PLUSMINUS_SIGN->{$code}, "plusminus $code 1" );
    is( $plm2, $PLUSMINUS_SIGN->{$code}, "plusminus $code 2" );
}
# ----------------------------------------------------------------
;1;
# ----------------------------------------------------------------
