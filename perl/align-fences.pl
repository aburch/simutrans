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

    for my $i (0..3) {
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

    for my $i (0..3) {
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

for my $b ("plexi", "plexinoground") {
    my $outimg = Imager->new(xsize=>$resolution*4, ysize=>$resolution*18,channels=>3);

    my $l = [
	[["$b.0.1", 96,-80],["$b.0.1", 32,-48],["$b.0.1",-32,-16],["$b.0.1",-96, 16],],
	[["$b.0.0",-96,-80],["$b.0.0",-32,-48],["$b.0.0", 32,-16],["$b.0.0", 96, 16],],

	[["$b.0.1", 96,-16],["$b.0.1", 32, 16],["$b.0.1",-32, 48],["$b.0.1",-96, 80],],
	[["$b.0.0",-96,-16],["$b.0.0",-32, 16],["$b.0.0", 32, 48],["$b.0.0", 96, 80],],
	];

    assemble($l, 0, $outimg, 0);

    my $colimg = Imager->new(xsize=>$resolution*4, ysize=>$resolution*2, channels=>3);

    assemble($l, 0, $colimg, 0);

    for my $xn (0..3) {
	for my $yn (0..1) {
	    my $partimg = $colimg->crop(left=>$xn * 128, top=>$yn * 128,
					width=>128, height=>128);

#	    $partimg = Imager::transform2({ expr => "x0=64; y0=64; return if(x > 64,getp1(x,y+int((x-64)/2)),getp1(x,y+int((64-x)/2)))"}, $partimg);
	    $partimg = Imager::transform2({ expr => "x0=64; y0=64; return if(x > 64,getp1(x,y+int((x-x0)/2)),getp1(x,y))"}, $partimg);
#	    $partimg = Imager::transform2({ expr => "x0=64; y0=64; return if(x > 64,getp1(x,y+int((x-64)/2)),getp1(x,y+int((64-x)/2)))"}, $partimg);

	    $outimg->compose(src=>$partimg, tx=>$xn*128, ty=>($yn+2)*128);
	}
    }

    $l = [
	[["$b.3.4",-32,-16],],
	[["$b.0.0",32,-48],],
	[["$b.3.4",32,16],],
	[["$b.0.0",-32,16],],
	];

    #assemble($l, 0, $outimg, 2);

    $l = [
	[["$b.0.1",-32,-48],],
	[["$b.3.5",32,-16],],
	[["$b.0.1",32,16],],
	[["$b.3.5",-32,16],],
	];

    assemble($l, 0, $outimg, 4);
    $l = [
	[["$b.2.2",-32,-8],],
	[["$b.0.0",32,-32],],
	[["$b.2.2",32,24],],
	[["$b.0.0",-32,16],],
	];

    assemble($l, 0, $outimg, 6);

    $l = [
	[["$b.0.1",-32,-32],],
	[["$b.2.3",32,-8],],
	[["$b.0.1",32,16],],
	[["$b.2.3",-32,24],],
	];

    assemble($l, 0, $outimg, 8);

    $l = [
	[["$b.0.1",-32,-16],],
	[["$b.3.2",32,-16],],
	[["$b.0.1",32,-16],],
	[["$b.3.2",-32,16],],
	];

    assemble($l, 0, $outimg, 10);

    $l = [
	[["$b.3.3",-32,-16],],
	[["$b.0.0",32,-16],],
	[["$b.3.3",32,16],],
	[["$b.0.0",-32,-16],],
	];

    assemble($l, 0, $outimg, 12);

    $l = [
	[["$b.0.1",-32,-16],],
	[["$b.0.4",32,-24],],
	[["$b.0.1",32,0],],
	[["$b.0.4",-32,8],],
	];

    assemble($l, 0, $outimg, 14);

    $l = [
	[["$b.0.5",-32,-24],],
	[["$b.0.0",32,-16],],
	[["$b.0.5",32,8],],
	[["$b.0.0",-32,0],],
	];

    assemble($l, 0, $outimg, 16);

    $outimg->write(file=>"out-$b.png");
    dispatch($b, "$b.4.1", "$b.4.0");
}

for my $b ("palissade", "berlinwallfixed") {
    my $outimg = Imager->new(xsize=>$resolution*4, ysize=>$resolution*18,channels=>3);

    my $l = [
	[["$b.0.1", 96,-80],["$b.0.1", 32,-48],["$b.0.1",-32,-16],["$b.0.1",-96, 16],],
	[["$b.0.0",-96,-80],["$b.0.0",-32,-48],["$b.0.0", 32,-16],["$b.0.0", 96, 16],],

	[["$b.0.1", 96,-16],["$b.0.1", 32, 16],["$b.0.1",-32, 48],["$b.0.1",-96, 80],],
	[["$b.0.0",-96,-16],["$b.0.0",-32, 16],["$b.0.0", 32, 48],["$b.0.0", 96, 80],],
	];

    assemble($l, 0, $outimg, 0);

    $l = [
	[["$b.3.4",-32,-16],],
	[["$b.0.0",32,-48],],
	[["$b.3.4",32,16],],
	[["$b.0.0",-32,16],],
	];

    assemble($l, 0, $outimg, 2);

    $l = [
	[["$b.0.1",-32,-48],],
	[["$b.3.5",32,-16],],
	[["$b.0.1",32,16],],
	[["$b.3.5",-32,16],],
	];

    assemble($l, 0, $outimg, 4);
    $l = [
	[["$b.0.2",-32,-8],],
	[["$b.0.0",32,-32],],
	[["$b.0.2",32,24],],
	[["$b.0.0",-32,16],],
	];

    assemble($l, 0, $outimg, 6);

    $l = [
	[["$b.0.1",-32,-32],],
	[["$b.0.3",32,-8],],
	[["$b.0.1",32,16],],
	[["$b.0.3",-32,24],],
	];

    assemble($l, 0, $outimg, 8);

    $l = [
	[["$b.0.1",-32,-16],],
	[["$b.3.2",32,-16],],
	[["$b.0.1",32,-16],],
	[["$b.3.2",-32,16],],
	];

    assemble($l, 0, $outimg, 10);

    $l = [
	[["$b.3.3",-32,-16],],
	[["$b.0.0",32,-16],],
	[["$b.3.3",32,16],],
	[["$b.0.0",-32,-16],],
	];

    assemble($l, 0, $outimg, 12);

    $l = [
	[["$b.0.1",-32,-16],],
	[["$b.0.4",32,-8],],
	[["$b.0.1",32,0],],
	[["$b.0.4",-32,24],],
	];

    assemble($l, 0, $outimg, 14);

    $l = [
	[["$b.0.5",-32,-8],],
	[["$b.0.0",32,-16],],
	[["$b.0.5",32,24],],
	[["$b.0.0",-32,0],],
	];

    assemble($l, 0, $outimg, 16);

    $outimg->write(file=>"out-$b.png");
    dispatch($b, undef, "$b.4.0");
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

for my $b ("blockwall") {
    my $outimg = Imager->new(xsize=>$resolution*4, ysize=>$resolution*18,channels=>3);

    my $l = [
	lseries("$b.0.0", -32,-32),
	lseries("$b.0.1", -32, 32),
	lseries("$b.0.0",  32,-32),
	lseries("$b.0.1",  32, 32),];

    assemble($l, 0, $outimg, 0);

    $l = [
	[["$b.3.0",-32,-16],],
	[["$b.0.1",32,-48],],
	[["$b.3.0",32,16],],
	[["$b.0.1",-32,16],],
	];

    assemble($l, 0, $outimg, 2);

    $l = [
	[["$b.0.0",-32,-48],],
	[["$b.3.1",32,-16],],
	[["$b.0.0",32,16],],
	[["$b.3.1",-32,16],],
	];

    assemble($l, 0, $outimg, 4);
    $l = [
	[["$b.3.2",-32,-8],],
	[["$b.0.1",32,-32],],
	[["$b.3.2",32,24],],
	[["$b.0.1",-32,16],],
	];

    assemble($l, 0, $outimg, 6);

    $l = [
	[["$b.0.0",-32,-32],],
	[["$b.3.3",32,-8],],
	[["$b.0.0",32,16],],
	[["$b.3.3",-32,24],],
	];

    assemble($l, 0, $outimg, 8);

    $l = [
	[["$b.0.0",-32,-16],],
	[["$b.4.1",32,-16],],
	[["$b.0.0",32,-16],],
	[["$b.4.1",-32,16],],
	];

    assemble($l, 0, $outimg, 10);

    $l = [
	[["$b.4.1",-32,-16],],
	[["$b.0.1",32,-16],],
	[["$b.4.1",32,16],],
	[["$b.0.1",-32,-16],],
	];

    assemble($l, 0, $outimg, 12);

    $l = [
	[["$b.0.0",-32,-16],],
	[["$b.5.2",32,-8],],
	[["$b.0.0",32,0],],
	[["$b.5.2",-32,24],],
	];

    assemble($l, 0, $outimg, 14);

    $l = [
	[["$b.5.3",-32,-8],],
	[["$b.0.1",32,-16],],
	[["$b.5.3",32,24],],
	[["$b.0.1",-32,0],],
	];

    assemble($l, 0, $outimg, 16);

    $outimg->write(file=>"out-$b.png");
    dispatch($b);
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
    print "is_fence=1\n";

    print "BackImage[0]=out-$b.0.1\n";
    print "BackImage[1]=out-$b.0.2\n";
    print "BackImage[2]=out-$b.0.3\n";

    print "BackImage[3]=out-$b.16.1\n";
    print "BackImage[4]=out-$b.16.2\n";
    print "BackImage[5]=out-$b.16.3\n";

    print "BackImage[6]=out-$b.8.1\n";
    print "BackImage[7]=out-$b.8.2\n";
    print "BackImage[8]=out-$b.8.3\n";

    print "BackImage[9]=out-$b.6.1\n";
    print "BackImage[10]=out-$b.6.2\n";
    print "BackImage[11]=out-$b.6.3\n";

    print "BackImage[12]=out-$b.14.1\n";
    print "BackImage[13]=out-$b.14.2\n";
    print "BackImage[14]=out-$b.14.3\n";

    print "BackImage[15]=out-$b.12.1\n";
    print "BackImage[16]=out-$b.12.2\n";
    print "BackImage[17]=out-$b.12.3\n";

    print "BackImage[18]=out-$b.4.1\n";
    print "BackImage[19]=out-$b.4.2\n";
    print "BackImage[20]=out-$b.4.3\n";

    print "BackImage[21]=out-$b.2.1\n";
    print "BackImage[22]=out-$b.2.2\n";
    print "BackImage[23]=out-$b.2.3\n";

    print "BackImage[24]=out-$b.10.1\n";
    print "BackImage[25]=out-$b.10.2\n";
    print "BackImage[26]=out-$b.10.3\n";

    print "FrontImage[0]=out-$b.1.1\n";
    print "FrontImage[1]=out-$b.1.2\n";
    print "FrontImage[2]=out-$b.1.3\n";

    print "FrontImage[3]=out-$b.17.1\n";
    print "FrontImage[4]=out-$b.17.2\n";
    print "FrontImage[5]=out-$b.17.3\n";

    print "FrontImage[6]=out-$b.9.1\n";
    print "FrontImage[7]=out-$b.9.2\n";
    print "FrontImage[8]=out-$b.9.3\n";

    print "FrontImage[9]=out-$b.7.1\n";
    print "FrontImage[10]=out-$b.7.2\n";
    print "FrontImage[11]=out-$b.7.3\n";

    print "FrontImage[12]=out-$b.15.1\n";
    print "FrontImage[13]=out-$b.15.2\n";
    print "FrontImage[14]=out-$b.15.3\n";

    print "FrontImage[15]=out-$b.13.1\n";
    print "FrontImage[16]=out-$b.13.2\n";
    print "FrontImage[17]=out-$b.13.3\n";

    print "FrontImage[18]=out-$b.5.1\n";
    print "FrontImage[19]=out-$b.5.2\n";
    print "FrontImage[20]=out-$b.5.3\n";

    print "FrontImage[21]=out-$b.3.1\n";
    print "FrontImage[22]=out-$b.3.2\n";
    print "FrontImage[23]=out-$b.3.3\n";

    print "FrontImage[24]=out-$b.11.1\n";
    print "FrontImage[25]=out-$b.11.2\n";
    print "FrontImage[26]=out-$b.11.3\n";

    print "-------\n";
}
