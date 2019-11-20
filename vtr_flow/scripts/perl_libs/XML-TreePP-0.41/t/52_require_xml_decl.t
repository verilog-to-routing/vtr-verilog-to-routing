# ----------------------------------------------------------------
    use strict;
    use Test::More;
# ----------------------------------------------------------------
    my $xml1 = <<"EOT";
<root>
    <elem>value</elem>
</root>
EOT
    my $xml2 = <<"EOT";
<?xml version="1.0" encoding="UTF-8"?>
<root>
    <elem>value</elem>
</root>
EOT
# ----------------------------------------------------------------
{
    plan tests => 5;
    use_ok('XML::TreePP');

    my $tpp1 = XML::TreePP->new();
    my $tpp2 = XML::TreePP->new( require_xml_decl => 1 );
    
    my $res;
    my $die;

    ($res, $die) = &test($tpp1, $xml1);
    is($res->{root}->{elem}, 'value', 'no decl and default');

    ($res, $die) = &test($tpp1, $xml2);
    is($res->{root}->{elem}, 'value', 'has decl and default');

    ($res, $die) = &test($tpp2, $xml1);
    like($die, qr/^XML DECLARATION NOT FOUND/i, 'require_xml_decl works');

    ($res, $die) = &test($tpp2, $xml2);
    is($res->{root}->{elem}, 'value', 'has decl and require_xml_decl');
}
# ----------------------------------------------------------------
sub test {
    my $tpp = shift;
    my $xml = shift;
    my $exp = shift;

    local $@;
    my $tree;
    eval {
        $tree = $tpp->parse($xml);
    };
    return ($tree, $@);
}
# ----------------------------------------------------------------
;1;
# ----------------------------------------------------------------
