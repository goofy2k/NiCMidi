/*
 *   NiCMidi - A C++ Class Library for MIDI
 *
 *   Copyright (C) 2004  J.D. Koftinoff Software, Ltd.
 *   www.jdkoftinoff.com jeffk@jdkoftinoff.com
 *   Copyright (C) 2020  Nicola Cassetta
 *   https://github.com/ncassetta/NiCMidi
 *
 *   This file is part of NiCMidi.
 *
 *   NiCMidi is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   NiCMidi is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with NiCMidi.  If not, see <http://www.gnu.org/licenses/>.
 */


/// \file
/// Contains the definition of the class MIDIMatrix.


#ifndef _JDKMIDI_MATRIX_H
#define _JDKMIDI_MATRIX_H

#include "msg.h"
#include "processor.h"


///
/// This MIDIProcessor subclass implements a matrix which keeps track of notes on and hold pedal for
/// every channel. Every time you call Process() giving it a MIDITimedMessage it updates the count of
/// notes on and pedal (it does nothing if the message is not a note or pedal message). It is used by
/// MIDIDriver and MIDISequencerTrackState.
///
class  MIDIMatrix : public MIDIProcessor {
    public:
        /// The constructor creates an empty matrix
                        MIDIMatrix();
        /// The destructor
        virtual        ~MIDIMatrix() {}

        /// Resets the matrix (no notes on, no pedal hold)
        virtual void    Reset();
        /// Processes the given MIDI message updating the matrix. Returns **true** if the message has changed some matrix
        /// parameter, **false** otherwise
        virtual bool    Process(MIDITimedMessage* msg);
        /// Returns the total number of notes on
        int             GetTotalCount() const                           { return total_count; }
        /// Returns the number of notes on for given channel (channels from 0 to 15)
        int             GetChannelCount(int channel) const              { return channel_count[channel]; }
        /// Returns the number of notes on given the channel and the note MIDI value
        int             GetNoteCount(int channel, int note) const       { return note_on_count[channel][note]; }
        /// Returns **true** if pedal is holding on given channel
        bool            GetHoldPedal(int channel) const                 { return hold_pedal[channel]; }

    protected:

        /// Decrements the note count for the given channel and note
        virtual void    DecNoteCount(int channel, int note);
        /// Increments the note count for the given channel and note
        virtual void    IncNoteCount (int channel, int note);
        /// Clear the note count and the pedal on the given channel
        virtual void    ClearChannel(int channel);
        /// Called by Process() for non note and non pedal messages. You can redefine it if you
        /// want your own processing (currently does nothing)
        virtual void    OtherMessage(const MIDIMessage* msg) {}
        /// Sets the note count
        void            SetNoteCount(unsigned char chan, unsigned char note, unsigned char val)
                                                                        { note_on_count[chan][note] = val; }
        /// Sets the channel note count
        void            SetChannelCount(unsigned char chan, int val)    { channel_count[chan] = val; }

    private:
        unsigned        char note_on_count[16][128];    // The note matrix
        int             channel_count[16];              // The number of notes sounding for everu channel
        bool            hold_pedal[16];                 // The pedal status for every channel
        int             total_count;                    // The total number of sounding notes
};


#endif


