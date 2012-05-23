# ----------------------------------------------------------------
    use strict;
    use Test::More tests => 25;
    BEGIN { use_ok('XML::TreePP') };
# ----------------------------------------------------------------
{
    my $tpp = XML::TreePP->new( force_array => [qw( one two three )] );
    my $source = <<"EOT";
<root>
    <zero>AAA</zero>
    <one>CCC</one>
    <two>DDD</two><two>EEE</two>
    <three/><three/><three/>
</root>
EOT
    my $tree = $tpp->parse( $source );

    ok( ! ref $tree->{root}->{zero}, "A: normal node" );
    ok( ref $tree->{root}->{one}   , "A: one force_array node" );
    ok( ref $tree->{root}->{two}   , "A: two child nodes" );
    ok( ref $tree->{root}->{three} , "A: three empty nodes" );

    is( scalar( @{$tree->{root}->{one}}   ), 1, "A: one force_array node" );
    is( scalar( @{$tree->{root}->{two}}   ), 2, "A: two child nodes" );
    is( scalar( @{$tree->{root}->{three}} ), 3, "A: three empty nodes" );

    is( scalar( grep {$_} @{$tree->{root}->{one}}   ), 1, "A: one force_array node" );
    is( scalar( grep {$_} @{$tree->{root}->{two}}   ), 2, "A: two child nodes" );
    is( scalar( grep {$_} @{$tree->{root}->{three}} ), 0, "A: three empty nodes" );
}
# ----------------------------------------------------------------
{
    my $tpp = XML::TreePP->new( force_array => [qw( one two three )] );
    my $source = <<"EOT";
<root>
    <one aaa="1"></one>
    <two bbb="2"></two><two ccc=""></two>
    <three ddd="4"/><three eee="5"/><three fff="6"></three>
</root>
EOT
    my $tree = $tpp->parse( $source );

    is( scalar( @{$tree->{root}->{one}}   ), 1, "B: one force_array node" );
    is( scalar( @{$tree->{root}->{two}}   ), 2, "B: two child nodes" );
    is( scalar( @{$tree->{root}->{three}} ), 3, "B: three empty nodes" );

    is( scalar( grep {ref $_} @{$tree->{root}->{one}}   ), 1, "B: one force_array node" );
    is( scalar( grep {ref $_} @{$tree->{root}->{two}}   ), 2, "B: two child nodes" );
    is( scalar( grep {ref $_} @{$tree->{root}->{three}} ), 3, "B: three empty nodes" );
}
# ----------------------------------------------------------------
{
    my $tpp = XML::TreePP->new( force_array => '*' );
    my $source = <<"EOT";
<root>
    <one>1</one>
	<two><three>3</three></two>
</root>
EOT
    my $tree = $tpp->parse( $source );

    is( ref $tree->{root},                  'ARRAY', 'C: root ARRAY' );
    is( ref $tree->{root}->[0],             'HASH',  'C: root HASH' );

    is( ref $tree->{root}->[0]->{one},      'ARRAY', 'C: one ARRAY' );
    is( $tree->{root}->[0]->{one}->[0],      '1',    'C: one text' );

    is( ref $tree->{root}->[0]->{two},      'ARRAY', 'C: two ARRAY' );
    is( ref $tree->{root}->[0]->{two}->[0], 'HASH',  'C: two HASH' );

    is( ref $tree->{root}->[0]->{two}->[0]->{three},  'ARRAY', 'C: three ARRAY' );
    is( $tree->{root}->[0]->{two}->[0]->{three}->[0], '3',     'C: three text' );
}
# ----------------------------------------------------------------
;1;
# ----------------------------------------------------------------
