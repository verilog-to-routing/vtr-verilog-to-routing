# ----------------------------------------------------------------
    use strict;
    use Test::More tests => 129;
    BEGIN { use_ok('XML::TreePP') };
# ----------------------------------------------------------------
{
    my $test = {
        '<xml><![CDATA[AAA]]></xml>'                            =>  'AAA',
        '<xml>AAA<![CDATA[BBB]]>CCC</xml>'                      =>  'AAABBBCCC',
        '<xml><![CDATA[AAA]]>BBB<![CDATA[CCC]]></xml>'          =>  'AAABBBCCC',
        '<xml>AAA<![CDATA[BBB]]>CCC<![CDATA[DDD]]>EEE</xml>'    =>  'AAABBBCCCDDDEEE',

        '<xml><!-- AAA --></xml>'                               =>  '',
        '<xml>AAA<!-- BBB -->CCC</xml>'                         =>  'AAACCC',
        '<xml><!-- AAA -->BBB<!-- CCC --></xml>'                =>  'BBB',
        '<xml>AAA<!-- BBB -->CCC<!-- DDD -->EEE</xml>'          =>  'AAACCCEEE',

        '<xml><!-- AAA -->BBB<![CDATA[CCC]]></xml>'                     =>  'BBBCCC',
        '<xml><![CDATA[AAA]]>BBB<!-- CCC --></xml>'                     =>  'AAABBB',

        '<xml><!-- AAA -->BBB<![CDATA[CCC]]>DDD<!-- EEE --></xml>'      =>  'BBBCCCDDD',
        '<xml><![CDATA[AAA]]>BBB<!-- CCC -->DDD<![CDATA[EEE]]></xml>'   =>  'AAABBBDDDEEE',

        '<xml><![CDATA[<!-- AAA -->]]></xml>'                           =>  '<!-- AAA -->',
        '<xml><!-- <![CDATA[AAA]]> --></xml>'                           =>  '',

        '<xml><![CDATA[<!-- AAA -->]]><!-- <![CDATA[BBB]]> --></xml>'   =>  '<!-- AAA -->',
        '<xml><!-- <![CDATA[AAA]]> --><![CDATA[<!-- BBB -->]]></xml>'   =>  '<!-- BBB -->',
    };
    &cdata_cdsect( $test );
    &cdata_cdsect( $test, { cdata_scalar_ref=>1 } );
}
# ----------------------------------------------------------------
sub cdata_cdsect {
    my $list = shift;
    my $opt  = shift;
    my $tpp  = XML::TreePP->new( %$opt );
	my $memo = exists $opt->{cdata_scalar_ref} ? 'cdata_scalar_ref ' : 'default ';

    foreach my $src ( keys %$list ) {
        my $val = $list->{$src};
        my $tree = $tpp->parse( $src );
        ok( exists $tree->{xml}, $memo.'exists' );

        my $cdata = $tree->{xml};
        $cdata = $$cdata if ( ref $cdata eq 'SCALAR' );
        ok( ! ref $cdata, $memo.'invalid ref' );
        is( $cdata, $val, $memo.$val );

        my $xml = $tpp->write( $tree );
        my $again = $tpp->parse( $xml );
        my $cdat2 = $again->{xml};
        $cdat2 = $$cdat2 if ( ref $cdat2 eq 'SCALAR' );
        is( $cdat2, $cdata, $memo.'round trip' );
    }
}
# ----------------------------------------------------------------
;1;
# ----------------------------------------------------------------
