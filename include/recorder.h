/*
 *   NiCMidi - A C++ Class Library for MIDI
 *
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
/// Contains the definitions of classes MIDIMultiTrackCopier and MIDIRecorder.


#ifndef RECORDER_H_INCLUDED
#define RECORDER_H_INCLUDED

#include "tick.h"
#include "multitrack.h"
#include "process.h"

#include <atomic>
#include <vector>



class MIDIMultiTrackCopier {
    public:
                                        MIDIMultiTrackCopier();
        virtual                         ~MIDIMultiTrackCopier();

        void                            Reset();
        MIDITrack*                      GetTrackDest(unsigned int n) const;

        void                            SetTrackDest(unsigned int n, MIDITrack* trk);
        void                            ResetTrackDest(unsigned int n);

        void                            Copy();

    private:
        MIDIMultiTrack* source;
        std::vector<MIDITrack*> dest;
};

/*
///
/// Stores current MIDI general parameters for a MIDISequencer object, embedding a MIDISequencerTrackState for each track.
/// It contains a MIDIMultiTrackIterator, allowing to set a 'now' time: when current time changes (because
/// the sequencer is playing or time is changed by the user) the object keep tracks of current timesig, keysig,
/// tempo(BPM), marker, measure and beat data. Moreover it contains an independent MIDISequencerTrackState
/// for every MIDI Track of the MIDISequencer, and you can examine them for knowing actual track parameters.
/// It inherits from the pure virtual MIDIProcessor: the MIDISequencer sends to it MIDI messages and it
/// processes them remembering actual parameters and notifying changes to the GUI.
/// All methods and attributes are public because they are used by MIDISequencer class; the advanced class
/// AdvancedSequencer allows you to know actual parameters without directly examining them, so the use of this
/// class is mainly internal.
/// However, you could subclass it if you want to keep track of other parameters.
///
class MIDIRecorderState : public MIDIProcessor {
// Doesn't inherit from MIDISequencerGUINotifier because notifier and sequencer must be independent objects
// (notifier is used also by the MIDIManager)
// All is public: used by various classes
    public:
        /// The constructor is called by the MIDISequencer class constructor, which sets appropriate
        /// values for parameters.
                                MIDIRecorderState(MIDIMultiTrack *multitrack_,
                                                  MIDISequencerGUINotifier *n = 0);
        /// The copy constructor. \note This only copies the pointers to the sequencer and the notifier,
        /// so you can use it only if you are copying different states of the same MIDISequencer
        /// class instance.
                                MIDIRecorderState(const MIDIRecorderState &s);
        /// The destructor. The sequencer and notifier pointers are not owned by the class and they
        /// won't freed.
        virtual                 ~MIDIRecorderState();
        /// The assignment operator. See the note to the copy constructor.
        const MIDIRecorderState& operator= (const MIDIRecorderState &s);

        /// Resets the state to default values. These are: cur_clock = 0, tempo = 120 bpm,
        /// time = 4/4, keysig = C Maj, no marker. Moreover resets all track states (see
        /// MIDISequencerTrackState::Reset()).
        void                    Reset();
        /// This is the process function inherited from MIDIProcessor. When you get a MIDI message
        /// from the sequencer, it is processed by the state, which updates its parameters and
        /// notifies the GUI if required.
        bool                    Process( MIDITimedMessage* msg );
        /// Notifies the GUI when something happens (a parameter was changed,
        /// current time is moved, etc.)
        void                    Notify(int group, int item = 0) const;
        /// These are used for notifying the GUI when something happens (a parameter was changed,
        /// current time is moved, etc.)
        void                    NotifyTrack(int item) const;

        MIDISequencerGUINotifier* notifier;         ///< The notifier
        MIDIMultiTrack*         multitrack;         ///< The MIDIMultiTrack holding MIDI messages
        MIDIMultiTrackIterator  iterator;           ///< The iterator for moving along the multitrack

        MIDIClockTime           cur_clock;          ///< The current MIDI clock in MIDI ticks
        float                   cur_time_ms;        ///< The current clock in milliseconds
        unsigned int            cur_beat;           ///< The current beat in the measure (1st beat is 0)
        unsigned int            cur_measure;        ///< The current measure (1st measure is 0)
        MIDIClockTime           beat_length;        ///< The duration of a beat
        MIDIClockTime           next_beat_time;     ///< The MIDI time of the next beat (for internal use)

        float                   tempobpm;           ///< The current tempo in beats per minute
        char                    timesig_numerator;  ///< The numerator of current time signature
        char                    timesig_denominator;///< The denominator of current time signature
        //char                    keysig_sharpflat;   ///< The current key signature accidents (
        //char                    keysig_mode;        ///< Major mode (0) or minor (1)
        //std::string             marker_text;        ///< The current marker
        //std::vector<MIDISequencerTrackState*>
        //                        track_states;       ///< A track state for every track
        int                     last_event_track;   ///< Internal use
        MIDIClockTime           last_beat_time;     ///< Internal use
        static int              metronome_mode;     ///< Flag affecting how metronome beat is calculated
};
*/


///
/// A MIDITickComponent which can record MIDI messages incoming from a MIDI in port, putting them into an internal
/// MIDIMultiTrack. You can select the port, channel and MIDI notes of the metronome clicks; moreover you can have
/// three types of click: the ordinary (beat) click, the measure click (first beat of a measure) and a subdivision
/// click. If you enable measure clicks the metronome can count the measures and the beats of a measure (so you can
/// represent them in a graphical interface).
///
class MIDIRecorder : public MIDITickComponent {
    public:
        /// The constructor.
                                        MIDIRecorder();
        /// The destructor
        virtual                         ~MIDIRecorder();
        virtual void                    Reset();

        /// Returns a pointer to the internal MIDIMultiTrack.
        MIDIMultiTrack*                 GetMultiTrack() const           { return multitrack; }
        /// Returns the recording tempo in bpm.
        float                           GetTempo() const                { return tempobpm; }
        /// Returns the start MIDI time of the recording.
        MIDIClockTime                   GetStartRecTime() const         { return rec_start_time; }
        /// Returns the end MIDI time of the recording.
        MIDIClockTime                   GetEndRecTime() const           { return rec_end_time; }
        /// Sets the recording tempo.
        /// \param t the tempo in bpm
        void                            SetTempo(float t);

        //void                            SetEnableRec(bool on_off)       { rec_on.store(on_off); }
        /// Sets the recording start time.
        /// \param t the MIDI clock time
        void                            SetStartRecTime(MIDIClockTime t)   { rec_start_time = t; }
        void                            SetStartRecTime(unsigned int meas, unsigned int beat = 0);

        void                            SetEndRecTime(MIDIClockTime t)      { rec_start_time = t; }
        void                            SetEndRecTime(unsigned int meas, unsigned int beat = 0);

        /// Enables a MIDI in port in the system for recording.
        /// \param port the system port id (you can use MIDIManager::GetNumMIDIIns() and
        /// MIDIManager::GetMIDIInName() for inspecting them).
        /// \param en_chans if *true* (default) enables recording from all channels, creating a MIDITrack in
        /// the multitrack for every one, otherwise you must set the recording channel with EnableChannel()
        void                            EnablePort(unsigned int port, bool en_chans = true);
        /// Enables a specific MIDI channel for recording, creating a track for it in the internal MIDIMultiTrack.
        /// \param port the system port id  (see EnablePort()); if it isn't enabled yet, enables it.
        /// \param ch the MIDI channel (in the range 0...15).
        void                            EnableChannel(unsigned int port, unsigned int ch);
        /// Disables a MIDI in port from recording, deleting all tracks associated with it in the internal
        /// MIDIMultiTrack. If the port is not enabled it does nothing.
        /// \param port the system port id (see EnablePort())
        void                            DisablePort(unsigned int port);
        /// Disables a specific MIDI channel from recording, deleting its track in the internal MIDIMultiTrack.
        /// \param port the system port id (see EnablePort())
        /// \param ch the MIDI channel (in the range 0...15).
        void                            DisableChannel(unsigned int port, unsigned int ch);

        /// Sets the current time to the beginning of the song, updating the internal status. This method is
        /// thread-safe and can be called during playback. Notifies the GUI a GROUP_ALL event to signify a
        /// full GUI reset.
        void                            GoToZero()                      { GoToTime(0); }
        /// Sets the current time to the given MIDI time, updating the internal status. This is as
        /// MIDISequencer::GoToTime() but uses a better algorithm and sends to the MIDI out ports all the
        /// appropriate sysex, patch, pitch bend and control change messages.
        bool                            GoToTime(MIDIClockTime time_clk);
        /// Same as GoToTime(), but the time is given in milliseconds.
        bool                            GoToTimeMs(float time_ms);

        /// Starts the recording from the enabled ports and channels.
        virtual void                    Start();
        /// Stops the recording from the enabled ports and channels.
        virtual void                    Stop();

    protected:
        /// Implements the static method inherited by MIDITickComponent and called at every timer tick.
        /// It only calls the member TickProc().
        static void                     StaticTickProc(tMsecs sys_time, void* pt);
        /// Implements the pure virtual method inherited from MIDITickComponent (you must not call it directly).
        virtual void                    TickProc(tMsecs sys_time);

        /// \cond EXCLUDED
        MIDIMultiTrack*                 multitrack;         // The internal MIDIMultiTrack
        std::vector<std::vector<MIDITrack*>*> en_ports;     // A vector which keeps track of the corrispondence between
                                                            // channels and tracks in the multitrack

        tMsecs                          rec_time_offset;    // The time between time 0 and sequencer start
        //tMsecs                          sys_time_offset;    ///< The time between the timer start and the sequencer start
        float                           tempobpm;           // The recording tempo
        MIDIClockTime                   rec_start_time;     // The MIDIClockTime of the beginning of recording
        MIDIClockTime                   rec_end_time;
        std::atomic<bool>               rec_on;
        /// \endcond
};



#endif // RECORDER_H_INCLUDED
