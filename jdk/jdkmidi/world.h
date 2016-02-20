/*
 *  libjdkmidi-2004 C++ Class Library for MIDI
 *
 *  Copyright (C) 2004  J.D. Koftinoff Software, Ltd.
 *  www.jdkoftinoff.com
 *  jeffk@jdkoftinoff.com
 *
 *  *** RELEASED UNDER THE GNU GENERAL PUBLIC LICENSE (GPL) April 27, 2004 ***
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/
/*
**	Copyright 1986 to 1998 By J.D. Koftinoff Software, Ltd.
**
**	All rights reserved.
**
**	No one may duplicate this source code in any form for any reason
**	without the written permission given by J.D. Koftinoff Software, Ltd.
**
*/

// Left old jdkmidi world.h (better than jdksmidi!)

#ifndef _JDKMIDI_WORLD_H
#define _JDKMIDI_WORLD_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ENTER(a)
#define DBG(a)
#define OSTYPE( a,b,c,d )       ((a)*0x1000000 + (b)*0x10000 + (c)*0x100 + (d) )

typedef unsigned char uchar;
typedef unsigned long ulong;
typedef unsigned int uint;
typedef unsigned short ushort;

const int           DEFAULT_CLKS_PER_BEAT = 120;
const int           DEFAULT_MAX_NUM_TRACKS = 64;


typedef unsigned long MIDIClockTime;
const MIDIClockTime TIME_INFINITE = 0xffffffff;

#endif


