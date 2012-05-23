# ----------------------------------------------------------------
    use strict;
    use Test::More;
# ----------------------------------------------------------------
    my $tree1 = {root=>{-attr=>'val', "#text"=>""}};
    my $tree2 = {root=>{-attr=>'val', "#text"=>undef}};
# ----------------------------------------------------------------
{
    plan tests => 3;
    use_ok('XML::TreePP');

    my $tpp = XML::TreePP->new();
    my $xml;

    $xml = $tpp->write($tree1);
    like($xml, qr:<root [^>]*></root>:, "text node with zero length string");

    $xml = $tpp->write($tree2);
    like($xml, qr:<root [^>]*/>:, "text node of undef");
}
# ----------------------------------------------------------------
;1;
# ----------------------------------------------------------------
