# ----------------------------------------------------------------
    use strict;
    use Test::More tests => 3;
    BEGIN { use_ok('XML::TreePP') };
# ----------------------------------------------------------------
{
    my $tree = {
        root    =>  {
            one     =>  1,
            two     =>  2,
            three   =>  3,
            four    =>  4,
            five    =>  5,
            six     =>  6,
            seven   =>  7,
            eight   =>  8,
            nine    =>  9,
        },
    };

    {
        my $tpp = XML::TreePP->new();
        $tpp->set( first_out => [qw( one   two   three )] );
        $tpp->set( last_out  => [qw( seven eight nine  )] );
        my $xml = $tpp->write( $tree );
        like( $xml, qr{<one>.*<two>.*<three>.*<five>.*<seven>.*<eight>.*<nine>}s, "1-2-3-*-5-*-7-8-9" );
    }

    {
        my $tpp = XML::TreePP->new();
        $tpp->set( first_out => [qw( seven eight nine  )] );
        $tpp->set( last_out  => [qw( one   two   three )] );
        my $xml = $tpp->write( $tree );
        like( $xml, qr{<seven>.*<eight>.*<nine>.*<five>.*<one>.*<two>.*<three>}s, "7-8-9-*-5-*-1-2-3" );
    }
}
# ----------------------------------------------------------------
;1;
# ----------------------------------------------------------------
