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
    my $ENC = 'UNKNOWN_ENCODING';
# ----------------------------------------------------------------
{
    plan tests => 4;
    use_ok('XML::TreePP');
    &test1();
    &test2();
    &test3();
}
# ----------------------------------------------------------------
sub test1 {
    my $xml = <<"EOT";
<?xml version="1.0" encoding="$ENC"?>
<root>
    <elem>value</elem>
</root>
EOT

    my $tpp = XML::TreePP->new();

    local $@;
    eval {
        my $tree = $tpp->parse( $xml );
    };
    like( $@, qr#^Unknown encoding#, 'parse: '.$@ );
}
# ----------------------------------------------------------------
sub test2 {
    my $tree = { root => { elem => 'value' }};
    my $tpp = XML::TreePP->new();

    local $@;
    eval {
        my $xml = $tpp->write( $tree, $ENC );
    };
    like( $@, qr#^Unknown encoding#, 'write: '.$@ );
}
# ----------------------------------------------------------------
sub test3 {
    my $tree = { root => { elem => 'value' }};
    my $tpp = XML::TreePP->new( output_encoding => $ENC );

    local $@;
    eval {
        my $xml = $tpp->write( $tree );
    };
    like( $@, qr#^Unknown encoding#, 'output_encoding: '.$@ );
}
# ----------------------------------------------------------------
;1;
# ----------------------------------------------------------------
