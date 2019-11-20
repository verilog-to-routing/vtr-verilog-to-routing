# ----------------------------------------------------------------
    use strict;
    use Test::More tests => 8;
    BEGIN { use_ok('XML::TreePP') };
## ----------------------------------------------------------------
{
    my $scalar = 'value';
    my $obj = MyObject->new( elem => 'value' );
    my $tree1 = { hashref   => { elem => 'value' } };
    my $tree2 = { arrayref  => { elem => [ 'first', 'last' ] }};
    my $tree3 = { scalarref => \$scalar };
    my $tree4 = { coderef   => sub {} };
    my $tree5 = { object    => $obj };
    my $tree6 = { blob      => *STDIN };
    my $tree7 = { blobref   => \*STDIN };

    my $tpp = XML::TreePP->new();

    local $SIG{__WARN__} = sub {};  # ignore warn messages

    my $xml1 = $tpp->write( $tree1 );
    like( $xml1, qr#<elem>value</elem>#, 'no1: HASHREF - child node' );

    my $xml2 = $tpp->write( $tree2 );
    like( $xml2, qr#<elem>first</elem>\s*<elem>last</elem>#s, 'no2: ARRAYREF - multiple nodes' );

    my $xml3 = $tpp->write( $tree3 );
    my $exp3 = '<scalarref><![CDATA[value]]></scalarref>';
    like( $xml3, qr#\Q$exp3\E#, 'no3: SCALARREF - cdata node' );

    my $xml4 = $tpp->write( $tree4 );
    like( $xml4, qr#xml#, 'no4: CODEREF - undefined behavior rather than die' );

    my $xml5 = $tpp->write( $tree5 );
    like( $xml5, qr#<elem>value</elem>#, 'no5: OBJECT - as a normal child node' );

    my $xml6 = $tpp->write( $tree6 );
    like( $xml6, qr#xml#, 'no6: BLOB - undefined behavior rather than die' );

    my $xml7 = $tpp->write( $tree7 );
    like( $xml7, qr#xml#, 'no7: BLOBREF - undefined behavior rather than die' );
}
# ----------------------------------------------------------------
package MyObject;

sub new {
    my $class = shift;
    my $hash = { @_ };
    bless $hash, $class;
}
# ----------------------------------------------------------------
;1;
# ----------------------------------------------------------------
