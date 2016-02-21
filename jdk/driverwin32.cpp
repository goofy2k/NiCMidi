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

// Updated to reflect changes in jdksmidi

#include "jdkmidi/world.h"
#include <iostream>

#ifdef _WIN32
#include "jdkmidi/driverwin32.h"


//
///////////////////////////// MIDIDriverWin32 /////////////////////////////////////
//

bool MIDIDriverWin32::init_devices_flag = MIDIDriverWin32::InitDevices();


MIDIDriverWin32::MIDIDriverWin32 ( int queue_size ) :
    MIDIDriver ( queue_size ), in_handle ( 0 ), out_handle ( 0 ), in_open ( false ), out_open ( false ),
    timer_open ( false )
{
}


MIDIDriverWin32::~MIDIDriverWin32() {
    StopTimer();
    CloseMIDIInPort();
    CloseMIDIOutPort();
}


bool MIDIDriverWin32::OpenMIDIInPort ( int id ) {
    if ( !in_open ) {
        if ( midiInOpen ( &in_handle, id, ( DWORD ) win32_midi_in, ( DWORD ) this,
                          CALLBACK_FUNCTION ) != MMSYSERR_NOERROR )
            return false;
        midiInStart ( in_handle );
        in_open = true;
    }
    return true;
}


bool MIDIDriverWin32::OpenMIDIOutPort ( int id ) {
    if ( !out_open ) {
        if ( midiOutOpen ( &out_handle, id, 0, 0, CALLBACK_NULL ) != MMSYSERR_NOERROR )
            return false;
        sysex_buffer = new char[DEFAULT_SYSEX_BUFFER_SIZE];
        sysex_buffer_size = DEFAULT_SYSEX_BUFFER_SIZE;
        out_open = true;
    }
    return true;
}


void MIDIDriverWin32::CloseMIDIInPort() {
    if ( in_open ) {
        midiInStop ( in_handle );
        midiInClose ( in_handle );
        in_open = false;
    }
}


void MIDIDriverWin32::CloseMIDIOutPort() {
    if ( out_open ) {
        midiOutClose ( out_handle );
        delete[] sysex_buffer;
        sysex_buffer_size = 0;
        out_open = false;
        Reset();
    }
}


void MIDIDriverWin32::ResetMIDIOut() {
    if (out_open)
        midiOutReset (out_handle);
}


bool MIDIDriverWin32::StartTimer(int res) {
    if (!timer_open) {
        TIMECAPS tc;
        if (timeGetDevCaps (&tc, sizeof (TIMECAPS)) != TIMERR_NOERROR)
            return false;

        timer_res = res;

        if ( timer_res < (int)tc.wPeriodMin )
            timer_res = (int)tc.wPeriodMin;

        if ( timer_res > (int)tc.wPeriodMax )
            timer_res = (int)tc.wPeriodMax;

        timeBeginPeriod ( timer_res );
        timer_id = timeSetEvent (res, res, win32_timer, (DWORD)this, TIME_PERIODIC);

        if ( timer_id )
            timer_open=true;
    }
    return true;
}


void MIDIDriverWin32::StopTimer() {
    if ( timer_open ) {
        timeKillEvent (timer_id);
        timeEndPeriod (timer_res);
        timer_open = false;
    }
}


/*  ORIGINAL WORKING FUNCTION (doesn't send sysex)
bool MIDIDriverWin32::HardwareMsgOut ( const MIDITimedBigMessage &msg ) {
    if ( out_open ) {
      // dont send sysex or meta-events     // TODO: output MIDIsysex messages

        if ( msg.GetStatus() <0xff && msg.GetStatus() !=0xf0 ) {
            DWORD winmsg;
            winmsg = (((DWORD) msg.GetStatus() &0xff ) <<0) |
                     (((DWORD) msg.GetByte1() &0xff ) <<8) |
                     (((DWORD) msg.GetByte2() &0xff ) <<16 );


            if ( midiOutShortMsg (out_handle, winmsg) !=0 )
                return false;
        }
        return true;
    }
    return false;
}
*/


bool MIDIDriverWin32::HardwareMsgOut ( const MIDITimedBigMessage &msg )
{
    if ( out_open )
    {
        // msg is a channel message
        if ( msg.IsChannelMsg() )
        {
            DWORD winmsg;
            winmsg =
                ( ( ( DWORD ) msg.GetStatus() & 0xFF )       )
                | ( ( ( DWORD ) msg.GetByte1()  & 0xFF ) <<  8 )
                | ( ( ( DWORD ) msg.GetByte2()  & 0xFF ) << 16 );

            if ( midiOutShortMsg ( out_handle, winmsg ) != MMSYSERR_NOERROR )
            {
                char s[100];
                std::cout << "Driver FAILED to send short message " << msg.MsgToText(s) << std::endl;
                return false;
            }
        }

        else if ( msg.IsSysEx() )
        {
            MIDIHDR hdr;
// TODO: the buffer of the MIDISystemExclusive class holds only sysex bytes, without the 0xF0 status, so we
// need sysex_buffer and put 0xF0 as 1st charachter. If the status byte would be held
// in the MIDISystemExclusive this function would be simpler. This is possible, but perhaps there are
// compatibility problems with older software using GetBuf(). WHAT TO DO?


            if ( msg.GetSysEx()->GetLength() + 1 > sysex_buffer_size )
            {   // reallocate sysex_buffer
                delete[] sysex_buffer;
                sysex_buffer_size = msg.GetSysEx()->GetLength() + 1;
                sysex_buffer = new CHAR[ sysex_buffer_size ];
            }

            sysex_buffer[0] = msg.GetStatus();
            memcpy(sysex_buffer + 1, msg.GetSysEx()->GetBuf(), msg.GetSysEx()->GetLength());
            hdr.lpData = sysex_buffer;
            hdr.dwBufferLength = msg.GetSysEx()->GetLength() + 1;
            hdr.dwFlags = 0;

            if ( midiOutPrepareHeader( out_handle, &hdr, sizeof ( MIDIHDR ) ) != MMSYSERR_NOERROR ) {
                char s[100];
                std::cout << "Driver FAILED to send SysEx on PrepareHeader " << msg.MsgToText(s) << std::endl;
                return false;
            }

            if ( midiOutLongMsg( out_handle, &hdr, sizeof ( MIDIHDR ) ) != MMSYSERR_NOERROR )
            {
                char s[100];
                std::cout << "Driver FAILED to send SysEx on OurLongMsg " << msg.MsgToText(s) << std::endl;
                return false;
            }
            while ( midiOutUnprepareHeader( out_handle, &hdr, sizeof( MIDIHDR ) ) == MIDIERR_STILLPLAYING )
            {
                /* Should put a delay in here rather than a busy-wait */
            }
            char s[100];
            std::cout << "Driver sent Sysex msg " << msg.MsgToText(s) << std::endl;
        }

        else
        {
            char s[100];
            std::cout << "Driver skipped message " << msg.MsgToText(s) << std::endl;
        }

        return true;
    }

    // std::cout << "Driver not open!" << std::endl;
    return false;
}


void CALLBACK MIDIDriverWin32::win32_timer (
    UINT wTimerID, UINT msg, DWORD dwUser,
    DWORD dw1, DWORD dw2 ) {
    MIDIDriverWin32 *self = (MIDIDriverWin32* ) dwUser;
    self->TimeTick ( timeGetTime() );
}


void CALLBACK MIDIDriverWin32::win32_midi_in (
    HMIDIIN hMidiIn, UINT wMsg, DWORD dwInstance, DWORD dwParam1, DWORD dwParam2) {
    MIDIDriverWin32 *self = ( MIDIDriverWin32 * ) dwInstance;

    if ( wMsg==MIM_DATA ) {
        MIDITimedBigMessage msg;
        msg.SetStatus ( ( unsigned char ) ( dwParam1&0xff ) );
        msg.SetByte1 ( ( unsigned char ) ( ( dwParam1>>8 ) &0xff ) );
        msg.SetByte2 ( ( unsigned char ) ( ( dwParam1>>16 ) &0xff ) );
        msg.SetTime ( timeGetTime() );
        self->HardwareMsgIn ( msg );
    }
}


bool MIDIDriverWin32::InitDevices()
{
    MIDIINCAPS InCaps;
    num_in_devs = midiInGetNumDevs();
    in_dev_names = new char* [num_in_devs];
    for(UINT i = 0; i < num_in_devs; i++)
    {
        if ( midiInGetDevCaps( i, &InCaps, sizeof(InCaps) ) == MMSYSERR_NOERROR )
        {
            in_dev_names[i] = new char[DEVICENAME_LEN];
            strncpy( in_dev_names[i], InCaps.szPname, DEVICENAME_LEN-1 );
            in_dev_names[i][DEVICENAME_LEN-1] = 0;
        }
    }

    MIDIOUTCAPS OutCaps;
    num_out_devs = midiOutGetNumDevs();
    out_dev_names= new char* [num_out_devs];
    for(UINT i = 0; i < num_out_devs; i++)
    {
        if ( midiOutGetDevCaps( i, &OutCaps, sizeof(OutCaps) ) == MMSYSERR_NOERROR )
        {
            out_dev_names[i] = new char[DEVICENAME_LEN];
            strncpy( out_dev_names[i], OutCaps.szPname, DEVICENAME_LEN-1 );
            out_dev_names[i][DEVICENAME_LEN-1] = 0;
        }
    }
    return true;
}

#endif      // _WIN32


