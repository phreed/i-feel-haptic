#! /usr/bin/perl -w

# Perl script to run usb-robot-slave, to test the iFeel mouse

# requires Time::HiRes

#    runslave2.pl, a testing program
#    Copyright (C) 2001  Adam Goode
#
#    This program is free software; you can redistribute it and/or modify
#    it under the terms of the GNU General Public License as published by
#    the Free Software Foundation; either version 2 of the License, or
#    (at your option) any later version.
#
#    This program is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    GNU General Public License for more details.
#
#    You should have received a copy of the GNU General Public License
#    along with this program; if not, write to the Free Software
#    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
#
#
#    You can contact the author at his email address: adam@evdebs.org

use Time::HiRes qw(usleep);

open SLAVE, "|usb-robot/usb-robot-slave vendor=0x046d product=0xc030";

$init_string = "decoding hex\nencoding hex\n";
$header_string = "transfer type=control dir=out requesttype=0x21 request=0x09 value=0x0200 index=0x0000 size=7\n";

$| = 1;
select SLAVE;
$| = 1;
select STDOUT;


print SLAVE $init_string;

$strength = 255;
$pitch = 0;
$length = 0;

$unknown1 = hex("11");
$unknown2 = hex("0a");

print SLAVE "$header_string\n", "14 00 00 00 00 00 00";

#while (<>) {
while(1) {

    #$current_time = $thing[0] * 1000000;
    #usleep ($current_time - $prev_time);

    #$_ = $thing;

    $thing = sprintf "%.2x %.2x %.2x %.2x 00 %.2x 00",
    $unknown1, $unknown2, $strength, $pitch, $length;

    print SLAVE "$header_string\n", "$thing\n";
    print "$thing\n";
    print "strength: $strength, pitch: $pitch, length: $length\n";

    usleep(100000);# ($length *200000);

    #$prev_time = $current_time;
    $pitch = int rand(20);
#    #$length++;
#    $strength++;
    
}
