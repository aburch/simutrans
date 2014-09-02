#!/usr/bin/perl
package align_fences;
no warnings "experimental::lexical_subs";
use feature qw(lexical_subs say);
use Imager;

our sub aref2pngspec {
    my ($l) = @_;

    return $l->[0] . "," . $l->[1] . "," . $l->[2];
}

our sub parse_pngspec {
    my ($self, $dpath) = @_;

    $dpath =~ s/> *//;

    if ($dpath =~ /^(.*)\.(\d+)\.(\d+)(?:,(-?\d+))?(?:,(-?\d+))?$/) {
	my ($basepath, $yn, $xn, $xoff, $yoff) = ($1, $2, $3, $4, $5);

	return ($basepath, $yn, $xn, $xoff, $yoff);
    }
}

our sub transparentify {
    my ($self, $img) = @_;

    #return Imager::transform2({expr => "pix=getp1(x,y); return ifp((red(pix)==231) && (green(pix)==255) && (blue(pix)==255),rgba(0,0,0,0),rgba(red(pix),green(pix),blue(pix),255))", channels=>4}, $img);

    my $otrans = Imager::Color->new(231, 255, 255, 255);
    my $ntrans = Imager::Color->new(0, 0, 0, 0);
    for my $x (0..$img->getwidth-1) {
	for my $y (0..$img->getheight-1) {
	    my ($r,$g,$b) = $img->getpixel(x=>$x, y=>$y)->rgba();

	    if ($r == 231 and $g == 255 and $b == 255) {
		$img->setpixel(x=>$x, y=>$y, color=>$ntrans);
	    }
	}
    }

    return $img;
}

our sub detransparentify {
    my ($self, $img) = @_;

    #return Imager::transform2({expr => "pix=getp1(x,y); return if(alpha(pix)==0,rgba(231,255,255,255),pix)", channels=>4}, $img);

    my $otrans = Imager::Color->new(231, 255, 255, 255);
    my $ntrans = Imager::Color->new(0, 0, 0, 0);
    for my $x (0..$img->getwidth-1) {
	for my $y (0..$img->getheight-1) {
	    my ($r,$g,$b,$a) = $img->getpixel(x=>$x, y=>$y)->rgba();

	    if ($a == 0) {
		$img->setpixel(x=>$x, y=>$y, color=>$otrans);
	    }
	}
    }

    return $img;
}

our sub pngspec2image {
    my ($self, $pngspec) = @_;

    my ($basepath, $yn, $xn, $xoff, $yoff) = $self->parse_pngspec($pngspec);

    my $img = Imager->new(file=>$basepath.".png");
    die "$pngspec -> $basepath" unless $img;
    $img = $img->convert(preset=>"addalpha");

    $img = $img->crop(left=>$xn * $self->{resolution}, top=>$yn * $self->{resolution},
		      width=>$self->{resolution}, height=>$self->{resolution});

    warn "$xn $yn $xoff $yoff";

    $img = $self->transparentify($img);

    return ($img,$xoff,$yoff);
}

sub assemble {
    my ($self, $aref, $minx, $maxx) = @_;

    my ($inimg, $xoff, $yoff) = $self->pngspec2image(aref2pngspec($aref));

    my $src_minx = $minx - $xoff;
    my $src_maxx = $maxx - $xoff;
    #$src_minx = 0;
    #$src_maxx = 128;
    if (0 && $xoff == 32) {
	$self->{outimg}->compose(src=>$inimg, tx=>$xoff+32, ty=>$yoff, src_minx=>32);
    } else {
	$self->{outimg}->compose(src=>$inimg, tx=>$xoff+$src_minx, ty=>$yoff, src_minx=>$src_minx, src_maxx=>$src_maxx, src_miny=>0, src_maxy=>128);
    }
}

sub assemble2 {
    my ($self, $pngspec) = @_;

    my ($inimg, $xoff, $yoff) = $self->pngspec2image($pngspec);
    $self->{outimg}->compose(src=>$inimg, tx=>$xoff, ty=>$yoff);
}

sub get_img {
    my ($self) = @_;
    $self->{outimg} = $self->detransparentify($self->{outimg});
    return $self->{outimg};
}

sub new {
    my ($class, $resolution) = @_;

    return bless { resolution => $resolution, outimg => Imager->new(xsize=>$resolution, ysize=>$resolution,channels=>4), }, $class;
}

package main;

my $resolution = 128;

sub assemble {
    my ($l, $front, $outimg, $yoff) = @_;
    my @l = @$l;

    for my $i (1..3) {
	my $x = ($i&3);
	my $y = $yoff;
	my $af = align_fences->new($resolution);

	for my $spec (@{$l->[0]}) {
	    $af->assemble($spec, 0, 64) if ($i&1); # ($i & 8);
	}
	for my $spec (@{$l->[1]}) {
	    $af->assemble($spec,64,128) if ($i&2); # ($i & 1);
	}

	$outimg->compose(src=>$af->get_img(), tx=>$x*$resolution, ty=>$y*$resolution);
    }

    for my $i (1..3) {
	my $x = ($i&3);
	my $y = $yoff + 1;
	my $af = align_fences->new($resolution);

	for my $spec (@{$l->[3]}) {
	    $af->assemble($spec,  0, 64) if ($i&2); # ($i & 4);
	}
	for my $spec (@{$l->[2]}) {
	    $af->assemble($spec, 64,128) if ($i&1); # ($i & 2);
	}

	$outimg->compose(src=>$af->get_img(), tx=>$x*$resolution, ty=>$y*$resolution);
    }
}

sub assemble_slope {
    my ($l, $outimg, $yoff, $xoff, $lr) = @_;

    my $y = $yoff;
    my $af = align_fences->new($resolution);

    for my $spec (@$l) {
	if ($lr == 2) {
	    $af->assemble($spec, 0, 128);
	} elsif ($lr) {
	    $af->assemble($spec, 0, 64);
	} else {
	    $af->assemble($spec, 64, 128);
	}
    }

    $outimg->compose(src=>$af->get_img(), tx=>$xoff*$resolution, ty=>$yoff*$resolution);
}

sub lseries2 {
    my ($b0, $b1, $y0, $y1, $ystep) = @_;
    my @l;

    if (defined($ystep)) {
	push @l, [$b0, -96, $y0 - 3 * $ystep/2];
	push @l, [$b0,  32, $y0 + 1 * $ystep/2];
	push @l, [$b1, -32, $y1 - 1 * $ystep/2];
	push @l, [$b1,  96, $y1 + 3 * $ystep/2];
    } else {
	$ystep = 32;
	push @l, [$b1,  32, $y1 - 1 * $ystep/2];
	push @l, [$b0, -32, $y0 + 1 * $ystep/2];
    }

    if ($ystep < 0) {
	@l = reverse @l;
    }

    return \@l;
}

sub lseries {
    my ($b, $y0, $ystep) = @_;
    my @l;

    push @l, [$b, -96, $y0 - 3 * $ystep/2];
    push @l, [$b, -32, $y0 - 1 * $ystep/2];
    push @l, [$b,  32, $y0 + 1 * $ystep/2];
    push @l, [$b,  96, $y0 + 3 * $ystep/2];

    if ($ystep < 0) {
	@l = reverse @l;
    }

    return \@l;
}

for my $b ("palissade", "plexi", "plexinoground3") {
    my $outimg = Imager->new(xsize=>$resolution*4, ysize=>$resolution*7,channels=>3);

    my $l = [
	lseries("$b.0.1",-32,-32),
	lseries("$b.0.0", 0,-32),

	lseries("$b.0.1", 0,32),
	lseries("$b.0.0",32,32),
	];

    assemble($l, 0, $outimg, 0);

    assemble_slope(lseries("$b.0.2",-32,-48), $outimg, 2, 0, 1);
    assemble_slope(lseries("$b.0.2", 48,-48), $outimg, 2, 1, 0);
    assemble_slope(lseries("$b.0.3",-32, 48), $outimg, 2, 2, 0);
    assemble_slope(lseries("$b.0.3", 48, 48), $outimg, 2, 3, 1);

    assemble_slope(lseries("$b.0.4",-16, 16), $outimg, 3, 2, 0);
    assemble_slope(lseries("$b.0.4", 32, 16), $outimg, 3, 3, 1);
    assemble_slope(lseries("$b.0.5",-16,-16), $outimg, 3, 0, 1);
    assemble_slope(lseries("$b.0.5", 32,-16), $outimg, 3, 1, 0);

    assemble_slope(lseries("$b.3.2",-16,  0), $outimg, 4, 0, 1);
    assemble_slope(lseries("$b.3.2", 16,  0), $outimg, 4, 1, 0);
    assemble_slope(lseries("$b.3.3",-16,  0), $outimg, 4, 2, 0);
    assemble_slope(lseries("$b.3.3", 16,  0), $outimg, 4, 3, 1);

    assemble_slope(lseries("$b.3.4",-48,-64), $outimg, 5, 0, 1);
    assemble_slope(lseries("$b.3.4", 48,-64), $outimg, 5, 1, 0);
    assemble_slope(lseries("$b.3.5",-48, 64), $outimg, 5, 2, 0);
    assemble_slope(lseries("$b.3.5", 48, 64), $outimg, 5, 3, 1);

    # assemble_slope(lseries("$b.1.4",-48,  0), $outimg, 6, 0, 1);
    # assemble_slope(lseries("$b.1.4",-48,  0), $outimg, 6, 1, 0);
    # assemble_slope(lseries("$b.1.5",-48,  0), $outimg, 6, 2, 0);
    # assemble_slope(lseries("$b.1.5",-48,  0), $outimg, 6, 3, 1);

    assemble_slope(lseries2("$b.1.4", "$b.1.5",  -16, 16, 0), $outimg, 6, 0, 2);
    assemble_slope(lseries2("$b.1.4", "$b.1.5",  -16, 16, 0), $outimg, 6, 1, 2);
    assemble_slope(lseries2("$b.2.4", "$b.2.5",    0,  0, undef), $outimg, 6, 2, 2);
    assemble_slope(lseries2("$b.2.4", "$b.2.5",    0,  0, undef), $outimg, 6, 3, 2);

    $outimg->write(file=>"out-$b.png");
    dispatch($b, "$b.4.1", "$b.4.0");
}

sub dispatch {
    my ($b, $cursor, $icon) = @_;
    $cursor //= "blockwall.5.1";
    $icon //= "blockwall.5.0";
print <<EOF;
Obj=way-object
Name=NoiseProtectionWall$b
copyright=James/Timothy Baldock
cost=90000
maintenance=400
topspeed=115
intro_year=1931
intro_month=5
waytype=road
own_waytype=noise_barrier
EOF
    print "cursor=$cursor\n";
    print "icon=> $icon\n";

    print "BackImage[N]=out-$b.0.1\n";
    print "BackImage[S]=out-$b.0.1\n";
    print "BackImage[NS]=out-$b.0.1\n";
    print "BackImage[NE]=out-$b.0.1\n";
    print "BackImage[NSE]=out-$b.0.1\n";

    print "BackImage[W]=out-$b.0.2\n";
    print "BackImage[E]=out-$b.0.2\n";
    print "BackImage[EW]=out-$b.0.2\n";
    print "BackImage[SW]=out-$b.0.2\n";
    print "BackImage[SEW]=out-$b.0.2\n";

    print "FrontImage[N]=out-$b.1.1\n";
    print "FrontImage[S]=out-$b.1.1\n";
    print "FrontImage[NS]=out-$b.1.1\n";
    print "FrontImage[SW]=out-$b.1.1\n";
    print "FrontImage[NSW]=out-$b.1.1\n";

    print "FrontImage[E]=out-$b.1.2\n";
    print "FrontImage[W]=out-$b.1.2\n";
    print "FrontImage[EW]=out-$b.1.2\n";
    print "FrontImage[NE]=out-$b.1.2\n";
    print "FrontImage[NEW]=out-$b.1.2\n";

    print "BackImage[SE]=out-$b.0.3\n";
    print "FrontImage[NW]=out-$b.1.3\n";

    print "BackImageUp2[3]=out-$b.4.0\n";
    print "FrontImageUp2[3]=out-$b.4.1\n";
    print "BackImageUp[3]=out-$b.3.0\n";
    print "FrontImageUp[3]=out-$b.3.1\n";

    print "BackImageUp2[6]=out-$b.4.2\n";
    print "FrontImageUp2[6]=out-$b.4.3\n";
    print "BackImageUp[6]=out-$b.3.2\n";
    print "FrontImageUp[6]=out-$b.3.3\n";

    print "BackImageUp2[9]=out-$b.5.2\n";
    print "FrontImageUp2[9]=out-$b.5.3\n";
    print "BackImageUp[9]=out-$b.2.2\n";
    print "FrontImageUp[9]=out-$b.2.3\n";

    print "BackImageUp2[12]=out-$b.5.0\n";
    print "FrontImageUp2[12]=out-$b.5.1\n";
    print "BackImageUp[12]=out-$b.2.0\n";
    print "FrontImageUp[12]=out-$b.2.1\n";

    print "FrontDiagonal[NE]=out-$b.6.2\n";
    print "FrontDiagonal[SW]=out-$b.6.2\n";
    print "FrontDiagonal[SE]=out-$b.6.0\n";
    print "FrontDiagonal[NW]=out-$b.6.0\n";

    print "BackDiagonal[NE]=out-$b.6.2\n";
    print "BackDiagonal[SW]=out-$b.6.2\n";
    print "BackDiagonal[SE]=out-$b.6.0\n";
    print "BackDiagonal[NW]=out-$b.6.0\n";

    print "-------\n";
}
