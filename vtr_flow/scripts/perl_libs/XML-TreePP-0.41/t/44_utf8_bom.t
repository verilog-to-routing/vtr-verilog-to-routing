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
    my $FILES = [qw(
        t/example/hello-en-utf8.xml
        t/example/hello-en-nodecl.xml
        t/example/hello-en-noenc.xml
        t/example/hello-en-utf8-bom.xml
        t/example/hello-en-nodecl-bom.xml
        t/example/hello-en-noenc-bom.xml
    )];
# ----------------------------------------------------------------
{
    plan tests => 13;
    use_ok('XML::TreePP');
    &test_main();
}
# ----------------------------------------------------------------
sub test_main {
    my $octets = 'Hello, World! §±×÷';
    my $string = $octets;
    require utf8;
    utf8::decode( $string );

    my $tpp1 = XML::TreePP->new( utf8_flag => 0 );
    my $tpp2 = XML::TreePP->new( utf8_flag => 1 );
    foreach my $file ( @$FILES ) {
        my $tree1 = $tpp1->parsefile( $file );
        is( $tree1->{root}->{text}, $octets, $file." without utf8_flag" );
        my $tree2 = $tpp2->parsefile( $file );
        is( $tree2->{root}->{text}, $string, $file." with utf8_flag" );
    }
}
# ----------------------------------------------------------------
;1;
# ----------------------------------------------------------------
