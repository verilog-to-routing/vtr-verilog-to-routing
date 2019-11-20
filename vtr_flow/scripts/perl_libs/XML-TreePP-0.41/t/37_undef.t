# ----------------------------------------------------------------
    use strict;
    use Test::More;
# ----------------------------------------------------------------
{
    plan tests => 6;
    use_ok('XML::TreePP');
    &test_undef( first_out => [qw( attr hash list empty undef -one -two three four )] );
}
# ----------------------------------------------------------------
sub test_undef {
    my $tpp = XML::TreePP->new(@_);

    my $empty = '';
    my $undef = undef;
    my $tree = {
        root    =>  {
            attr    =>  { -one=>'', -two=>undef },
            hash    =>  { three => '', four => undef },
            list    =>  [ '', undef ],
            empty   =>  \$empty,
            undef   =>  \$undef,
        }
    };
    my $xml = $tpp->write( $tree );

    like( $xml, qr{<attr one="" two=""}, 'attr one two' );
    like( $xml, qr{ <hash>\s*<three }xs, 'hash three' );
    like( $xml, qr{ </three>\s*<four }xs, 'hash four' );
    like( $xml, qr{ <empty><!\[CDATA\[ }xs, 'empty cdata' );
    like( $xml, qr{ <undef><!\[CDATA\[ }xs, 'undef cdata' );
}
# ----------------------------------------------------------------
;1;
# ----------------------------------------------------------------
