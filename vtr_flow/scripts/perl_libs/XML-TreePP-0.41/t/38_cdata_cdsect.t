# ----------------------------------------------------------------
    use strict;
    use Test::More tests => 161;
    BEGIN { use_ok('XML::TreePP') };
# ----------------------------------------------------------------
{
    my $test = {
        '<cdata><![CDATA[]]></cdata>'       =>  '',
        '<cdata><![CDATA[]]]></cdata>'      =>  ']',
        '<cdata><![CDATA[>]]></cdata>'      =>  '>',

        '<cdata><![CDATA[]]]]></cdata>'                 =>  ']]',
        '<cdata><![CDATA[]]]><![CDATA[]]]></cdata>'     =>  ']]',

        '<cdata><![CDATA[]>]]></cdata>'                 =>  ']>',
        '<cdata><![CDATA[]]]><![CDATA[>]]></cdata>'     =>  ']>',

        '<cdata>]<![CDATA[]]]>&gt;</cdata>'             =>  ']]>',
        '<cdata>]<![CDATA[]>]]></cdata>'                =>  ']]>',
        '<cdata><![CDATA[]]]]>&gt;</cdata>'             =>  ']]>',
        '<cdata><![CDATA[]]]]><![CDATA[>]]></cdata>'    =>  ']]>',
        '<cdata><![CDATA[]]]><![CDATA[]>]]></cdata>'    =>  ']]>',
        '<cdata>]<![CDATA[]]]><![CDATA[>]]></cdata>'    =>  ']]>',
        '<cdata><![CDATA[]]]><![CDATA[]]]>&gt;</cdata>' =>  ']]>',
        '<cdata><![CDATA[]]]>]<![CDATA[>]]></cdata>'    =>  ']]>',
        '<cdata><![CDATA[]]]><![CDATA[]]]><![CDATA[>]]></cdata>'    =>  ']]>',

        '<cdata><![CDATA[]]]><![CDATA[]>]]]]><![CDATA[>]]></cdata>' =>  ']]>]]>',
        '<cdata><![CDATA[]]]><![CDATA[]>]]]><![CDATA[]>]]></cdata>' =>  ']]>]]>',

        '<cdata><![CDATA[]]]><![CDATA[]>]]><![CDATA[]]]]><![CDATA[>]]></cdata>' =>  ']]>]]>',
        '<cdata><![CDATA[]]]]><![CDATA[>]]><![CDATA[]]]><![CDATA[]>]]></cdata>' =>  ']]>]]>',
    };
    &cdata_cdsect( $test );
    &cdata_cdsect( $test, { cdata_scalar_ref=>1 } );
}
# ----------------------------------------------------------------
sub cdata_cdsect {
    my $list = shift;
    my $opt  = shift;
    my $tpp  = XML::TreePP->new( %$opt );

    foreach my $src ( keys %$list ) {
        my $val = $list->{$src};
        my $tree = $tpp->parse( $src );
        ok( exists $tree->{cdata}, 'exists' );

        my $cdata = $tree->{cdata};
        $cdata = $$cdata if ( ref $cdata eq 'SCALAR' );
        ok( ! ref $cdata, 'invalid ref' );
        is( $cdata, $val, $val );

        my $xml = $tpp->write( $tree );
        my $again = $tpp->parse( $xml );
        my $cdat2 = $again->{cdata};
        $cdat2 = $$cdat2 if ( ref $cdat2 eq 'SCALAR' );
        is( $cdat2, $cdata, 'round trip' );
    }
}
# ----------------------------------------------------------------
;1;
# ----------------------------------------------------------------
