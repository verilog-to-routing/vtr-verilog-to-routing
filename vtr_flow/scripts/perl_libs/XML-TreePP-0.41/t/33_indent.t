# ----------------------------------------------------------------
    use strict;
    use Test::More;
# ----------------------------------------------------------------
{
    plan tests => 73;
    use_ok('XML::TreePP');
    &test_indent( undef );
    &test_indent( 1 );
    &test_indent( 4 );
}
# ----------------------------------------------------------------
sub test_indent {
    my $indent = shift;
    my $order = [qw( one two three four five six seven eight nine )];
    my $tpp = XML::TreePP->new( first_out => $order, indent => $indent );

    my $nine = '9';
    my $tree = {
        root    =>  {
            one =>  '1',
            two =>  {
                '#text' =>  '2',
                three   =>  undef,
            },
            four    =>  [{
                five    =>  '5',
                six     =>  {
                    '#text' =>  '6',
                },
            }, {
                seven   =>  {
                    '#text' =>  '7',
                    -eight  =>  '8',
                },
            }],
            nine    =>  \$nine,
        },
    };

    my $xml = $tpp->write( $tree );
    my $space = $indent ? '\040' x $indent : '';
    $indent ||= 0;

    like( $xml, qr{ <one>1</one> }x, "[$indent] text node" );
    like( $xml, qr{ <two><three  }x, "[$indent] child node" );
    like( $xml, qr{ />2</two>    }x, "[$indent] text node after empty node" );
    like( $xml, qr{ <six>6</six> }x, "[$indent] explicit text node" );
    like( $xml, qr{ >7</seven>   }x, "[$indent] text node after attribute" );
    like( $xml, qr{ <nine><!\[CDATA\[9\]\]></nine> }x, "[$indent] cdata node" );

    like( $xml, qr{ ^<root>        }mx, "[$indent] no-indent root" );
    like( $xml, qr{ ^$space<one>   }mx, "[$indent] indent one" );
    like( $xml, qr{ ^$space<two>   }mx, "[$indent] indent two" );
    like( $xml, qr{ ^$space<four>  }mx, "[$indent] indent four" );
    like( $xml, qr{ ^$space</four> }mx, "[$indent] indent four end" );
    like( $xml, qr{ ^$space$space<five> }mx, "[$indent] indent five" );
    like( $xml, qr{ ^$space$space<six>  }mx, "[$indent] indent six" );
    like( $xml, qr{ ^$space$space<seven }mx, "[$indent] indent seven" );
    like( $xml, qr{ ^$space<nine>  }mx, "[$indent] indent nine" );
    like( $xml, qr{ ^</root>       }mx, "[$indent] no-indent root end" );

    like( $xml, qr{ <root>\n  }x, "[$indent] line root" );
    like( $xml, qr{ </one>\n  }x, "[$indent] line one" );
    like( $xml, qr{ </two>\n  }x, "[$indent] line two" );
    like( $xml, qr{ </five>\n }x, "[$indent] line five" );
    like( $xml, qr{ </six>\n  }x, "[$indent] line six" );
    like( $xml, qr{ </four>\n }x, "[$indent] line four" );
    like( $xml, qr{ </nine>\n }x, "[$indent] line nine" );
    like( $xml, qr{ </root>\n }x, "[$indent] line root" );
}
# ----------------------------------------------------------------
=example
<root>
    <one>1</one>
    <two><three />2</two>
    <four>
        <five>5</five>
        <six>6</six>
    </four>
    <four>
        <seven eight="8">7</seven>
    </four>
    <nine><![CDATA[9]]></nine>
</root>
=cut
# ----------------------------------------------------------------
;1;
# ----------------------------------------------------------------
