# ----------------------------------------------------------------
    use strict;
    use Test::More tests => 5;
    BEGIN { use_ok('XML::TreePP') };
# ----------------------------------------------------------------
    my $tpp = XML::TreePP->new(require_xml_decl=>1);

    my $source = join('', <DATA>);
    ok ($source, 'loaded inline content to be parsed');
#   diag ('Loaded ' . length($source) . ' bytes');

    # The following should not segfault
    my $tree;
    eval {
        $tree = $tpp->parse($source);
    };
    like( $@, qr/XML declaration not found/, 'parsing random html should not segfault' );
    is( $tree, undef, 'parsing random html returns undefined values');

    ok( 1, 'https://rt.cpan.org/Ticket/Display.html?id=42441' );
# ----------------------------------------------------------------
;1;
# ----------------------------------------------------------------

__DATA__
<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN"
        "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">

<html xmlns="http://www.w3.org/1999/xhtml" xml:lang="en" lang="en">

<head>
	<link rel="Shortcut Icon" href="/tam.ico">
	<meta http-equiv="Content-Type" content="text/html; charset=utf-8" />
	<meta name="description" content="The Texas A&M Official Athletic Site, partner of CSTV Networks, Inc. The most comprehensive coverage of Texas A&M Athletics on the web." />

	<link href="http://grfx.cstv.com/schools/tam/library/css/tam-08-display.css" rel="stylesheet" type="text/css" />
    <script type="text/javascript" src="http://grfx.cstv.com/schools/tam/library/scripts/tam-08-tabs.js"></script>
    <script type="text/javascript" src="http://grfx.cstv.com/scripts/common.js"></script>
    <script type="text/javascript" src="http://grfx.cstv.com/scripts/oas-omni-controls.js"></script>

	<title>Texas A&M Official Athletic Site</title>
	<!-- CBS College Sports 2008 - Producer: Ashton R. Edwin-Kent -->
	























	
</head>

<body>

<div id="wrap-sport">
	
	<div id="mast">
		<div id="mast-top-nav">
			<ul id="top-nav">
	<li id="top-nav-01"><a href="/sports/m-baskbl/tam-m-baskbl-body.html"><span>Men's Basketball</span><!--[if gt IE 6]><!--></a><!--<![endif]-->
			<!--[if lt IE 7]><table><tr><td><![endif]-->
		<ul>
			<li><a href="/sports/m-baskbl/sched/tam-m-baskbl-sched.html">Schedule</a></li>
			<li><a href="/sports/m-baskbl/mtt/tam-m-baskbl-mtt.html"">Roster</a></li>
			<li><a href="/sports/m-baskbl/stats/2007-2008/teamcume.html"">Stats</a></li>
		</ul>
		<!--[if lt IE 7]></td></tr></table></a><![endif]-->
	</li>
	<li id="top-nav-02"><a href="/sports/m-basebl/tam-m-basebl-body.html" ><span>Baseball</span><!--[if gt IE 6]><!--></a><!--<![endif]-->
			<!--[if lt IE 7]><table><tr><td><![endif]-->
		<ul>
	 		<li><a href="/sports/m-basebl/sched/tam-m-basebl-sched.html"">Schedule</a></li>
			<li><a href="/sports/m-basebl/mtt/tam-m-basebl-mtt.html"">Roster</a></li>
			<li><a href="http://aggieathletics.cstv.com/sports/m-basebl/stats/2007-2008/teamcume.html">Stats</a></li>
		</ul>
		<!--[if lt IE 7]></td></tr></table></a><![endif]-->
	</li>
	<li id="top-nav-03"><a href="/sports/m-footbl/tam-m-footbl-body.html" ><span>Football</span><!--[if gt IE 6]><!--></a><!--<![endif]-->
			<!--[if lt IE 7]><table><tr><td><![endif]-->
		<ul>
			<li><a href="/sports/m-footbl/sched/tam-m-footbl-sched.html"">Schedule</a></li>
			<li><a href="/sports/m-footbl/mtt/tam-m-footbl-mtt.html">Roster</a></li>
			<li><a href="/sports/m-footbl/stats/2008-2009/teamstat.html"">Stats</a></li>
		</ul>
		<!--[if lt IE 7]></td></tr></table></a><![endif]-->
	</li>
	<li id="top-nav-04"><a href="/sports/m-golf/tam-m-golf-body.html" ><span>Men's Golf</span><!--[if gt IE 6]><!--></a><!--<![endif]-->
			<!--[if lt IE 7]><table><tr><td><![endif]-->
		<ul>
			<li><a href="/sports/m-golf/sched/tam-m-golf-sched.html"">Schedule</a></li>
			<li><a href="/sports/m-golf/mtt/tam-m-golf-mtt.html"">Roster</a></li>
			<li><a href="http://golfstat.com/ultimate/M522web/M522index.htm" target="_blank">Stats</a></li>
		</ul>
		<!--[if lt IE 7]></td></tr></table></a><![endif]-->
	</li>
	<li id="top-nav-05"><a href="/sports/m-swim/tam-m-swim-body.html" ><span>Men's Swimming & Diving</span><!--[if gt IE 6]><!--></a><!--<![endif]-->
			<!--[if lt IE 7]><table><tr><td><![endif]-->
		<ul>
			<li><a href="/sports/m-swim/sched/tam-m-swim-sched.html"">Schedule</a></li>
			<li><a href="/sports/m-swim/mtt/tam-m-swim-mtt.html"">Roster</a></li>
			<li><a href="/sports/m-swim/spec-rel/m-swim-bests-0708.html"">Stats</a></li>
		</ul>
		<!--[if lt IE 7]></td></tr></table></a><![endif]-->
	</li>
	<li id="top-nav-06"><a href="/sports/m-tennis/tam-m-tennis-body.html" ><span>Men's Tennis</span><!--[if gt IE 6]><!--></a><!--<![endif]-->
			<!--[if lt IE 7]><table><tr><td><![endif]-->
		<ul>
			<li><a href="/sports/m-tennis/sched/tam-m-tennis-sched.html"">Schedule</a></li>
			<li><a href="/sports/m-tennis/mtt/tam-m-tennis-mtt.html"">Roster</a></li>
			<li><a href="http://aggieathletics.cstv.com/photos/schools/tam/sports/m-tennis/auto_pdf/m-tennis-stats-08.pdf">Stats</a></li>
		</ul>
		<!--[if lt IE 7]></td></tr></table></a><![endif]-->
	</li>
	<li id="top-nav-07"><a href="/sports/c-xctrack/tam-c-xctrack-body.html" ><span>Track & Field</span><!--[if gt IE 6]><!--></a><!--<![endif]-->
			<!--[if lt IE 7]><table><tr><td><![endif]-->
		<ul>
			<li><a href="/sports/c-xctrack/sched/tam-c-xctrack-sched.html"">Schedule</a></li>
			<li><a href="/sports/c-xctrack/mtt/tam-c-xctrack-mtt.html"">Roster</a></li>
			<li><a href="/sports/c-xctrack/spec-rel/tam-m-track-2008-results.html"">Stats</a></li>
		</ul>
		<!--[if lt IE 7]></td></tr></table></a><![endif]-->
	</li>
	<li id="top-nav-08"><a href="/sports/w-baskbl/tam-w-baskbl-body.html" ><span>Women's Basketball</span><!--[if gt IE 6]><!--></a><!--<![endif]-->
			<!--[if lt IE 7]><table><tr><td><![endif]-->
		<ul>
			<li><a href="/sports/w-baskbl/sched/tam-w-baskbl-sched.html"">Schedule</a></li>
			<li><a href="/sports/w-baskbl/mtt/tam-w-baskbl-mtt.html"">Roster</a></li>
			<li><a href="http://aggieathletics.cstv.com/sports/w-baskbl/stats/2007-2008/teamcume.html">Stats</a></li>
		</ul>
		<!--[if lt IE 7]></td></tr></table></a><![endif]-->
	</li>
	<li id="top-nav-09"><a href="/sports/w-equest/tam-w-equest-body.html" ><span>Equestrian</span><!--[if gt IE 6]><!--></a><!--<![endif]-->
			<!--[if lt IE 7]><table><tr><td><![endif]-->
		<ul>
			<li><a href="/sports/w-equest/sched/tam-w-equest-sched.html"">Schedule</a></li>
			<li><a href="/sports/w-equest/mtt/tam-w-equest-mtt.html"">Roster</a></li>
			<li><a href="http://aggieathletics.cstv.com/photos/schools/tam/sports/w-equest/auto_pdf/08stats-weq.pdf">Stats</a></li>
		</ul>
		<!--[if lt IE 7]></td></tr></table></a><![endif]-->
	</li>
	<li id="top-nav-10"><a href="/sports/w-golf/tam-w-golf-body.html" ><span>Women's Golf</span><!--[if gt IE 6]><!--></a><!--<![endif]-->
			<!--[if lt IE 7]><table><tr><td><![endif]-->
		<ul>
			<li><a href="/sports/w-golf/sched/tam-w-golf-sched.html"">Schedule</a></li>
			<li><a href="/sports/w-golf/mtt/tam-w-golf-mtt.html"">Roster</a></li>
			<li><a href="http://golfstat.com/ultimate/W112web/W112index.htm" target="_blank">Stats</a></li>
		</ul>
		<!--[if lt IE 7]></td></tr></table></a><![endif]-->
	</li>
	<li id="top-nav-11"><a href="/sports/w-soccer/tam-w-soccer-body.html" ><span>Soccer</span><!--[if gt IE 6]><!--></a><!--<![endif]-->
			<!--[if lt IE 7]><table><tr><td><![endif]-->
		<ul>
			<li><a href="/sports/w-soccer/sched/tam-w-soccer-sched.html"">Schedule</a></li>
			<li><a href="/sports/w-soccer/mtt/tam-w-soccer-mtt.html"">Roster</a></li>
			<li><a href="/sports/w-soccer/stats/2008-2009/teamstat.html"">Stats</a></li>
		</ul>
		<!--[if lt IE 7]></td></tr></table></a><![endif]-->
	</li>
	<li id="top-nav-12"><a href="/sports/w-softbl/tam-w-softbl-body.html" ><span>Softball</span><!--[if gt IE 6]><!--></a><!--<![endif]-->
			<!--[if lt IE 7]><table><tr><td><![endif]-->
		<ul>
			<li><a href="/sports/w-softbl/sched/tam-w-softbl-sched.html"">Schedule</a></li>
			<li><a href="/sports/w-softbl/mtt/tam-w-softbl-mtt.html"">Roster</a></li>
			<li><a href="/sports/w-softbl/stats/2008-2009/teamcume.html"">Stats</a></li>
		</ul>
		<!--[if lt IE 7]></td></tr></table></a><![endif]-->
	</li>
	<li id="top-nav-13"><a href="/sports/w-swim/tam-w-swim-body.html" ><span>Women's Swimming & Diving</span><!--[if gt IE 6]><!--></a><!--<![endif]-->
			<!--[if lt IE 7]><table><tr><td><![endif]-->
		<ul>
			<li><a href="/sports/w-swim/sched/tam-w-swim-sched.html"">Schedule</a></li>
			<li><a href="/sports/w-swim/mtt/tam-w-swim-mtt.html"">Roster</a></li>
			<li><a href="http://aggieathletics.cstv.com/photos/schools/tam/sports/w-swim/auto_pdf/bests-0708.pdf">Stats</a></li>
		</ul>
		<!--[if lt IE 7]></td></tr></table></a><![endif]-->
	</li>
	<li id="top-nav-14"><a href="/sports/w-tennis/tam-w-tennis-body.html" ><span>Women's Tennis</span><!--[if gt IE 6]><!--></a><!--<![endif]-->
			<!--[if lt IE 7]><table><tr><td><![endif]-->
		<ul>
			<li><a href="/sports/w-tennis/sched/tam-w-tennis-sched.html"">Schedule</a></li>
			<li><a href="/sports/w-tennis/mtt/tam-w-tennis-mtt.html"">Roster</a></li>
			<li><a href="http://aggieathletics.cstv.com/photos/schools/tam/sports/w-tennis/auto_pdf/w-tennis-stats-08.pdf">Stats</a></li>
		</ul>
		<!--[if lt IE 7]></td></tr></table></a><![endif]-->
	</li>
	<li id="top-nav-15"><a href="/sports/w-volley/tam-w-volley-body.html" ><span>Women's Volleyball</span><!--[if gt IE 6]><!--></a><!--<![endif]-->
			<!--[if lt IE 7]><table><tr><td><![endif]-->
		<ul>
			<li><a href="/sports/w-volley/sched/tam-w-volley-sched.html"">Schedule</a></li>
			<li><a href="/sports/w-volley/mtt/tam-w-volley-mtt.html"">Roster</a></li>
			<li><a href="/sports/w-volley/stats/2008-2009/teamstat.html"">Stats</a></li>
		</ul>
		<!--[if lt IE 7]></td></tr></table></a><![endif]-->
	</li>
	<li id="top-nav-16"><a href="http://www.12thmanfoundation.com/tickets/ticketbox.asp" target="_blank"><span>Tickets</span></a></li>
	<li id="top-nav-17"><a href="http://www.12thmanfoundation.com/" target="_blank"><span>Donate</span></a></li>
	<li id="top-nav-18"><a href="http://aggieathletics.cstv.com/store/?partner_id=15181"><span>Merchandise</span></a></li>
	<li id="top-nav-19"><a href="http://aggieathletics.cstvauctions.com/gallery.cfm"><span>Auctions</span></a></li>
	<li id="top-nav-20"><a href="/"><span>Home</span></a></li>
	<li id="top-nav-21"><a href="#"><span>TBD</span></a></li>
</ul>
			
		</div>
		<div id="mast-left" class="wrap">
			<div class="wrap">
				<div id="mast-left-logo"></div>
			</div>
			<div class="wrap">
				<div id="mast-left-promo"></div>
			</div>
			<div class="clear"></div>
			<div id="mast-left-nav">
				<ul id="nav">
	<li id="nav-01"><a href="#"><span>Sports</span><!--[if gt IE 6]><!--></a><!--<![endif]-->
			<!--[if lt IE 7]><table><tr><td><![endif]-->
		<ul>
			<li><a href="/sports/m-basebl/tam-m-basebl-body.html">Baseball</a></li>
			<li><a href="/sports/m-baskbl/tam-m-baskbl-body.html">Men's Basketball</a></li>
			<li><a href="/sports/w-baskbl/tam-w-baskbl-body.html">Women's Basketball</a></li>
			<li><a href="/sports/w-equest/tam-w-equest-body.html">Equestrian</a></li>
			<li><a href="/sports/m-footbl/tam-m-footbl-body.html">Football</a></li>
			<li><a href="/sports/m-golf/tam-m-golf-body.html">Men's Golf</a></li>
			<li><a href="/sports/w-golf/tam-w-golf-body.html">Women's Golf</a></li>
			<li><a href="/sports/w-soccer/tam-w-soccer-body.html">Soccer</a></li>
			<li><a href="/sports/w-softbl/tam-w-softbl-body.html">Softball</a></li>
			<li><a href="/sports/m-swim/tam-m-swim-body.html">Men's Swimming & Diving</a></li>
			<li><a href="/sports/w-swim/tam-w-swim-body.html">Women's Swimming & Diving</a></li>
			<li><a href="/sports/m-tennis/tam-m-tennis-body.html">Men's Tennis</a></li>
			<li><a href="/sports/w-tennis/tam-w-tennis-body.html">Women's Tennis</a></li>
			<li><a href="/sports/c-xctrack/tam-c-xctrack-body.html">Track & Field, Cross Country</a></li>
			<li><a href="/sports/w-volley/tam-w-volley-body.html">Volleyball</a></li>
		</ul>
		<!--[if lt IE 7]></td></tr></table></a><![endif]-->
	</li>
	<li id="nav-02"><a href="#"><span>Tickets</span><!--[if gt IE 6]><!--></a><!--<![endif]-->
			<!--[if lt IE 7]><table><tr><td><![endif]-->
		<ul>
			<li><a href="http://www.12thmanfoundation.com/tickets/ticketbox.asp" target="_blank">Buy Tickets</a></li>
			<li><a href="http://sports-admin.tamu.edu/mysportspass" target="_blank">My Sports Pass</a></li>
			<li><a href="http://www.stubhub.com/texas-am-tickets/?GCID=C12289x268-nav&cb=aggies" target="_blank">StubHub Ticket Marketplace</a></li>
            <!--<li><a href="http://www.12thmanfoundation.com/tickets/bs09-sale.asp" target="_blank">Baseball Tickets</a></li> -->
		</ul>
		<!--[if lt IE 7]></td></tr></table></a><![endif]-->
	</li>
	<li id="nav-03"><a href="#"><span>Departments</span><!--[if gt IE 6]><!--></a><!--<![endif]-->
			<!--[if lt IE 7]><table><tr><td><![endif]-->
		<ul>
			<li><a href="http://www.12thmanfoundation.com/" target="_blank">12th Man Foundation</a></li>
			<li><a href="http://www.aggieathletics.com/12thManTeam/" target="_blank">12th Man Team Rewards</a></li>
			<li><a href="/school-bio/ask-bill-byrne.html">Bill Byrne</a></li>
			<li><a href="/camps/tam-camps.html">Camps</a></li>
			<li><a href="http://cisr.tamu.edu/" target="_blank">Charity Request</a></li>
            <li><a href="http://concessions.tamu.edu/" target="_blank">Concessions</a></li>
		  <li><a href="/staffdir/tam-staffdir.html">Staff Directory</a></li>
            <li><a href="http://lettermen.tamu.edu/" target="_blank">Lettermen</a></li>
			<li><a href="/athleteservices/tam-athleteservices.html">Academic Services</a></li>
			<li><a href="/ath-training/tam-ath-training.html">Athletic Training</a></li>
			<li><a href="/compliance/tam-compliance.html">Compliance</a></li>
			<li><a href="http://www.junioraggieclub.com/" target="_blank">Junior Aggie Club</a></li>
			<!-- <li><a href="">Strength/Conditioning</a></li> -->
			<li><a href="/ot/aggie-wheels.html">Aggie Wheels</a></li>
			<li><a href="/sponsorship/tam-sponsorship.html">BCS Team</a></li>
			<li><a href="http://www.learfieldsports.com/gestalt/go.cfm?objectid=BC7B08AD-C290-EABA-63FB549046B2170A" target="_blank">Sponsorship Information</a></li>
		    <li><a href="http://ope.ed.gov/athletics/GetOneInstitutionData.aspx" target="_blank">EADA Reports</a></li>
		</ul>
	  <!--[if lt IE 7]></td></tr></table></a><![endif]-->
	</li>
	<li id="nav-04"><a href="#"><span>Multimedia</span><!--[if gt IE 6]><!--></a><!--<![endif]-->
			<!--[if lt IE 7]><table><tr><td><![endif]-->
		<ul>
			<li><a href="/multimedia/aggie-sports-connection.html">Aggie Sports Connection</a></li>
		    <!-- <li><a href="">Mark Turgeon Show</a></li> -->
			<li><a href="/multimedia/aggies-on-the-radio.html">Aggies on the Radio</a></li>
			<li><a href="/multimedia/bask-radio-network.html">Texas A&M Football Radio Network</a></li>
		    <li><a href="/multimedia/aggies-on-tv.html">Aggies on TV</a></li>
			<li><a href="javascript:window.open('http://all-access.cbssports.com/player.html?code=tam','VideoBroadcastMediaPlayer','toolbar=no,resizable=yes,scrollbars=no,width=1000,height=746' ); void('');">Aggies All-Access</a></li>
            <li><a href="/sports/m-baskbl/spec-rel/aggie-basketball-show.html">Aggie Basketball TV Show</a></li>
            <li><a href="/sports/m-baskbl/spec-rel/turgeon-radio-show.html">Mark Turgeon Radio Show</a></li>
			<li><a href="http://interact.fansonly.com/nl_sign_up/index.cfm?nl_code=tam">Newsletter</a></li>
            <li><a href="/multimedia/outlook-schedules.html">Outlook Schedules</a></li>
		    <li><a href="#" onClick="javascript:window.open('/photogallery/?school=tam', 'PopupViewer', 'toolbar=no,menubar=no,status=no,location=no,directories=no,titlebar=no,scrollbars=yes,resizable=yes,width=970,height=705');return false;">Photo Album</a></li>
			<li><a href="/ot/tam-wallpaper.html">Wallpaper</a></li>
            <li><a href="/genrel/111208aaa.html">2008 Early Signees</a></li>
			<li><a href="http://www.aggieathletics.com/genrel/aggiesinthepros.html">Aggies in the Pro's</a></li>
			<li><a href="http://www.aggieathletics.com/calendar/events/">Composite Calendar </a></li>
			<!-- <li><a href="NEED LINK">Wireless</a></li> -->
			<li><a href="http://www.aggieathletics.com/ot/audio-archive.html" target="_blank">Audio Archive</a></li>
            <li><a href="http://www.aggieathletics.com/ot/tam-all-sports-spec-rel.html">News Archive</a></li>
            <li><a href="http://www.aggieathletics.com/genrel/latestnews.html">Latest Athletics News</a></li>
		  <li><a href="/multimedia/upcoming-live-events.html">Upcoming Live Events</a></li>
		</ul>
	  <!--[if lt IE 7]></td></tr></table></a><![endif]-->
	</li>
	<li id="nav-05"><a href="#"><span>Gameday Info</span><!--[if gt IE 6]><!--></a><!--<![endif]-->
			<!--[if lt IE 7]><table><tr><td><![endif]-->
		<ul>
		    <li><a href="/facilities/tam-facilities.html">Facilities</a></li>
		    <li><a href="http://transport.tamu.edu/" target="_blank">Parking</a></li>
		    <li><a href="/school-bio/tam-visitors-guide.html">Visitor's Guide</a></li>
            <!--<li><a href="/sports/m-footbl/spec-rel/ut.html">Football Gameday Central</a></li>-->
          <li><a href="/sports/m-baskbl/spec-rel/gameday.html">Basketball Gameday Info</a></li>
          <li><a href="/school-bio/aggie-fan-zone.html">Aggie Fan Zone</a></li>
		  <li><a href="/school-bio/in-around-town.html">In & Around Town</a></li>
			<li><a href="http://transport.tamu.edu/" target="_blank">In & Around Campus</a></li>
			<li><a href="/school-bio/tickets-and-venues.html">Tickets and Venues</a></li>
            <li><a href="/genrel/aggiescan.html">Aggies Can</a></li>
			<li><a href="/school-bio/helpful-maps.html">Helpful Maps</a></li>
			<li><a href="/school-bio/gameday-guide.html">Gameday Guide</a></li>
			<li><a href="http://www.visitaggieland.com" target="_blank">Bryan/College Station Convention and Visitors Bureau</a></li>
		</ul>
		<!--[if lt IE 7]></td></tr></table></a><![endif]-->
	</li>
	<li id="nav-06"><a href="#"><span>Students</span><!--[if gt IE 6]><!--></a><!--<![endif]-->
			<!--[if lt IE 7]><table><tr><td><![endif]-->
		<ul>
			<li><a href="/trads/hullabaloo-band.html">Hullabaloo Band</a></li>
			<li><a href="http://tamubands.tamu.edu/" target="_blank">Fightin' Texas Aggie Band</a></li>
            <li><a href="/athleteservices/csa-aai.html">Aggie Athletes Involved</a></li>
            <li><a href="/trads/30-loves.html">Aggie 30-Loves</a></li>
			<li><a href="/trads/aggie-angels.html">Aggie Angels</a></li>
			<li><a href="/trads/aggie-hostesses.html">Aggie Hostesses</a></li>
			<li><a href="/trads/diamond-darlings.html">Diamond Darlings</a></li>
			<li><a href="/trads/danceteam.html">Dance Team</a></li>
		</ul>
		<!--[if lt IE 7]></td></tr></table></a><![endif]-->
	</li>
	<li id="nav-07"><a href="#"><span>Contribute</span><!--[if gt IE 6]><!--></a><!--<![endif]-->
			<!--[if lt IE 7]><table><tr><td><![endif]-->
		<ul>
		    <li><a href="http://www.12thmanfoundation.com/" target="_blank">12th Man Foundation</a></li>
			<!--<li><a href="http://www.bcsaggieqbclub.com/" target="_blank">Aggie Quarterback Club</a></li>-->
			<li><a href="/trads/maroon-club.html">Maroon Club</a></li>
			<!-- <li><a href="">Dugout Club</a></li> -->
			<!-- <li><a href="">Double Play Club</a></li> -->
		</ul>
		<!--[if lt IE 7]></td></tr></table></a><![endif]-->
	</li>
	<li id="nav-08"><a href="#"><span>Shop</span><!--[if gt IE 6]><!--></a><!--<![endif]-->
			<!--[if lt IE 7]><table><tr><td><![endif]-->
		<ul>
			<li><a href="http://aggieathletics.cstvauctions.com/gallery.cfm">Aggie Auctions</a></li>
			<li><a href="http://aggieathletics.cstv.com/store/">Aggie Locker Merchandise</a></li>
		</ul>
		<!--[if lt IE 7]></td></tr></table></a><![endif]-->
	</li>
</ul>
			</div>
		</div>
		<div id="mast-right" class="wrap">
			<div class="wrap">
				<div id="mast-right-buildingchamps"></div>
			</div>
			<div class="wrap">
				<div id="mast-right-left-08">
					
	<div id="multimedia-title" class="wrap">MULTIMEDIA >></div>
	<div class="wrap">
		<ul id="multimedia-nav">
			<li id="multimedia-nav-01" onmouseover="onRoll(this)" onmouseoff="alert('')"><a href="/multimedia/upcoming-live-events.html" ><span>LIVE STATS >></span></a></li>
			<li id="multimedia-nav-02" onmouseover="onRoll(this)"><a href="/multimedia/aggies-on-the-radio.html" ><span>RADIO >></span></a></li>
			<li id="multimedia-nav-03" onmouseover="onRoll(this)"><a href="http://phobos.apple.com/WebObjects/MZStore.woa/wa/viewPodcast?id=191160729" target="_blank"><span>PODCAST >></span></a></li>
			<li id="multimedia-nav-04" onmouseover="onRoll(this)"><a href="javascript:window.open('http://all-access.cbssports.com/player.html?code=tam','VideoBroadcastMediaPlayer','toolbar=no,resizable=yes,scrollbars=no,width=1000,height=746' ); void('');" ><span>WEBCAST >></span></a></li>
			<li id="multimedia-nav-05" onmouseover="onRoll(this)"><a href="/rss/rss-index.html" ><span>RSS >></span></a></li>
			<li id="multimedia-nav-06" onmouseover="onRoll(this)"><a href="#" onClick="javascript:window.open('/photogallery/?school=tam', 'PopupViewer', 'toolbar=no,menubar=no,status=no,location=no,directories=no,titlebar=no,scrollbars=yes,resizable=yes,width=970,height=705');return false;" ><span>GALLERY >></span></a></li>
			<li id="multimedia-nav-07" onmouseover="onRoll(this)"><a href="http://www.aggieathletics.com/wireless/" ><span>WIRELESS >></span></a></li>
			<li id="multimedia-nav-08" onmouseover="onRoll(this)"><a href="http://interact.fansonly.com/nl_sign_up/index.cfm?nl_code=tam" ><span>NEWSLETTER >></span></a></li>
			<li id="multimedia-nav-09" onmouseover="onRoll(this)"><a href="javascript:window.open('http://all-access.cbssports.com/player.html?code=tam','VideoBroadcastMediaPlayer','toolbar=no,resizable=yes,scrollbars=no,width=1000,height=746' ); void('');" ><span>VIDEO >></span></a></li>
		</ul>
	</div>
	<div class="clear"></div>

				</div>
				<div id="mast-right-search">
					<form action="/search/tam-search.html" method="get">
	<div id="search-icon"></div>
	<div id="search-wrap">
		<input id="search-tam" type="text" name="s3q" value="" hidden="50" />
	</div>
	<div id="btn-search-go">
		<input width="24" height="26" border="0" type="image" value="search" src="http://graphics.fansonly.com/schools/tam/graphics/spacer.gif" class="searchsubmit"/>
	</div>
	<div class="clear"></div>
</form>
					
				</div>
			</div>
		</div>
		<div class="clear"></div>
	</div>
	<div id="hdr-bg-sport">
		<img width="181" height="60" src="http://grfx.cstv.com/schools/tam/graphics/tam-08-sports-title-m-footbl.gif"/>
	</div>
	<script>
	document.getElementById("hdr-bg-sport").style.background = 
			"url('http://grfx.cstv.com/schools/tam/graphics/tam-08-hdr-bg-m-footbl.gif') no-repeat";
	</script>
	

















































































































































































































<table id="columns">
        <tr>
            <td id="column-2-sport">
	
				<div id="sport-links">
					<!--<script language="JavaScript" src="http://sports.tamu.edu/cbs_scripts/statbar/bar.php?SID=MFB" type="text/javascript"></script>
<link rel="stylesheet" type="text/css" href="http://www.aggieathletics.com/library/css/statBarStyle.css" />-->

<style type="text/css">
.audio-date {
font-weight: bold;
}
.audio-content {
padding-bottom: 5px;
}
#verticle-nav {
height: 366px;/*Add 16px for every new line*/
}
#infoanchor {
left: 0px; position: relative; top: 0px;
}
#mast-left-promo {
	background:	url("http://grfx.cstv.com/schools/tam/graphics/newbg/tam-08-mast-promospot-thisishome-mfbl.jpg") no-repeat;
	width: 		288px;
	height: 	73px;
}
#sportInfoBar{
		top: -430px; position: absolute; left: 275px; 
		height: 39px;
		width: 622px;
		margin: 0;
		font-family: Verdana, Arial, Helvetica, sans-serif;
		}
	
	#sportInfoBar table{
		margin: 0;
		padding: 0;
		}
	
	#sportInfoBarLeft{
		float:left;
		width: 210px;
		height: 39px;
		margin: 0;
		}
	
	#sportInfoBarRight{
		float:left;
		width: 390px;
		height: 39px;
		margin: 0;
		padding-left: 2px;
		border-left: #b61900 solid 2px;
		}	
	.sportInfoBarTops{
		color:#a87a75;
		font-size: 10px;
		font-weight:bold;
		margin: 0;
		padding: 0;
		}
	
	.sportInfoBarRecord{
		color:#ffffff;
		font-size: 14px;
		font-weight:bold;
		}
	
	.sportInfoBarOpponent{
		color:#ffffff;
		font-size: 12px;
		font-weight:bold;
		margin: 0px;
		}
		
	.sportInfoBarNotes{
		color:#ffffff;
		font-size: 9px;
		font-weight:bold;
		}
	
	.sportInfoBarLinks{
		margin-top: 0px;
		/*margin-left: 65px;*/
		height: 10px;
		z-index: 1;
		color:#d7d7d7;
		font-size: 8px;
		}
	
	.sportInfoBarLinks a{
		margin-left: 5px;
		color:#d7d7d7;
		font-size: 8px;
		}
	
	.sportInfoBarLinks a:hover{
		color:#FFFFFF;
	}
</style>
<ul id="verticle-nav">
	<li class="verticle-nav-01"><a href="/sports/m-footbl/tam-m-footbl-body.html"><span>Football Home</span></a></li>
	<li class="verticle-nav-01"><a href="/sports/m-footbl/spec-rel/tam-m-footbl-spec-rel.html"><span>News</span></a></li>
	<li class="verticle-nav-01"><a href="/sports/m-footbl/sched/tam-m-footbl-sched.html"><span>Schedule/Results</span></a></li>
	<li class="verticle-nav-01"><a href="/sports/m-footbl/stats/2008-2009/teamstat.html"><span>Statistics</span></a></li>
    <li class="verticle-nav-01"><a href="http://www.aggieathletics.com/sports/m-footbl/spec-rel/mfbl-stats-pdf.html"><span>Statistics (PDF)</span></a></li>
    <li class="verticle-nav-01"><a href="/sports/m-footbl/mtt/tam-m-footbl-mtt.html"><span>Roster</span></a></li>
    <li class="verticle-nav-01"><a href="/photos/schools/tam/sports/m-footbl/auto_pdf/depth-chart.pdf"><span>Depth Chart</span></a></li>
	<li class="verticle-nav-01"><a href="/sports/m-footbl/spec-rel/coaching-staff.html"><span>Coaches</span></a></li>
	<li class="verticle-nav-01"><a href="/sports/m-footbl/spec-rel/football-staff.html"><span>Support Staff</span></a></li>
    <!--<li class="verticle-nav-01"><a href="/sports/m-footbl/spec-rel/ut.html"><span>Gameday Central</span></a></li>-->
<li class="verticle-nav-01"><a href="/sports/m-footbl/spec-rel/tam-gamenotes.html"><span>Game Notes</span></a></li>
	<li class="verticle-nav-01"><a href="/sports/m-footbl/spec-rel/tam-m-footbl-history.html"><span>History</span></a></li>
	<li class="verticle-nav-01"><a href="http://www.aggieathletics.com/genrel/aggiesinthenfl.html"><span>Aggies in the NFL</span></a></li>
	<li class="verticle-nav-01"><a href="/multimedia/bask-radio-network.html"><span>Radio Network</span></a></li>
	<li class="verticle-nav-01"><a href="/sports/m-footbl/spec-rel/football-facilities.html"><span>Facilities</span></a></li>
	<li class="verticle-nav-01"><a href="http://www.big12sports.com/SportSelect.dbml?DB_OEM_ID=10410&KEY=&SPID=13139&SPSID=106181" target="_blank"><span>Big 12 Standings</span></a></li>
	<li class="verticle-nav-01"><a href="http://www.aggieathletics.com/camps/football/"><span>Camps</span></a></li>
<!--	<li class="verticle-nav-01"><a href="/sports/m-footbl/spec-rel/aggies-in-the-nfl.html"><span>Aggies in the NFL</span></a></li> -->
	<li class="verticle-nav-01"><a href="javascript:window.open('http://aggieathletics.cstv.com/sports/m-footbl/spec-rel/aggie-online-window-003.html','VideoBroadcastMediaPlayer','toolbar=no,resizable=no,scrollbars=no,width=910,height=600' ); void('');"><span>12thManOnline.com</span></a></li>
	<li class="verticle-nav-01"><a href="/school-bio/gameday-guide.html"><span>Gameday Guide</span></a></li>
	<!-- <li class="verticle-nav-01"><a href="/sports/m-footbl/spec-rel/coaches-clinic-08.html"><span>Coaches Clinic</span></a></li> -->
	<li class="verticle-nav-01"><a href="http://www.aggiecushion.com/" target="_blank"><span>Aggie Seat Cushion</span></a></li>
	<li class="verticle-nav-01"><a href="https://secure.assistantcoach.net/TicketSystem/Default.aspx?id=1&ORG_ID=4" target="_blank"><span>HS Coaches' Ticket Request</span></a></li>
	<li class="verticle-nav-01"><a href="/sports/m-footbl/spec-rel/sherman-radio-show.html"><span>Mike Sherman Radio Show</span></a></li>
<li class="verticle-nav-01"><a href="/sports/m-footbl/spec-rel/tam-m-footbl-psa-questionnaire.html"><span>PSA Questionnaire</span></a></li>
		<!-- <li><a href="/sports/m-footbl/spec-rel/tam-m-footbl-quick-facts.html">Quick Facts</span></a></li>
<li><a href="http://www.aggieathletics.com/sports/m-footbl/spec-rel/ut.html"><span id="gdhomeimage" style="top: 10px; position:relative; left:-1px"><img src="http://grfx.cstv.com/schools/tam/graphics/gdhomeimage.jpg" width="179" height="60" border="0" /></span></a> -->
</ul>
<!--<div id="infoanchor">
<script type="text/javascript">writeBar();</script>
</div>-->
<div id="infoanchor">
<div id="sportInfoBar">
<div id="sportInfoBarLeft">
<table width="100%" border="0" cellspacing="0" cellpadding="0">
  <tr>
    <td><span class="sportInfoBarTops">RANK: NR <!--(AP &amp; ESPN/USA TODAY)--></span></td>
  </tr>
  <tr>
    <td><span class="sportInfoBarRecord">0-0, 0-0</span> <span class="sportInfoBarNotes">[BIG 12]</span></td>
  </tr>
</table>
</div>
<div id="sportInfoBarRight">
<table width="375" border="0" cellspacing="0" cellpadding="0">
  <tr>
    <td width="31%"><span class="sportInfoBarTops">UP NEXT</span></td>
    <td width="52%">&nbsp;</td>
    <td width="17%">&nbsp;</td>
  </tr>
  <tr>
    <td><span class="sportInfoBarNotes">09/05</span></td>
    <td><span class="sportInfoBarOpponent">vs New Mexico <!--<a href="http://www.aggieathletics.com/multimedia/aggies-on-tv.html"><img src="http://grfx.cstv.com/graphics/teams/icons/media-icon-tv.gif" alt="ESPN" border="0" /></a>--></span></td>
    <td><span class="sportInfoBarNotes">TBA</span></td>
  </tr>
  <tr>
    <td></td>
    <td align="left"><span class="sportInfoBarLinks"><a href="javascript:window.open('http://aggieathletics.cstv.com/gametracker/launch/?event=729828&amp;school=tam&amp;sport=mfootbl&amp;','GameTrackerv3','toolbar=no,resizeable=no,scrollbars=no,width=976,height=700');%20void(0);">LIVE STATS </a> <a href="#">LIVE AUDIO</a> <!--<a href="http://sports.espn.go.com/broadband/espn360/index?id=290140245" target="_blank">ESPN360</a>--></span></td>
    <td>&nbsp;</td>
  </tr>
</table>

</div>
</div>
</div>

				</div>
				
				<div id="store-sport">
					<div id="store-portal">
	<script type="text/javascript">
		<!--
		var portalSWF = "http://grfx.cstv.com/schools/cs/store/07_oas_storeportal.swf";

		/* CHANGE OUT THESE 4 VARS TO CUSTOMIZE */
		var portal_width = 181;
		var portal_height = 164;
		var portal_thumb_height = 100; //ALWAYS START AT 100
		var portal_school = "tam";
		
		var portal_sport = "m-footbl"; //leave blank for index page
		
		/* THE TXT FILE TO CUSTOMIZE COLORS SHOULD BE PUT IN THE SCHOOL'S "store" DIRECTORY - SEE TACO FOR AN EXAMPLE */

		var flashSWF = portalSWF + "?area_wd=" + portal_width + "&area_ht=" + portal_height + "&thumb_ht=" + portal_thumb_height + "&portal_school=" + portal_school + "&portal_sport=" + portal_sport + "&&"; //full url string - path to swf, passing any vars

		var flashWidth = portal_width;
		var flashHeight = portal_height;
		var flashVersion = 8;
		var flashScale = "noscale";
		var flashWmode = "transparent";
		var flashBkgCol = "CCCCCC";
		var flashDefault = '<a href="http://www.macromedia.com/shockwave/download/index.cgi?P1_Prod_Version=ShockwaveFlash" target="_blank"><img src="http://graphics.cstv.com/store/.gif" width="'+flashWidth+'" height="'+flashHeight+'" border="0" alt="Click here to download the latest version of the Flash plugin to view this content"></a>';

		//-->
	</script>
	<script type="text/javascript" src="http://graphics.fansonly.com/scripts/flash-embed2.js"></script>

</div><!-- #store-portal -->
				</div>
				
				<div id="column-2-auctions-sport">
					<div id="column-2-hdr-auctions-sport">
						<a href="http://aggieathletics.cstvauctions.com/gallery.cfm">
							<img width="179" height="18" alt="" src="http://grfx.cstv.com/schools/tam/graphics/spacer.gif"/>								
						</a>
					</div>
					<div id="auctions-portal">
	 <script type="text/javascript" language="JavaScript">
	<!--
	var auctionsSWF = "http://grfx.cstv.com/schools/cbs/store/08_oas_auctionsportal.swf?3";
	
	/* CHANGE OUT THESE 4 VARS TO CUSTOMIZE */
	var auctions_width = 179;
	var auctions_height = 124;
	var auctions_school = "tam";
	var auctions_sport = "m-footbl"; //leave blank for index page
	/* THE TXT FILE TO CUSTOMIZE COLORS SHOULD BE PUT IN THE SCHOOL'S "store" DIRECTORY - SEE TACO FOR AN EXAMPLE */
	
	var auctionsVars = "area_wd=" + auctions_width + "&area_ht=" + auctions_height + "&portal_school=" + auctions_school + "&portal_sport=" + auctions_sport + "&"; //full url string - path to swf, passing any vars
	
	var auctionsWidth = auctions_width;
	var auctionsHeight = auctions_height;
	var auctionsVersion = 8;
	var auctionsScale = "noscale";
	var auctionsWmode = "transparent";
	var auctionsBkgCol = "CCCCCC";
	var auctionsDefault = '<div align="center" style="margin:5px"><a href="http://www.macromedia.com/shockwave/download/index.cgi?P1_Prod_Version=ShockwaveFlash" target="_blank">Click here to download the latest version of the Flash plugin to view this content</a></div>';
	
	//-->
	</script>
	<script type="text/javascript" language="javascript" src="http://grfx.cstv.com/schools/cbs/store/08_oas_auctionsportal.js"></script>

</div><!-- #auction-portal -->
				</div>
				<img src="http://graphics.fansonly.com/graphics/spacer.gif" width=1 height=5 border=0 alt="">
				<div align="right">
				<a href="http://www.stubhub.com/texas-am-tickets/?GCID=C12289x268-football&cb=aggies">
				<img src="http://grfx.cstv.com/schools/tam/graphics/aggies_179x90_mes3bl.gif" border="0"></a>
				</div>
				
				<div id="column-2-grfx-links">
					<a href="/ot/physicians-guestcoach.html"><img src="http://grfx.cstv.com/schools/tam/graphics/physicians-180x120.gif" alt="Nominate a Professor" /></a>
<a href="/sports/m-footbl/spec-rel/eccell-entertowin.html"><img src="http://grfx.cstv.com/schools/tam/graphics/eccellgroup-180x120.gif" alt="Eccell Group - Enter To Win" /></a>
<a href="/sports/m-footbl/spec-rel/eureka-giveaway08.html"><img src="http://grfx.cstv.com/schools/tam/graphics/eurekaChairs_180x120gif.gif" alt="Eureka Chairs - Enter To Win" /></a>
<a href="http://www.benjaminknox.com" target="_blank"><img src="http://grfx.cstv.com/schools/tam/graphics/benKnox_180x60.jpg" alt="Benjamin Knox" /></a>
<!--<a href="/sports/m-footbl/spec-rel/texaco-playofthegame.html"> <img src="http://grfx.cstv.com/schools/tam/graphics/texaco_playofthegame180.gif" alt="Texaco Play of the Game" /></a> -->

				</div>
				
            </td><!-- #column-2 -->
            <td id="column-1">

				<!-- *********************** BEGIN PHOTO *********************** -->
<div id="column-1-bsi-top">
	<a href="javascript:goToStory();">
		<img id="frame_photo" name="frame_photo" src="http://grfx.cstv.com/photos/schools/tam/sports/m-footbl/auto_wide_photo/2759119.jpeg" width="464" height="271" border="0" alt="">
	</a>
	<div id="relativeFrame">
		<div id="frm0">
			<span class="rF-date">
				<p class="pubdate">Football, 2/17/2009</p>
			</span>
			<span class="rF-headline">
				<h1><a href="/sports/m-footbl/spec-rel/021709aaa.html">Four Aggies Set to Participate at NFL Combine</a></h1>
			</span>
			<span class="rF-synopsis">
				Four Aggies will work out for NFL coaches and front office personnel at this week's NFL Combine in Indianapolis. Justin Brantly, Michael Bennett, Mike Goodson and Stephen McGee have been invited by the league to...
			</span>
		</div>
	
		<div id="frm1">
			<span class="rF-date">
				<p class="pubdate">Football, 2/11/2009</p>
			</span>
			<span class="rF-headline">
				<h1><a href="/sports/m-footbl/spec-rel/021109aac.html">AGGIE FOOTBALL TEAM ATTENDS DISTINGUISHED LECTURE SERIES</a></h1>
			</span>
			<span class="rF-synopsis">
				The Texas A&M football team attended the University Distinguished Lecture Series on Tuesday, Feb. 10 at 7:30 p.m. in the Anneberg Presidential Conference Center on the Texas A&M campus.
			</span>
		</div>
	
		<div id="frm2">
			<span class="rF-date">
				<p class="pubdate">Football, 2/4/2009</p>
			</span>
			<span class="rF-headline">
				<h1><a href="/sports/m-footbl/spec-rel/020409aab.html">Aggies Ink 28 on National Signing Day</a></h1>
			</span>
			<span class="rF-synopsis">
				Aggie head football coach Mike Sherman announced the signing of 28 student-athletes to national letters of intent in a press conference Wednesday on the Texas A&M campus.
			</span>
		</div>
	
		
		<div id="frm3">
			<span class="rF-date">
				<p class="pubdate">Football, 2/11/2009</p>
			</span>
			<span class="rF-headline">
				<h1><a href="/sports/m-footbl/spec-rel/021109aae.html">Funeral Arrangements Set for Former A&M Great</a></h1>
			</span>
			<span class="rF-synopsis">
				McLean was a record-setting receiver for the Aggies from 1962-63 and 1965. Forty-three seasons after his final game in the maroon and white, McLean still holds the school standard for receiving yards in a single game.
			</span>
		</div>
		
	</div>
</div>
<!-- *********************** END PHOTO *********************** -->



<!-- *********************** BEGIN HEADLINE PANELS *********************** -->
<div id="column-1-bsi-minis">
<!-- *********************** END HEADLINE PANELS *********************** -->

<!-- *********************** BEGIN MINI GROUP *********************** -->
<div id="minis">
	<div id="mini0" class="miniOn">
		<a href="#" onClick="rotateToPanel(0);return false;"><img src="http://grfx.cstv.com/photos/schools/tam/sports/m-footbl/auto_thumb_photo/2759122.jpeg" width="157" height="83" border="0" alt=""></a>
		<a href="/sports/m-footbl/spec-rel/021709aaa.html">Four Aggies Set to Participate...</a>
	</div>
	
	<div id="mini1">
		<a href="#" onClick="rotateToPanel(1);return false;"><img src="http://grfx.cstv.com/photos/schools/tam/sports/m-footbl/auto_thumb_photo/2739194.jpeg" width="157" height="83" border="0" alt=""></a>
		<a href="/sports/m-footbl/spec-rel/021109aac.html">AGGIE FOOTBALL TEAM ATTENDS...</a>
	</div>
	
	<div id="mini2">
		<a href="#" onClick="rotateToPanel(2);return false;"><img src="http://grfx.cstv.com/photos/schools/tam/sports/m-footbl/auto_thumb_photo/2714379.jpeg" width="157" height="83" border="0" alt=""></a>
		<a href="/sports/m-footbl/spec-rel/020409aab.html">Aggies Ink 28 on National...</a>
	</div>
		
	<div id="mini3">
		<a href="#" onClick="rotateToPanel(3);return false;"><img src="http://grfx.cstv.com/photos/schools/tam/sports/m-footbl/auto_thumb_photo/2740499.jpeg" width="157" height="83" border="0" alt=""></a>
		<a href="/sports/m-footbl/spec-rel/021109aae.html">Funeral Arrangements Set for...</a>
	</div>
</div>
<div class="clear"></div>

<div id="controls">
	<ul>
		<li><a href="#" onclick="javascript:rotateBack(); return false;"><img src="http://graphics.fansonly.com/schools/wash/graphics/wash-07-bsi-back.gif" alt="Back" /></a></li>
		<li><a href="#" onclick="changeImage(); return false;"><img src="http://graphics.fansonly.com/schools/wash/graphics/wash-07-bsi-pause.gif" alt="PlayOrStop" name="playOrstop" /></a></li>
		<li><a href="#" onclick="javascript:rotateForward(); return false;"><img src="http://graphics.fansonly.com/schools/wash/graphics/wash-07-bsi-forward.gif" alt="Next" /></a></li>
	</ul>
</div><!-- #controls -->
<div class="clear"></div>

<div id="sportnav-wrap">
	<ul id="sportnav">
		<li><a href="/sports/m-footbl/headline-rss.xml"><img align="top" src="http://grfx.cstv.com/schools/tam/graphics/tam-08-btn-sports-rss.gif" width="37" height="13" alt="RSS"></a></li>
		<li><a href="/sports/m-footbl/spec-rel/tam-m-footbl-spec-rel.html">All Football Press Releases</a>&nbsp;&nbsp;|</li>
		<li>&nbsp;&nbsp;<a href="/sports/m-footbl/sched/tam-m-footbl-sched.html">Schedule</a>&nbsp;&nbsp;|</li>
		<li class="first">&nbsp;&nbsp;<a href="/sports/m-footbl/mtt/tam-m-footbl-mtt.html">Roster</a>&nbsp;&nbsp;|</li>
		<li>&nbsp;&nbsp;<a href="/sports/m-footbl/archive/tam-m-footbl-archive.html">Archives</a></li>
		<li class="aggiecreditunion">
			<a href="http://www.csmedcenter.com/" target="_new">
				<img src="http://grfx.cstv.com/schools/tam/graphics/spacer.gif" align="top" width="130" height="40" alt="College Station Medical Center">
			</a>
		</li>
	</ul>
</div>
<div class="clear"></div>

</div>
<div id="column-1-bsi-bottom"></div>

<!-- *********************** END MINI GROUP *********************** -->

<SCRIPT language="JavaScript">
<!-- 

//////////////// BEGIN BSI VARS /////////////
//var bsiMiniOnBg = "CC0000";
//var bsiMiniOffBg = "440000";
var bsiRotationTime = 6500;
//////////////// END BSI VARS /////////////

////////////////// BEGIN TELL HEIGHT ////////////////////
function tellHeight(){
	//document.getElementById('mini0').style.backgroundColor = bsiMiniOnBg;
	document.getElementById('mini0').className="miniOn";

	var i = framePanels-1;

var layerHeights = [];
if(document.getElementById('frm0')) layerHeights[0] = document.getElementById('frm0').offsetHeight;
if(document.getElementById('frm1')) layerHeights[1] = document.getElementById('frm1').offsetHeight;
if(document.getElementById('frm2')) layerHeights[2] = document.getElementById('frm2').offsetHeight;
if(document.getElementById('frm3')) layerHeights[3] = document.getElementById('frm3').offsetHeight;
		/*
		var layerHeights=[
		document.getElementById('frm0').offsetHeight,
		document.getElementById('frm1').offsetHeight,
		document.getElementById('frm2').offsetHeight];
		*/
		if(layerHeights[layerHeights.length-1] > 0){
			function compare(a,b){
			return b - a;
}
		layerHeights.sort(compare);
		//alert(layerHeights);
		document.getElementById('relativeFrame').style.height =  layerHeights[0] + "px";
		clearInterval(mwcBoxAdjust);

//		rotate_timer = setInterval("toggleFlip()",bsiRotationTime);
		do{
		document.getElementById("mini"+i).style.visibility="visible";
		}while(i--);
		}

}
////////////////// END TELL HEIGHT ////////////////////

if(!document.layers){
var layerHeights=[document.getElementById('frm0')];

}else{
oLayer = window.document.relativeFrame;
var layerHeights=[oLayer.document.frm0.clip.height,oLayer.document.frm1.clip.height,oLayer.document.frm2.clip.height,oLayer.document.frm3.clip.height];
}

layerHeights.sort()
var divAreaWidth=375;
var divAreaHeight = layerHeights[layerHeights.length-1];
if(document.layers){
document.writeln('<img src="http://grfx.cstv.com/graphics/spacer.gif" width="'+divAreaWidth+'" height="'+divAreaHeight+'" border="0" alt="">');
}


//var getMyRelFrame = document.getElementById("relativeFrame");
var getMyDivCount = document.getElementById("relativeFrame").getElementsByTagName("div");
var panels_i = getMyDivCount.length-1;
var getMyPanelCount=0;
do{
	if(getMyDivCount[panels_i].id.indexOf("frm") != -1){
	getMyPanelCount++;
	}
}while(panels_i--);


//alert(getMyPanelCount);




var framePanels=getMyPanelCount;  // set you base panel number here
var oasPrimeToggle = 0;
var oasPrimeToggleState = 0;
var currActiveFrame = 0;
var rotate_timer;

var bsiPhotosArray =[];
bsiPhotosArray[0] = 'http://grfx.cstv.com/photos/schools/tam/sports/m-footbl/auto_wide_photo/2759119.jpeg';
bsiPhotosArray[1] = 'http://grfx.cstv.com/photos/schools/tam/sports/m-footbl/auto_wide_photo/2739191.jpeg';
bsiPhotosArray[2] = 'http://grfx.cstv.com/photos/schools/tam/sports/m-footbl/auto_wide_photo/2717210.jpeg';
bsiPhotosArray[3] = 'http://grfx.cstv.com/photos/schools/tam/sports/m-footbl/auto_wide_photo/2740491.jpeg';

var bsiPhotoUrls =[];
bsiPhotoUrls[0] = '/sports/m-footbl/spec-rel/021709aaa.html';
bsiPhotoUrls[1] = '/sports/m-footbl/spec-rel/021109aac.html';
bsiPhotoUrls[2] = '/sports/m-footbl/spec-rel/020409aab.html';
bsiPhotoUrls[3] = '/sports/m-footbl/spec-rel/021109aae.html';



var last =0;
function goToStory(){
parent.window.location = bsiPhotoUrls[currActiveFrame];
}//end func

function toggleFlip(currFrame,lastFrame){
rotate(oasPrimeToggle);

last=oasPrimeToggle;
oasPrimeToggleState++;  // if you want toggle to start at Zero, place this statement below:  toggle = toggleState%5;

oasPrimeToggle = oasPrimeToggleState%framePanels;
}

function rotate(currFrame){
currActiveFrame = currFrame;
var i = framePanels-1;
if(!document.layers){
do{
document.getElementById('frm'+i).style.visibility="hidden";
document.getElementById("mini"+i).className="";
//document.getElementById("mini"+i).style.backgroundColor=bsiMiniOffBg;


}while(i--);
document.getElementById('frm'+currFrame).style.visibility="visible";
document.getElementById("mini"+currFrame).className="miniOn";
document.getElementById('frame_photo').src = bsiPhotosArray[currFrame];
//document.getElementById('lead-photo').style.backgroundImage =   'url(' +bsiPhotosArray[currFrame]+ ')';
//document.getElementById("mini"+currFrame).style.backgroundColor=bsiMiniOnBg;
//document.getElementById('numbers').src = "http://grfx.cstv.com/confs/c-usa/graphics/c-usa-07-bsi-nav-" + (currFrame+1) + ".gif";
}

last=currFrame;

//alert(last);
}

function rotateToPanel(panel){
oasPrimeToggle=panel;
oasPrimeToggleState=panel;
clearInterval(rotate_timer);
changeImage(1);
rotate(oasPrimeToggle);

}

///////////////////// display minis /////////////////////
var bsiAddMinis = getMyPanelCount-1;
do{
	document.getElementById("mini"+bsiAddMinis).style.display="block";
	//document.getElementById("mini"+bsiAddMinis).style.backgroundColor = bsiMiniOffBg;
}while(bsiAddMinis--);
///////////////////// display minis /////////////////////


///////////////////////////////////////// INITIALIZE /////////////////////////////////////////
if(!document.layers){
//mwcBoxAdjust = setInterval("tellHeight()",500);
}else{
//rotate_timer = setInterval("toggleFlip()",bsiRotationTime);
}
///////////////////////////////////////// INITIALIZE /////////////////////////////////////////


/////////////////////////////////////////////////////////////////////
function rotateBack(){
clearInterval(rotate_timer);
if(last==0){last = framePanels-1;}else{last--;}
changeImage(1);
rotate(last);
}//end func

/////////////////////////////////////////////////////////////////////
function rotateForward(){
clearInterval(rotate_timer);
if(last==framePanels-1){last = 0;}else{last++;}
changeImage(1);
rotate(last);
}//end func


/////////////////////////////////////////////////////////////////////
function rotatePlay(){
if(last==framePanels-1){oasPrimeToggle = 0;}
//rotate_timer = setInterval("toggleFlip()",bsiRotationTime);
}//end func


/////////////////////// CONTROLS //////////////////	
		pr_imgroll = new Array();
		pr_imgroll[0] = "";// ok to leave empty
		pr_imgroll[1] = "back";
		pr_imgroll[2] = "forward";
		pr_onimages = [];
		pr_offimages = [];
		for(i=0; i < pr_imgroll.length; i++){
			pr_onimages[i] =  new Image();
			pr_onimages[i].src = "http://graphics.fansonly.com/schools/wash/graphics/wash-07-bsi-"+pr_imgroll[i]+".gif";
			pr_offimages[i] =  new Image();
			pr_offimages[i].src = "http://graphics.fansonly.com/schools/wash/graphics/wash-07-bsi-"+pr_imgroll[i]+".gif";
		}
		function bsiImgOn(imageName) {
			num = imageName.split("e");
			document[imageName].src = pr_onimages[num[1]].src;  
		}
		function bsiImgOff(imageName) {  
			num = imageName.split("e");
			document[imageName].src = pr_offimages[num[1]].src;
		}

		oasToggle = 0;
		oasToggleState = 0;

		playButtons = [];
		playButtons[0] = new Image;
		playButtons[0].src = "http://graphics.fansonly.com/schools/wash/graphics/wash-07-bsi-play.gif";
		playButtons[1] = new Image;
		playButtons[1].src = "http://graphics.fansonly.com/schools/wash/graphics/wash-07-bsi-play.gif";

		stopButtons = [];
		stopButtons[0] = new Image;
		stopButtons[0].src = "http://graphics.fansonly.com/schools/wash/graphics/wash-07-bsi-pause.gif";
		stopButtons[1] = new Image;
		stopButtons[1].src = "http://graphics.fansonly.com/schools/wash/graphics/wash-07-bsi-pause.gif";

		ButtonGroups = [];
		ButtonGroups[0] = stopButtons;
		ButtonGroups[1] = playButtons;

		function changeImage(state) {
		oasToggleState++;  // if you want oasToggle to start at Zero, place this statement below:  oasToggle = oasToggleState%5;
		if(state){
		oasToggleState = state;
		toggle=last;
		toggleState=last;
		}
		oasToggle = oasToggleState%2;
		document['playOrstop'].src = ButtonGroups[oasToggle][0].src;//Number one needs to become oasToggle
		if(oasToggle==0){
		rotatePlay()
		}else{
		clearTimeout(rotate_timer)
		}

		}

		function rollOver(state){
		document['playOrstop'].src = ButtonGroups[oasToggle][state].src;//Number one needs to become oasToggle
		}
		
		//rotate_timer = setInterval("toggleFlip()",bsiRotationTime);
//--> 
</script>
                <div style="margin-left: 18px;"><a href="http://signingday.aggieathletics.com"><img src="http://grfx.cstv.com/schools/tam/graphics/studentspot_signingday09.gif" /></a></div>

				<div id="column-1-middle">
					
					<div class="wrap">
						<div id="column-1-middle-hdr-fancenter">
	<div class="wrap" style="width: 293px; height:31px;">
	</div>
	<div class="wrap" style="width: 100px; height: 31px">
		<a href="http://www.verizonwireless.com/" target="_new">
			<div class="image-block">
				<img src="http://grfx.cstv.com/schools/tam/graphics/spacer.gif" width="100px" height="30px" alt="Verizon">
			</div> 
		</a>
	</div>
	<div class="clear"></div>
</div>
<div id="column-1-middle-fancenter">
	<div id="all-access-tab" class="on-all-access">
	    <ul class="headline-1">
	        <li id="tab-all-access"><a href="#" onclick="allAccessTabs('on-all-access', this.href); return false;"><span>All Access</span></a></li>
	        <li id="tab-audio"><a href="#" onclick="allAccessTabs('on-audio', this.href); return false;"><span>Audio</span></a></li>
			<li id="tab-photo-gallery"><a href="#" onclick="allAccessTabs('on-photo-gallery', this.href); return false;"><span>Photo Gallery</span></a></li>
			<li id="tab-in-the-news"><a href="#" onclick="allAccessTabs('on-in-the-news', this.href); return false;"><span>In The News</span></a></li>
	    </ul>
	    <div id="all-access" >
			<div id="video-cover" class="wrap">
				<div style="display:none;"><img src="http://graphics.cstv.com/cgi-bin/set_throughput_cookie.cgi" height="1" width="1" border="0" alt="cookie"></div>

<div id="video">

	<script type="text/javascript"><!--
	var videoContaner = {
	vidabbrev : "msu",
	aaVid : "http://mfile.akamai.com/27612/wmv/cstvcbs.download.akamai.com/8108/open/tam/08-09/tam_files/audio-video/football.wmv",
	aaImg : 'http://grfx.cstv.com/schools/tam/graphics/tam-08-video-cover.gif',
	aaW : "232",
	aaH : "185",
	videoAutoPlay : false,
	asControls : "y",
	aaStatusBar : "n",
	aAd : "&bumper=both" // make this string EMPTY to restore FREQUENCY CAPPING - first, last or both otherwise
	};
	// -->
	</script>

	<script src="http://graphics.cstv.com/library/oas/highlights/video-global.js" type="text/javascript"></script>

</div><!-- #video -->



			</div>
			<div id="hdr-all-access" class="wrap">
				<div id="hdr-all-access-wrap">
					<a href="javascript:window.open('http://all-access.cbssports.com/player.html?code=tam&media=94174','VideoBroadcastMediaPlayer','toolbar=no,resizable=yes,scrollbars=no,width=1000,height=746' ); void('');" class="more">Game:  @ Texas <img class="camera" src="http://graphics.collegesports.com/graphics/allaccess/camera.gif"/></a>
					<br>
				  <a href="javascript:window.open('http://all-access.cbssports.com/player.html?code=tam&media=91722','VideoBroadcastMediaPlayer','toolbar=no,resizable=yes,scrollbars=no,width=1000,height=746' ); void('');" class="more">Game:  @ Baylor <img class="camera" src="http://graphics.collegesports.com/graphics/allaccess/camera.gif"/></a>
					<br>
				  <a href="javascript:window.open('http://all-access.cbssports.com/player.html?code=tam&media=89066','VideoBroadcastMediaPlayer','toolbar=no,resizable=yes,scrollbars=no,width=1000,height=746' ); void('');" class="more">Game:  vs Oklahoma <img class="camera" src="http://graphics.collegesports.com/graphics/allaccess/camera.gif"/></a>
					<br>
				  <a href="javascript:window.open('http://all-access.cbssports.com/player.html?code=tam&media=85516','VideoBroadcastMediaPlayer','toolbar=no,resizable=yes,scrollbars=no,width=1000,height=746' ); void('');" class="more">Game: vs Colorado <img class="camera" src="http://graphics.collegesports.com/graphics/allaccess/camera.gif"/></a>
					<br>
				  <a href="javascript:window.open('http://all-access.cbssports.com/player.html?code=tam&media=84482','VideoBroadcastMediaPlayer','toolbar=no,resizable=yes,scrollbars=no,width=1000,height=746' ); void('');" class="more">Game:  @ Iowa St.  <img class="camera" src="http://graphics.collegesports.com/graphics/allaccess/camera.gif"/></a>			  </div>
				<a href="javascript:window.open('http://all-access.cbssports.com/player.html?code=tam','VideoBroadcastMediaPlayer','toolbar=no,resizable=yes,scrollbars=no,width=1000,height=746' ); void('');">
					<img src="http://grfx.cstv.com/schools/tam/graphics/tam-08-btn-all-access-2.gif" width="140" height="34">
				</a>
			</div>
			<div class="clear"></div>
		</div>
	    <div id="audio" style="display:none">
			

<div class="audio-date">02/02/09</div>
<div class="audio-content">- <a href="http://cstvpodcast.cstv.com.edgesuite.net/texasam/090204_signingday.mp3">Football Signing Day: Mike Sherman Press Conference</a></div>

<div class="audio-date">12/5/2008</div>
<div class="audio-content">- <a href="http://cstvpodcast.cstv.com.edgesuite.net/texasam/081205_sherman.mp3">Football Press Conference - Head Coach Mike Sherman</a></div>

<div class="audio-date">11/27/2008</div>
<div class="audio-content">- <a href="http://cstvpodcast.cstv.com.edgesuite.net/texasam/081127_001.mp3">Football vs. Texas Postgame - Mike Sherman</a></div>

<div class="audio-date">11/27/2008</div>
<div class="audio-content">- <a href="http://cstvpodcast.cstv.com.edgesuite.net/texasam/081127_002.mp3">Football vs. Texas Postgame - Alton Dixon</a></div>

<div class="audio-date">11/27/2008</div>
<div class="audio-content">- <a href="http://cstvpodcast.cstv.com.edgesuite.net/texasam/081127_003.mp3">Football vs. Texas Postgame - Trent Hunter</a></div>

<div class="audio-date">11/27/2008</div>
<div class="audio-content">- <a href="http://cstvpodcast.cstv.com.edgesuite.net/texasam/081127_004.mp3">Football vs. Texas Postgame - Stephen McGee</a></div>

<div class="audio-date">11/20/2008</div>
<div class="audio-content">- <a href="http://cstvpodcast.cstv.com.edgesuite.net/texasam/081120_sherman.mp3">Texas Media Luncheon - Coach Mike Sherman</a></div>

<div class="audio-date">11/20/2008</div>
<div class="audio-content">- <a href="http://cstvpodcast.cstv.com.edgesuite.net/texasam/081120_kines.mp3">Texas Media Luncheon - Coach Joe Kines</a></div>

<div class="audio-date">11/20/2008</div>
<div class="audio-content">- <a href="http://cstvpodcast.cstv.com.edgesuite.net/texasam/081120_hunter.mp3">Texas Media Luncheon - Trent Hunter</a></div>

<div class="audio-date">11/20/2008</div>
<div class="audio-content">- <a href="http://cstvpodcast.cstv.com.edgesuite.net/texasam/081120_mcgee.mp3">Texas Media Luncheon - Stephen McGee</a></div>

<div class="audio-date">11/20/2008</div>
<div class="audio-content">- <a href="http://cstvpodcast.cstv.com.edgesuite.net/texasam/081120_schneider.mp3">Texas Media Luncheon - Travis Schneider</a></div>

<div class="audio-date">11/20/2008</div>
<div class="audio-content">- <a href="http://cstvpodcast.cstv.com.edgesuite.net/texasam/081120_bennett_pugh.mp3">Texas Media Luncheon - Michael Bennett &amp; Jordan Pugh</a></div>

<div class="audio-date">11/20/2008</div>
<div class="audio-content">- <a href="http://cstvpodcast.cstv.com.edgesuite.net/texasam/081120_featherston.mp3">Texas Media Luncheon - Matt Featherston</a></div>

<div class="audio-date">11/20/2008</div>
<div class="audio-content">- <a href="http://cstvpodcast.cstv.com.edgesuite.net/texasam/081120_fuller.mp3">Texas Media Luncheon - Jeff Fuiller</a></div>

<div class="audio-date">11/20/2008</div>
<div class="audio-content">- <a href="http://cstvpodcast.cstv.com.edgesuite.net/texasam/081120_johnson.mp3">Texas Media Luncheon - Jerrod Johnson</a></div>

<div class="audio-date">11/15/2008</div>
<div class="audio-content">- <a href="http://cstvpodcast.cstv.com.edgesuite.net/texasam/081115_sherman.mp3">Football vs. Baylor Postgame Audio - Head Coach Mike Sherman</a></div>

<div class="audio-date">11/15/2008</div>
<div class="audio-content">- <a href="http://cstvpodcast.cstv.com.edgesuite.net/texasam/081115_johnson.mp3">Football vs. Baylor Postgame Audio - QB Jerrod Johnson</a></div>

<div class="audio-date">11/15/2008</div>
<div class="audio-content">- <a href="http://cstvpodcast.cstv.com.edgesuite.net/texasam/081115_gray.mp3">Football vs. Baylor Postgame Audio - RB Cyrus Gray</a></div>
<div class="audio-date">11/15/2008</div>
<div class="audio-content">- <a href="http://cstvpodcast.cstv.com.edgesuite.net/texasam/081115_featherston.mp3">Football vs. Baylor Postgame Audio - LB Matt Featherston</a></div>

<div class="audio-date">11/3/2008</div>
<div class="audio-content">- <a href="http://cstvpodcast.cstv.com.edgesuite.net/texasam/081103_sherman.mp3">Oklahoma Media Luncheon - Head Coach Mike Sherman</a></div>

<div class="audio-date">11/3/2008</div>
<div class="audio-content">- <a href="http://cstvpodcast.cstv.com.edgesuite.net/texasam/081103_kines.mp3">Oklahoma Media Luncheon - Coach Joe Kines</a></div>

<div class="audio-date">11/3/2008</div>
<div class="audio-content">- <a href="http://cstvpodcast.cstv.com.edgesuite.net/texasam/081103_hunter.mp3">Oklahoma Media Luncheon - Trent Hunter</a></div>

<div class="audio-date">11/3/2008</div>
<div class="audio-content">- <a href="http://cstvpodcast.cstv.com.edgesuite.net/texasam/081103_johnson.mp3">Oklahoma Media Luncheon - Jerrod Johnson</a></div>

<div class="audio-date">11/3/2008</div>
<div class="audio-content">- <a href="http://cstvpodcast.cstv.com.edgesuite.net/texasam/081103_fuller.mp3">Oklahoma Media Luncheon - Jeff Fuller</a></div>

<div class="audio-date">11/1/2008</div>
<div class="audio-content">- <a href="http://cstvpodcast.cstv.com.edgesuite.net/texasam/081101_sherman.mp3">Colorado Post Game - Head Coach Mike Sherman</a></div>

<div class="audio-date">11/1/2008</div>
<div class="audio-content">- <a href="http://cstvpodcast.cstv.com.edgesuite.net/texasam/081101_kines.mp3">Colorado Post Game - Coach Joe Kines</a></div>

<div class="audio-date">11/1/2008</div>
<div class="audio-content">- <a href="http://cstvpodcast.cstv.com.edgesuite.net/texasam/081101_pugh_hunter.mp3">Colorado Post Game - DB Jordan Pugh &amp; DB Trent Hunter</a></div>

<div class="audio-date">11/1/2008</div>
<div class="audio-content">- <a href="http://cstvpodcast.cstv.com.edgesuite.net/texasam/081101_obiozor.mp3">Colorado Post Game - DL Cyril Obiozor</a></div>

<div class="audio-date">11/1/2008</div>
<div class="audio-content">- <a href="http://cstvpodcast.cstv.com.edgesuite.net/texasam/081101_johnson_gray_fuller.mp3">Colorado Post Game - QB Jerrod Johnson, RB Cyrus Gray, WR Jeff Fuller</a></div>

<div class="audio-date">10/27/2008</div>
<div class="audio-content">- <a href="http://cstvpodcast.cstv.com.edgesuite.net/texasam/081027_sherman.mp3">Colorado Media Luncheon - Head Coach Mike Sherman</a></div>

<div class="audio-date">10/27/2008</div>
<div class="audio-content">- <a href="http://cstvpodcast.cstv.com.edgesuite.net/texasam/081027_kines.mp3">Colorado Media Luncheon - Coach Joe Kines</a></div>

<div class="audio-date">10/27/2008</div>
<div class="audio-content">- <a href="http://cstvpodcast.cstv.com.edgesuite.net/texasam/081027_hunter.mp3">Colorado Media Luncheon - Trent Hunter</a></div>

<div class="audio-date">10/27/2008</div>
<div class="audio-content">- <a href="http://cstvpodcast.cstv.com.edgesuite.net/texasam/081027_johnson.mp3">Colorado Media Luncheon - Jerrod Johnson</a></div>

<div class="audio-date">10/27/2008</div>
<div class="audio-content">- <a href="http://cstvpodcast.cstv.com.edgesuite.net/texasam/081027_mccoy.mp3">Colorado Media Luncheon - Jamie and Terrence McCoy</a></div>

<div class="audio-date">10/25/2008</div>
<div class="audio-content">- <a href="http://cstvpodcast.cstv.com.edgesuite.net/texasam/081025-isupg-sherman.mp3">Iowa St. Post Game - Head Coach Mike Sherman</a></div>

<div class="audio-date">10/25/2008</div>
<div class="audio-content">- <a href="http://cstvpodcast.cstv.com.edgesuite.net/texasam/081025-isupg-johnson-obiozor.mp3">Iowa St. Post Game - Jerrod Johnson and Cyril Obiozor</a></div>

<div class="audio-date">10/25/2008</div>
<div class="audio-content">- <a href="http://cstvpodcast.cstv.com.edgesuite.net/texasam/081025-isupg-tannehill-mccoy.mp3">Iowa St. Post Game - Ryan Tannehill,Terrence McCoy and Jamie McCoy
</a></div>

<div class="audio-date">10/20/2008</div>
<div class="audio-content">- <a href="http://cstvpodcast.cstv.com.edgesuite.net/texasam/081020_sherman.mp3">Iowa St. Media Luncheon - Head Coach Mike Sherman</a></div>

<div class="audio-date">10/20/2008</div>
<div class="audio-content">- <a href="http://cstvpodcast.cstv.com.edgesuite.net/texasam/081020_hunter.mp3">Iowa St. Media Luncheon - Trent Hunter</a></div>

<div class="audio-date">10/20/2008</div>
<div class="audio-content">- <a href="http://cstvpodcast.cstv.com.edgesuite.net/texasam/081020_johnson.mp3">Iowa St. Media Luncheon - Jerrod Johnson</a></div>

<div class="audio-date">10/20/2008</div>
<div class="audio-content">- <a href="http://cstvpodcast.cstv.com.edgesuite.net/texasam/081020_tannehill.mp3">Iowa St. Media Luncheon - Ryan Tannehill</a></div>

<div class="audio-date">10/20/2008</div>
<div class="audio-content">- <a href="http://cstvpodcast.cstv.com.edgesuite.net/texasam/081020_grimes.mp3">Iowa St. Media Luncheon - Lee Grimes</a></div>

<div class="audio-date">10/18/2008</div>
<div class="audio-content">- <a href="http://cstvpodcast.cstv.com.edgesuite.net/texasam/081018_sherman.mp3">Texas Tech Post Game - Head Coach Mike Sherman</a></div>

<div class="audio-date">10/18/2008</div>
<div class="audio-content">- <a href="http://cstvpodcast.cstv.com.edgesuite.net/texasam/081018_kines.mp3">Texas Tech Post Game - Coach Joe Kines</a></div>

<div class="audio-date">10/18/2008</div>
<div class="audio-content">- <a href="http://cstvpodcast.cstv.com.edgesuite.net/texasam/081018_hunter_bennett.mp3">Texas Tech Post Game - Trent Hunter and Michael Bennett</a></div>

<div class="audio-date">10/18/2008</div>
<div class="audio-content">- <a href="http://cstvpodcast.cstv.com.edgesuite.net/texasam/081018_tannehill_johnson.mp3">Texas Tech Post Game - Ryan Tannehill and Jerrod Johnson</a></div>

<div class="audio-date">10/13/2008</div>
<div class="audio-content">- <a href="http://cstvpodcast.cstv.com.edgesuite.net/texasam/081013_sherman.mp3">Texas Tech Media Luncheon - Coach Sherman</a></div>

<div class="audio-date">10/13/2008</div>
<div class="audio-content">- <a href="http://cstvpodcast.cstv.com.edgesuite.net/texasam/081013_kines.mp3">Texas Tech Media Luncheon - Coach Kines</a></div>

<div class="audio-date">10/13/2008</div>
<div class="audio-content">- <a href="http://cstvpodcast.cstv.com.edgesuite.net/texasam/081013_hunter.mp3">Texas Tech Media Luncheon - Trent Hunter</a></div>

<div class="audio-date">10/13/2008</div>
<div class="audio-content">- <a href="http://cstvpodcast.cstv.com.edgesuite.net/texasam/081013_tannehill.mp3">Texas Tech Media Luncheon - Ryan Tannehill</a></div>

<div class="audio-date">10/13/2008</div>
<div class="audio-content">- <a href="http://cstvpodcast.cstv.com.edgesuite.net/texasam/081013_johnson.mp3">Texas Tech Media Luncheon - Jerrod Johnson</a></div>

<div class="audio-date">10/11/2008</div>
<div class="audio-content">- <a href="http://cstvpodcast.cstv.com.edgesuite.net/texasam/081011_sherman.mp3">KSU Post Game - Coach Sherman</a></div>

<div class="audio-date">10/11/2008</div>
<div class="audio-content">- <a href="http://cstvpodcast.cstv.com.edgesuite.net/texasam/081011_kines.mp3">KSU Post Game - Coach Kines</a></div>

<div class="audio-date">10/11/2008</div>
<div class="audio-content">- <a href="http://cstvpodcast.cstv.com.edgesuite.net/texasam/081011_3.mp3">KSU Post Game - QB Johnson, FB Lane, WR Fuller</a></div>

<div class="audio-date">10/11/2008</div>
<div class="audio-content">- <a href="http://cstvpodcast.cstv.com.edgesuite.net/texasam/081011_4.mp3">KSU Post Game - WR Tannehill, LB Miller</a></div>

<div class="audio-date">10/6/2008</div>
<div class="audio-content">- <a href="http://cstvpodcast.cstv.com.edgesuite.net/texasam/081006_sherman.mp3">KSU Media Luncheon - Coach Sherman</a></div>


<div class="audio-date">10/6/2008</div>
<div class="audio-content">- <a href="http://cstvpodcast.cstv.com.edgesuite.net/texasam/081006_schneider.mp3">KSU Media Luncheon - Travis Schneider</a></div>


<div class="audio-date">10/6/2008</div>
<div class="audio-content">- <a href="http://cstvpodcast.cstv.com.edgesuite.net/texasam/081006_kines.mp3">KSU Media Luncheon - Coach Kines</a></div>

<div class="audio-date">10/6/2008</div>
<div class="audio-content">- <a href="http://cstvpodcast.cstv.com.edgesuite.net/texasam/081006_fuller.mp3">KSU Media Luncheon - Jeff Fuller</a></div>

<div class="audio-date">10/6/2008</div>
<div class="audio-content">- <a href="http://cstvpodcast.cstv.com.edgesuite.net/texasam/081006_johnson.mp3">KSU Media Luncheon - Jerrod Johnson</a></div>

<div class="audio-date">10/4/2008</div>
<div class="audio-content">- <a href="http://cstvpodcast.cstv.com.edgesuite.net/texasam/081004-osupg-sherman.mp3">OSU Post Game - Coach Sherman</a></div>

<div class="audio-date">10/4/2008</div>
<div class="audio-content">- <a href="http://cstvpodcast.cstv.com.edgesuite.net/texasam/081004-osupg-goodson.mp3">OSU Post Game - Mike Goodson</a></div>

<div class="audio-date">10/4/2008</div>
<div class="audio-content">- <a href="http://cstvpodcast.cstv.com.edgesuite.net/texasam/081004-osupg-gorrer.mp3">OSU Post Game- Danny Gorrer</a></div>

<div class="audio-date">10/4/2008</div>
<div class="audio-content">- <a href="http://cstvpodcast.cstv.com.edgesuite.net/texasam/081004-osupg-johnson.mp3">OSU Post Game- Jerrod Johnson</a></div>

<div class="audio-date">10/4/2008</div>
<div class="audio-content">- <a href="http://cstvpodcast.cstv.com.edgesuite.net/texasam/081004-osupg-patterson.mp3">OSU Post Game- Lucas Patterson</a></div>

<div class="audio-date">9/29/2008</div>
<div class="audio-content">- <a href="http://cstvpodcast.cstv.com.edgesuite.net/texasam/080929_Sherman.mp3">OSU Media Luncheon - Coach Sherman</a></div>

<div class="audio-date">9/29/2008</div>
<div class="audio-content">- <a href="http://cstvpodcast.cstv.com.edgesuite.net/texasam/080929_Featherston.mp3">OSU Media Luncheon - Matt Featherston</a></div>

<div class="audio-date">9/29/2008</div>
<div class="audio-content">- <a href="http://cstvpodcast.cstv.com.edgesuite.net/texasam/080929_Kines.mp3">OSU Media Luncheon - Coach Kines</a></div>

<div class="audio-date">9/29/2008</div>
<div class="audio-content">- <a href="http://cstvpodcast.cstv.com.edgesuite.net/texasam/080929_Goodson.mp3">OSU  Media Luncheon - Mike Goodson</a></div>

<div class="audio-date">9/29/2008</div>
<div class="audio-content">- <a href="http://cstvpodcast.cstv.com.edgesuite.net/texasam/080929_Hunter.mp3">OSU Media Luncheon - Trent Hunter</a></div>

<div class="audio-date">9/29/2008</div>
<div class="audio-content">- <a href="http://cstvpodcast.cstv.com.edgesuite.net/texasam/080929_Johnson.mp3">OSU Media Luncheon - Jerrod Johnson</a></div>

<div class="audio-date">9/27/2008</div>
<div class="audio-content">- <a href="http://cstvpodcast.cstv.com.edgesuite.net/texasam/080927_Sherman.mp3">Army Post Game - Coach Sherman A&amp;M 21, Army 17</a></div>

<div class="audio-date">9/27/2008</div>
<div class="audio-content">- <a href="http://cstvpodcast.cstv.com.edgesuite.net/texasam/080927_Freeney.mp3">Army Post Game - Paul Freeney A&amp;M 21, Army 17</a></div>

<div class="audio-date">9/27/2008</div>
<div class="audio-content">- <a href="http://cstvpodcast.cstv.com.edgesuite.net/texasam/080927_Johnson_Lane.mp3">Army Post Game - Jerrod Johnson and Javorskie Lane</a><a href="http://cstvpodcast.cstv.com.edgesuite.net/texasam/92008_sherman.mp3"> A&amp;M 21, Army 17</a></div>

<div class="audio-date">9/27/2008</div>
<div class="audio-content">- <a href="http://cstvpodcast.cstv.com.edgesuite.net/texasam/080927_Hunter.mp3">Army Post Game - Trent Hunter A&amp;M 21, Army 17</a></div>

<div class="audio-date">9/27/2008</div>
<div class="audio-content">- <a href="http://cstvpodcast.cstv.com.edgesuite.net/texasam/080927_Fuller.mp3">Army Post Game - Jeff Fuller A&amp;M 21, Army 17</a></div>

<div class="audio-date">9/27/2008</div>
<div class="audio-content">- <a href="http://cstvpodcast.cstv.com.edgesuite.net/texasam/080927_McCoy.mp3">Army Post Game - Jamie McCoy A&amp;M 21, Army 17</a></div>

<div class="audio-date">9/22/2008</div>
<div class="audio-content">- <a href="http://cstvpodcast.cstv.com.edgesuite.net/texasam/080922_Sherman.mp3">Miami Media Luncheon - Coach Sherman <br />
</a></div>

<div class="audio-date">9/22/2008</div>
<div class="audio-content">- <a href="http://cstvpodcast.cstv.com.edgesuite.net/texasam/080922_Bennett.mp3">Miami Media Luncheon - Michael Bennett <br />
</a></div>

<div class="audio-date">9/22/2008</div>
<div class="audio-content">- <a href="http://cstvpodcast.cstv.com.edgesuite.net/texasam/080922_Fuller.mp3">Miami Media Luncheon - Jeff Fuller <br />
</a></div>

<div class="audio-date">9/22/2008</div>
<div class="audio-content">- <a href="http://cstvpodcast.cstv.com.edgesuite.net/texasam/080922_Johnson.mp3">Miami Media Luncheon - Jerrod Johnson <br />
</a></div>

<div class="audio-date">9/20/2008</div>
<div class="audio-content">- <a href="http://cstvpodcast.cstv.com.edgesuite.net/texasam/92008_sherman.mp3">Miami Post Game - Coach Sherman UofM 41, A&amp;M 23</a></div>

<div class="audio-date">9/20/2008</div>
<div class="audio-content">- <a href="http://cstvpodcast.cstv.com.edgesuite.net/texasam/92008_peterson_goodson.mp3">Miami Post Game - Goodson and Peterson</a><a href="http://cstvpodcast.cstv.com.edgesuite.net/texasam/92008_sherman.mp3"> UofM 41, A&amp;M 23</a></div>

<div class="audio-date">9/20/2008</div>
<div class="audio-content">- <a href="http://cstvpodcast.cstv.com.edgesuite.net/texasam/92008_johnson_fuller.mp3">Miami Post Game - Johnson and Fuller</a><a href="http://cstvpodcast.cstv.com.edgesuite.net/texasam/92008_sherman.mp3"> UofM 41, A&amp;M 23</a></div>

<div class="audio-date">9/20/2008</div>
<div class="audio-content">- <a href="http://cstvpodcast.cstv.com.edgesuite.net/texasam/92008_kines.mp3">Miami Post Game - Coach Kines</a><a href="http://cstvpodcast.cstv.com.edgesuite.net/texasam/92008_sherman.mp3"> UofM 41, A&amp;M 23</a>
</div>

<div class="audio-date">9/15/2008</div>
<div class="audio-content">- <a href="http://cstvpodcast.cstv.com.edgesuite.net/texasam/091508Jarrod_Johnson.mp3">Miami Media Luncheon - Jerrod Johnson Aggies Meet Media at Weekly Luncheon</a></div>

<div class="audio-date">9/15/2008</div>
<div class="audio-content">- <a href="http://cstvpodcast.cstv.com.edgesuite.net/texasam/091508Joe_Kines.mp3">Miami Media Luncheon - Joe Kines Aggies Meet Media at Weekly Luncheon</a></div>

<div class="audio-date">9/15/2008</div>
<div class="audio-content">- <a href="http://cstvpodcast.cstv.com.edgesuite.net/texasam/091508Jordan_Peterson.mp3">Miami Media Luncheon - Jordan Peterson Aggies Meet Media at Weekly Luncheon</a></div>

<div class="audio-date">9/15/2008</div>
<div class="audio-content">- <a href="http://cstvpodcast.cstv.com.edgesuite.net/texasam/091508Michael_Bennett.mp3">Miami Media Luncheon - Michael Bennett Aggies Meet Media at Weekly Luncheon</a></div>

<div class="audio-date">9/15/2008</div>
<div class="audio-content">- <a href="http://cstvpodcast.cstv.com.edgesuite.net/texasam/091508Stephen_McGee.mp3">Miami Media Luncheon - Stephen McGee Aggies Meet Media at Weekly Luncheon</a></div>

<div class="audio-date">9/01/2008</div>
<div class="audio-content">- <a href="http://cstvpodcast.cstv.com.edgesuite.net/texasam/090108_goodson.mp3">New Mexico Media Luncheon - Mike Goodson Aggies Meet Media at Weekly Luncheon</a></div>

<div class="audio-date">9/01/2008</div>
<div class="audio-content">- <a href="http://cstvpodcast.cstv.com.edgesuite.net/texasam/090108_tannehill.mp3">New Mexico Media Luncheon - Ryan Tannehill Aggies Meet Media at Weekly Luncheon</a></div>

<div class="audio-date">9/01/2008</div>
<div class="audio-content">- <a href="http://cstvpodcast.cstv.com.edgesuite.net/texasam/090108_featherston.mp3">New Mexico Media Luncheon - Matt Featherston Aggies Meet Media at Weekly Luncheon</a></div>

<div class="audio-date">9/01/2008</div>
<div class="audio-content">- <a href="http://cstvpodcast.cstv.com.edgesuite.net/texasam/090108_mcgee.mp3">New Mexico Media Luncheon - Stephen McGee Aggies Meet Media at Weekly Luncheon</a></div>

<div class="audio-date">9/01/2008</div>
<div class="audio-content">- <a href="http://cstvpodcast.cstv.com.edgesuite.net/texasam/090108_sherman.mp3">New Mexico Media Luncheon - Mike Sherman Aggies Meet Media at Weekly Luncheon</a></div>

<div class="audio-date">8/30/2008</div>
<div class="audio-content">- <a href="http://cstvpodcast.cstv.com.edgesuite.net/texasam/083008_kines.mp3">ASU Postgame -- Joe Kines ASU 18, Texas A&M 14</a></div>

<div class="audio-date">8/30/2008</div>
<div class="audio-content">- <a href="http://cstvpodcast.cstv.com.edgesuite.net/texasam/083008_sherman.mp3">ASU Postgame -- Mike Sherman ASU 18, Texas A&M 14</a></div>
	    </div>
	    <div id="photo-gallery" style="display:none">
			<ul id="galleryblock">
<li><a href="#" onClick="javascript:window.open('http://onlyfans.cstv.com/schools/tam/view.gal?id=42691', 'PopupViewer', 'toolbar=no,menubar=no,status=no,location=no,directories=no,titlebar=no,scrollbars=yes,resizable=yes,width=970,height=705');"><img src="http://grfx.cstv.com/photos/schools/tam/sports/m-baskbl/auto_galblocktwo/2758722.jpeg"></a>Aggies Thump Longhorns, 81-66</li><li><a href="#" onClick="javascript:window.open('http://onlyfans.cstv.com/schools/tam/view.gal?id=42684', 'PopupViewer', 'toolbar=no,menubar=no,status=no,location=no,directories=no,titlebar=no,scrollbars=yes,resizable=yes,width=970,height=705');"><img src="http://grfx.cstv.com/photos/schools/tam/sports/m-baskbl/auto_galblocktwo/2758371.jpeg"></a>Men's Basketball vs. Texas</li><li><a href="#" onClick="javascript:window.open('http://onlyfans.cstv.com/schools/tam/view.gal?id=42577', 'PopupViewer', 'toolbar=no,menubar=no,status=no,location=no,directories=no,titlebar=no,scrollbars=yes,resizable=yes,width=970,height=705');"><img src="http://grfx.cstv.com/photos/schools/tam/sports/w-baskbl/auto_galblocktwo/2752309.jpeg"></a>Women's Basketball vs Colorado</li><li><a href="#" onClick="javascript:window.open('http://onlyfans.cstv.com/schools/tam/view.gal?id=42159', 'PopupViewer', 'toolbar=no,menubar=no,status=no,location=no,directories=no,titlebar=no,scrollbars=yes,resizable=yes,width=970,height=705');"><img src="http://grfx.cstv.com/photos/schools/tam/sports/w-baskbl/auto_galblocktwo/2730641.jpeg"></a>Women's Basketball vs. Nebraska</li><li><a href="#" onClick="javascript:window.open('http://onlyfans.cstv.com/schools/tam/view.gal?id=42078', 'PopupViewer', 'toolbar=no,menubar=no,status=no,location=no,directories=no,titlebar=no,scrollbars=yes,resizable=yes,width=970,height=705');"><img src="http://grfx.cstv.com/photos/schools/tam/sports/m-baskbl/auto_galblocktwo/2726856.jpeg"></a>Men's Basketball vs. Kansas St.</li><li><a href="#" onClick="javascript:window.open('http://onlyfans.cstv.com/schools/tam/view.gal?id=42058', 'PopupViewer', 'toolbar=no,menubar=no,status=no,location=no,directories=no,titlebar=no,scrollbars=yes,resizable=yes,width=970,height=705');"><img src="http://grfx.cstv.com/photos/schools/tam/sports/w-tennis/auto_galblocktwo/2726007.jpeg"></a>A&M vs Auburn Tennis</li></ul>

	    </div>
	    <div id="in-the-news" style="display:none">
			
	    </div>
	</div><!-- #headlines-tab -->
</div>
					</div>
					<div class="wrap"><!-- wrapper start-->
						<div id="holder-story"></div>
					</div>
					<div class="clear"></div>
					
				</div>
				<div id="column-1-bottom">

					<div id="column-1-sport-headlines" class="wrap">
						<div id="column-1-hdr-sport-headlines">
							<div id="sport-headlines-ad">
								<a href="http://giving.tamu.edu/home.aspx" target="_new">
									<img src="http://grfx.cstv.com/schools/tam/graphics/spacer.gif" width="100px" height="32px" alt="Presented by Texas A&M Foundation">
								</a>
							</div>
						</div>
						<script>
						document.getElementById("column-1-hdr-sport-headlines").style.background =
							"url('http://grfx.cstv.com/schools/tam/graphics/tam-08-hdr-hdlns-m-footbl.gif') no-repeat;";
						</script>
						<div id="column-1-sport-headlines-content">
							<div id="sec-headlines-wrap">
			<p class="pubdate">01/27/2009</p>
			<h1><a href="/sports/m-footbl/spec-rel/012709aaa.html">Aggies set to compete in the Texas vs. The Nation All-Star Football Game</a></h1>
			<p class="synopsis">Texas A&M will be well represented at the third annual Texas vs. The Nation All-Star Football Game held at the Sun Bowl in El Paso, Texas, this Saturday, Jan. 31</h>
		</div>
	
		<div id="sec-headlines-wrap">
			<p class="pubdate">02/03/2009</p>
			<h1><a href="/sports/m-footbl/spec-rel/020309aab.html">Signing Day to have Live Interviews</a></h1>
			<p class="synopsis">Watch and listen to live Signing Day interviews with the Texas A&M Football Coaches beginning Wednesday morning at 10am beginning with Tight End & Special Teams Coordinator Kirk Doll and ending with Head Coach Mike Sherman's Press Conference beginning at 1pm. Full footballl, soccer and track and field signing day coverage with player bios and photos is available at http://signingday.aggieathletics.com</h>
		</div>
	
		<div id="sec-headlines-wrap">
			<p class="pubdate">01/12/2009</p>
			<h1><a href="/sports/m-footbl/spec-rel/011209aaa.html">McGee Wins FCA/Bobby Bowden Athlete of the Year Award</a></h1>
			<p class="synopsis">The Fellowship of Christian Athletes selected Stephen McGee as the recipient of the 2008 FCA Bobby Bowden Athlete of the Year. McGee received the sixth annual award from Coach Bowden on Wednesday in Nashville.</h>
		</div>
	
		<div id="sec-headlines-wrap">
			<p class="pubdate">01/09/2009</p>
			<h1><a href="/sports/m-footbl/spec-rel/010909aab.html">McGee Preps for Combine</a></h1>
			<p class="synopsis">Texas A&M's Stephen McGee visited with the media on Friday on the Texas A&M campus as he continues workouts and preparations for the 2009 NFL Combine</h>
		</div>
	
		<div id="sec-headlines-wrap">
			<p class="pubdate">12/22/2008</p>
			<h1><a href="/sports/m-footbl/spec-rel/122208aaa.html">McGee Named Finalist for Bobby Bowden Award</a></h1>
			<p class="synopsis">Texas A&M quarterback Stephen McGee has been selected as one of three finalists for the Bobby Bowden Award the Fellowship of Christian Athletes announced Monday.</h>
		</div>
	
		<div id="sec-headlines-wrap">
			<p class="pubdate">12/17/2008</p>
			<h1><a href="/sports/m-footbl/spec-rel/121708aaa.html">Five Aggies Named to Phil Steele's All-Freshman Team</a></h1>
			<p class="synopsis">Five members of the Texas A&M football team have been named to Phil Steele's All-Freshman team, the magazine's website announced Wednesday.</h>
		</div>
						</div>
					</div>
					
					<div class="wrap">
						<div id="column-2-hdr-events">
							<div class="wrap" style="width: 125px; height:32px;">
							</div>
							<div class="wrap" style="width: 100px; height: 32px">
								<a href="http://www.firstvictoria.com/" target="_new">
									<div class="image-block">
										<img src="http://grfx.cstv.com/schools/tam/graphics/spacer.gif" width="100px" height="32px" alt="Presented by First Victoria Bank">
									</div> 
								</a>
							</div>
							<div class="clear"></div>
						</div>
						<div id="column-2-events">
							<div class="calendar">

	
	<div class="calendar-date">There are no upcoming events.</div>
	

</div><!-- .calendar -->

	
						</div>
						<div class="column-2-btm-poll"></div>
					</div>
					<div class="clear"></div>
					
				</div>
            </td><!-- #column-1 -->
        </tr>
    </table> <!-- #columns -->

	<div id="side-ad">
		<div id="holder-banner"></div>
	</div>

		
</div><!-- #wrap -->

<div id="footer">
	
	
			<style>
			#logo-wrap{
				float:left;
				background: url("http://grfx.cstv.com/graphics/new-footer-06-white-logo.png") no-repeat;
						*background-image:none;
				filter:progid:DXImageTransform.Microsoft.AlphaImageLoader(src="http://grfx.cstv.com/graphics/new-footer-06-white-logo.png");
				width:50%;
				display:block;
				height:50px;
				display:block
			}
			#logo-wrap img{
				float:left;
				width:290px;
				height:50px;
			}
			#dots{
				clear:both;
				background: url("http://grfx.cstv.com/graphics/new-footer-06-white-dots.png, sizingMethod='scale'") no-repeat;
						*background-image:none;
				filter:progid:DXImageTransform.Microsoft.AlphaImageLoader(src="http://grfx.cstv.com/graphics/new-footer-06-white-dots.png, sizingMethod='scale'");
				height:6px;
				display:block;
						background-position: center;
			}
			</style>
			<link href="http://grfx.cstv.com/schools/tam/library/tam-07-footer.css" rel="stylesheet" type="text/css" />
			<div id="footer-wrap-temp">
			<div id="logo-wrap"><a href="http://www.sportsline.com/cbscollegesports/" target="_blank"><img src="http://grfx.cstv.com/graphics/spacer.gif" width="290" height="50" alt="CBS College Sports" border="0"/></a></div>
			
			<div id="dropdown-wrap">

				<form>
					<script language="javascript">
						document.write("<div><label for=\"cbsi_footer_menu\">Visit other CBS Interactive Sites</label><br><select name=\"cbsi_footer_menu\" id=\"cbsi_footer_menu\" class=\"rb_visit_sel\"> <option value=\"http://www.bnet.com\">BNET</option><option value=\"http://www.cbssports.com/cbscollegesports/\">CBS College Sports</option><option value=\"http://www.cbsradio.com/streaming/\">CBS Radio</option><option value=\"http://www.cbs.com\">CBS.com</option><option value=\"http://www.cbsnews.com\">CBSNews.com</option><option value=\"http://www.cbssports.com\">CBSSports.com</option><option value=\"http://www.chow.com\">CHOW</option><option value=\"http://www.cnet.com\">CNET</option><option value=\"http://www.cnetcontentsolutions.com\">CNET Content Solutions</option><option value=\"http://www.findarticles.com\">Find Articles</option><option value=\"http://www.gamespot.com\">GameSpot</option><option value=\"http://www.cnetnetworks.com/advertise/media-kit/international.html?tag=ft\">International</option><option value=\"http://www.last.fm\">Last.fm</option><option value=\"http://www.metacritic.com\">Metacritic.com</option><option value=\"http://www.mp3.com\">MP3.com</option><option value=\"http://www.mysimon.com\">mySimon</option><option value=\"http://www.ncaa.com\">NCAA</option><option value=\"http://www.search.com\">Search.com</option><option value=\"http://www.shopper.com\">Shopper.com</option><option value=\"http://www.techrepublic.com\">TechRepublic</option><option value=\"http://www.theinsider.com\">The Insider</option><option value=\"http://www.tv.com\">TV.com</option><option value=\"http://www.urbanbaby.com\">UrbanBaby.com</option><option value=\"http://www.uwire.com\">UWIRE</option><option value=\"http://www.zdnet.com\">ZDNet</option></select> <input type=\"button\" value=\"Go\" onClick=\"window.location=document.getElementById('cbsi_footer_menu').options[document.getElementById('cbsi_footer_menu').selectedIndex].value;\" class=\"rb_visit_go\" /></div>");
					</script>
				</form>
				<noscript>
					<div> <a href="http://www.bnet.com">BNET</a>  |  
<a href="http://www.cbssports.com/cbscollegesports/">CBS College Sports</a>  |  
<a href="http://www.cbsradio.com/streaming/">CBS Radio</a>  |  
<a href="http://www.cbs.com">CBS.com</a>  |  
<a href="http://www.cbsnews.com">CBSNews.com</a>  |  
<a href="http://www.cbssports.com">CBSSports.com</a>  |  
<a href="http://www.chow.com">CHOW</a>  |  
<a href="http://www.cnet.com">CNET</a>  |  
<a href="http://www.cnetcontentsolutions.com">CNET Content Solutions</a>  |  
<a href="http://www.findarticles.com">Find Articles</a>  |  
<a href="http://www.gamespot.com">GameSpot</a>  |  
<a href="http://www.cnetnetworks.com/advertise/media-kit/international.html?tag=ft">International</a>  |  
<a href="http://www.last.fm">Last.fm</a>  |  
<a href="http://www.metacritic.com">Metacritic.com</a>  |  
<a href="http://www.mp3.com">MP3.com</a>  |  
<a href="http://www.mysimon.com">mySimon</a>  |  
<a href="http://www.ncaa.com">NCAA</a>  |  
<a href="http://www.search.com">Search.com</a>  |  
<a href="http://www.shopper.com">Shopper.com</a>  |  
<a href="http://www.techrepublic.com">TechRepublic</a>  |  
<a href="http://www.theinsider.com">The Insider</a>  |  
<a href="http://www.tv.com">TV.com</a>  |  
<a href="http://www.urbanbaby.com">UrbanBaby.com</a>  |  
<a href="http://www.uwire.com">UWIRE</a>  |  
<a href="http://www.zdnet.com">ZDNet</a>  |  
 </div>
				</noscript>
			</div>
		
			<div id="dots"></div>
			<div id="footer-links">&copy; 2009 CBS Interactive. All rights reserved. | <a href='http://www.cstv.com/ot/privacy.html'>Privacy Policy</a> | <a href='http://www.cstv.com/ot/cs-tos.html'>Terms of Use</a> | <a href='http://www.sportsline.com/cbscollegesports/'>About Us</a> | <a href='http://www.cbs.com/sales/'>Advertise</a> | <a href=http://www.aggieathletics.com/feedback/tam-feedback.html class="csdisclaimerlink">Feedback</a> |
			<a href="/rss/rss-index.html"><span class="rss-box">XML</span></a> <a href="/rss/rss-index.html">RSS Feeds</a>

			</div>
			</div>
			


</div><!-- #footer -->

<div id="holder-banner"></div>

<div id="banner"><script language="javascript1.2"> procad("http://ad.doubleclick.net/adj/CSTV.TAM/SPORTS.MFOOTBL.BODY;sect=mfootbl;pos=skyscraper;sz=160x600;dcopt=ist;",0); </script></div>
<div id="story"><script language="javascript1.2"> procad("http://ad.doubleclick.net/adj/CSTV.TAM/SPORTS.MFOOTBL.BODY;sect=mfootbl;pos=story;sz=300x250;dcopt=ist;",0); </script></div>

<script type="text/javascript" src="http://grfx.cstv.com/schools/tam/library/scripts/tam-08-url.js"></script>
<script type="text/javascript">
	document.getElementById("holder-banner").appendChild(document.getElementById("banner")); 
	document.getElementById("holder-story").appendChild(document.getElementById("story"));
</script>

<script language="javascript1.2"> procad("http://ad.doubleclick.net/adj/CSTV.TAM/SPORTS.MFOOTBL.BODY;sect=mfootbl;pos=popup;sz=1x1;dcopt=ist;",0); </script>
<script language="javascript1.2"> procad("http://ad.doubleclick.net/adj/CSTV.TAM/SPORTS.MFOOTBL.BODY;sect=mfootbl;pos=popunder;sz=1x1;dcopt=ist;",0); </script>


<!-- CNET tag for reporting OAS traffic -->
<script type="text/javascript" src="http://dw.com.com/js/dw.js"></script>
<script type="text/javascript">
  DW.pageParams = {
    siteId: '225'
  };
  DW.clear();
</script>
<noscript>
<img src="http://dw.com.com/clear/c.gif?ts=1234949537&amp;sid=225&amp;im=img" border="0" height="1" width="1" alt="" />
</noscript>

<style type="text/css">
#inseguitore{position:absolute;left:0;top:0;}
</style>
<div id="inseguitore">
<!-- START Nielsen//NetRatings SiteCensus V5.3 -->
<!-- COPYRIGHT 2007 Nielsen//NetRatings -->
<script type="text/javascript">
var _rsCI="us-cstv";
var _rsCG="school-official";
var _rsDN="//secure-us.imrworldwide.com/";
var _rsCL=1;
</script>
<script type="text/javascript" src="//secure-us.imrworldwide.com/v53.js"></script>
<noscript>
<div><img src="//secure-us.imrworldwide.com/cgi-bin/m?ci=us-cstv&amp;cg=school-official&cc=1" alt=""/></div>
</noscript>
<!-- END Nielsen//NetRatings SiteCensus V5.3 --> 
</div>

</body>
</html>

