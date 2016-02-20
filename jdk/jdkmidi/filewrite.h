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

// Modified by Nicola Cassetta to allow I/O by C++ streams

#ifndef _JDKMIDI_FILEWRITE_H
#define _JDKMIDI_FILEWRITE_H

#include <fstream>

#include "midi.h"
#include "msg.h"
#include "sysex.h"
#include "file.h"


class MIDIFileWriteStream {
public:
    MIDIFileWriteStream() {}
    virtual ~MIDIFileWriteStream() {}

    virtual long Seek( long pos, int whence=SEEK_SET ) = 0;
    virtual int WriteChar( int c ) = 0;
};


/* Inherited from MIDIFileWriteStream, is used by FileReadMultiTrack for saving MIDI files
 * My version uses fstreams and doesn't open/close them, so it can be used for appending
 * (as we do in CompSong::Save)
*/

class MIDIFileWriteStreamFile : public MIDIFileWriteStream {
public:
    MIDIFileWriteStreamFile( const char *fname );
    MIDIFileWriteStreamFile(std::ostream* ofs);
    MIDIFileWriteStreamFile( FILE *f_ );
    virtual ~MIDIFileWriteStreamFile();

    long Seek( long pos, int whence=SEEK_SET );
    int WriteChar( int c );
    bool IsValid();

private:
    FILE *f;
    std::ostream* outfs;
    std::streampos begin;
    bool del;
};






  class MIDIFileWrite : protected MIDIFile
    {
    public:
      MIDIFileWrite( MIDIFileWriteStream *out_stream_ );
      virtual   	~MIDIFileWrite();


      bool ErrorOccurred()          	{ return error;         }
      unsigned long   GetFileLength()         { return file_length;   }
      unsigned long   GetTrackLength()        { return track_length;  }
      void    ResetTrackLength()      { track_length=0;       }
      void    ResetTrackTime()        { track_time=0;         }

      void    WriteFileHeader(
        int format,
        int ntrks,
        int division
        );

      void    WriteTrackHeader( unsigned long length );

      void    WriteEvent( const MIDITimedMessage &m );
      void    WriteEvent( unsigned long time, const MIDISystemExclusive *e );
      void    WriteEvent( unsigned long time, unsigned short text_type, const char *text );
      void	  WriteEvent( const MIDITimedBigMessage &m );

      void    WriteMetaEvent( unsigned long time, unsigned char type, const unsigned char *data, long length );
      void    WriteTempo( unsigned long time, long tempo );
      void    WriteKeySignature( unsigned long time, char sharp_flat, char minor );
      void    WriteTimeSignature(
        unsigned long time,
        char numerator=4,
        char denominator_power=2,
        char midi_clocks_per_metronome=24,
        char num_32nd_per_midi_quarter_note=8
        );

      void    WriteEndOfTrack(unsigned long time );

      virtual void    RewriteTrackLength();

    protected:
      virtual	void    Error(char *s);

      void    WriteCharacter( uchar c )
        {
          if( out_stream->WriteChar( c )<0 )
            error=true;
        }

      void    Seek( long pos )
        {
          if( out_stream->Seek( pos )<0 )
            error=true;
        }

      void    IncrementCounters( int c )   { track_length+=c; file_length+=c; }

      void    WriteShort( unsigned short c );
      void    Write3Char( long c );
      void    WriteLong( unsigned long c );

      int     WriteVariableNum( unsigned long n );

      void    WriteDeltaTime( unsigned long time );

    private:
      bool error;
      bool within_track;
      unsigned long   file_length;
      unsigned long   track_length;
      unsigned long   track_time;
      unsigned long   track_position;
      uchar   running_status;

      MIDIFileWriteStream *out_stream;

    };


#endif







