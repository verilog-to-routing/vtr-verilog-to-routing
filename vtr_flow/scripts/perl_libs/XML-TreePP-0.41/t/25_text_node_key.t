#   25_text_noe_key.t

use strict;
use Test::More tests => 13;
BEGIN { use_ok('XML::TreePP') };

my $tpp = XML::TreePP->new();
$tpp->set( cdata_scalar_ref => 1 );

my $hello = 'Hello, World!';
my $tnode_keys = [ '#text', '_content', '0' ];

foreach my $tkey ( @$tnode_keys ) {
    my $rand = int(rand() * 9000 + 1000);
    my $text = "$hello $rand $tkey";

    my $tree = {
        root    =>  {
            text    =>  {
                -attr   =>  $text,
                $tkey   =>  $text,
            },
            cdata   =>  {
                -attr   =>  $text,
                $tkey   =>  \$text,
            },
        }
    };
    $tpp->set( text_node_key => $tkey );
    my $write = $tpp->write( $tree );
#   print STDERR $write;

    my $back = $tpp->parse( $write );
    is( $back->{root}->{text}->{-attr}, $text, "attribute1 for $tkey" );
    is( $back->{root}->{text}->{$tkey}, $text, "text node for $tkey" );
    is( $back->{root}->{cdata}->{-attr}, $text, "attribute2 for $tkey" );
    my $ref = $back->{root}->{cdata}->{$tkey};
    is( $$ref, $text, "cdata node for $tkey (content)" ) if ref $ref;
    is( $text, 'SCALAR(0x...)', "cdata node for $tkey (ref)" ) unless ref $ref;
}

1;
