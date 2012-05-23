# ----------------------------------------------------------------
    use strict;
    use Test::More tests => 5;
    BEGIN { use_ok('XML::TreePP') };
# ----------------------------------------------------------------
{
    my $tpp = XML::TreePP->new();
    my $scalar = 'string';

    {
        local $@;
        eval {
            my $xml = $tpp->write( undef );
        };
        like( $@, qr#^Invalid tree#, 'undef: '.$@ );
    }
    {
        local $@;
        eval {
            my $xml = $tpp->write( [] );
        };
        like( $@, qr#^Invalid tree#, 'arrayref: '.$@ );
    }
    {
        local $@;
        eval {
            my $xml = $tpp->write( $scalar );
        };
        like( $@, qr#^Invalid tree#, 'scalar: '.$@ );
    }
    {
        local $@;
        eval {
            my $xml = $tpp->write( \$scalar );
        };
        like( $@, qr#^Invalid tree#, 'scalarref: '.$@ );
    }
}
# ----------------------------------------------------------------
;1;
# ----------------------------------------------------------------
