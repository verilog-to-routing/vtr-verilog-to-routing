# ----------------------------------------------------------------
    use strict;
    use Test::More;
# ----------------------------------------------------------------
SKIP: {
    local $@;
    eval { require Tie::IxHash; } unless defined $Tie::IxHash::VERSION;
    if ( ! defined $Tie::IxHash::VERSION ) {
        plan skip_all => 'Tie::IxHash is not loaded.';
    }
    plan tests => 19;
    use_ok('XML::TreePP');
    &test_parse();
    &test_write();
    &test_parse_with_attr();
    &test_write_with_attr();
}
# ----------------------------------------------------------------
sub test_parse {
    my $xml = <<"EOT";
<root>
    <one>1</one>
    <two>2</two>
    <three>3</three>
    <four>4</four>
    <five>5</five>
    <six>6</six>
    <seven>7</seven>
    <eight>8</eight>
    <nine>9</nine>
</root>
EOT

    my $tpp = XML::TreePP->new();
    $tpp->set( use_ixhash => 1 );
    my $tree = $tpp->parse( $xml );

    my $prev;
    foreach my $key ( keys %{$tree->{root}} ) {
        my $val = $tree->{root}->{$key};
        is( $prev+1, $val, "<$key>$val</$key>" ) if $prev;
        $prev = $val;
    }
}
# ----------------------------------------------------------------
sub test_write {
    my $root = {};
    tie( %$root, 'Tie::IxHash' );

    $root->{one}   = 1;
    $root->{two}   = 2;
    $root->{three} = 3;
    $root->{four}  = 4;
    $root->{five}  = 5;
    $root->{six}   = 6;
    $root->{seven} = 7;
    $root->{eight} = 8;
    $root->{nine}  = 9;

    my $tree = {
        root => $root
    };
    my $tpp = XML::TreePP->new();
    $tpp->set( use_ixhash => 1 );
    my $xml = $tpp->write( $tree );
    like( $xml, qr{1.*2.*3.*4.*5.*6.*7.*8.*9}s, "1-2-3-4-5-6-7-8-9" );
}
# ----------------------------------------------------------------
sub test_parse_with_attr {
    my $xml = <<"EOT";
<root one="1" two="2" three="3" four="4" five="5">
    <six>6</six>
    <seven>7</seven>
    <eight>8</eight>
    <nine>9</nine>
</root>
EOT

    my $tpp = XML::TreePP->new();
    $tpp->set( use_ixhash => 1 );
    my $tree = $tpp->parse( $xml );

    my $prev;
    foreach my $key ( keys %{$tree->{root}} ) {
        my $val = $tree->{root}->{$key};
        my $view = ( $key =~ /^-/ ) ? "$key=" : "<$key>";
        is( $prev+1, $val, $view.$val ) if $prev;
        $prev = $val;
    }
}
# ----------------------------------------------------------------
sub test_write_with_attr {
    my $root = {};
    tie( %$root, 'Tie::IxHash' );

    $root->{one}    = 1;
    $root->{two}    = 2;
    $root->{-three} = 3;
    $root->{-four}  = 4;
    $root->{-five}  = 5;
    $root->{-six}   = 6;
    $root->{-seven} = 7;
    $root->{eight}  = 8;
    $root->{nine}   = 9;

    my $tree = {
        root => $root
    };
    my $tpp = XML::TreePP->new();
    $tpp->set( use_ixhash => 1 );
    my $xml = $tpp->write( $tree );
    like( $xml, qr{3.*4.*5.*6.*7.*1.*2.*8.*9}s, "3-4-5-6-7-1-2-8-9" );
}
# ----------------------------------------------------------------
;1;
# ----------------------------------------------------------------
