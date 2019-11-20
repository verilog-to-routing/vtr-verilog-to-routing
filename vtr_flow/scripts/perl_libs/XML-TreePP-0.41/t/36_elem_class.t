# ----------------------------------------------------------------
    use strict;
    use Test::More;
# ----------------------------------------------------------------
{
    plan tests => 14;
    use_ok('XML::TreePP');
    &test_elem_class( force_array => [qw( six )], elem_class => 'Element' );
}
# ----------------------------------------------------------------
sub test_elem_class {
    my $tpp = XML::TreePP->new(@_);

    my $xml = <<"EOT";
<root>
    <one>1</one>
    <two attr="-2">2</two>
    <three>
        <four>4</four>
        <five>
            5
            <empty/>
            5
        </five>
    </three>
    <six><seven attr="-7">7</seven></six>
    <eight>8</eight>
    <eight><nine>9</nine></eight>
    <foo>
        <bar hoge="1"/>
        <bar pomu="2"/>
    </foo>
</root>
EOT

    my $tree = $tpp->parse( $xml );
    is( ref $tree,                          'Element',          '/root' );
    is( ref $tree->{root},                  'Element::root',    '/root' );
    is( ref $tree->{root}->{two},           'Element::two',     '/root/two' );
    is( ref $tree->{root}->{three},         'Element::three',   '/root/three' );
    is( ref $tree->{root}->{three}->{five}, 'Element::five',    '/root/three/five' );
    is( ref $tree->{root}->{six},           'ARRAY',            '/root/six (ARRAY)' );
    is( ref $tree->{root}->{six}->[0],      'Element::six',     '/root/six' );
    is( ref $tree->{root}->{six}->[0]->{seven}, 'Element::seven', '/root/six/seven' );
    is( ref $tree->{root}->{eight},         'ARRAY',            '/root/eight (ARRAY)' );
    is( ref $tree->{root}->{eight}->[1],    'Element::eight',   '/root/eight' );

	# 2007/08/07 added
    is( ref $tree->{root}->{foo}, 'Element::foo', '/root/foo' );
    is( ref $tree->{root}->{foo}->{bar}, 'ARRAY', '/root/foo/bar (ARRAY)' );
    is( ref $tree->{root}->{foo}->{bar}->[0], 'Element::bar', '/root/foo/bar' );
}
# ----------------------------------------------------------------
;1;
# ----------------------------------------------------------------
