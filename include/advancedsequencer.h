/*
 *   NiCMidi - A C++ Class Library for MIDI
 *
 *   Copyright (C) 2004  J.D. Koftinoff Software, Ltd.
 *   www.jdkoftinoff.com jeffk@jdkoftinoff.com
 *   Copyright (C) 2021, 2022  Nicola Cassetta
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
/// Contains the definition of the classes MIDISequencerTrackProcessor and AdvancedSequencer.


#ifndef _JDKMIDI_ADVANCEDSEQUENCER_H
#define _JDKMIDI_ADVANCEDSEQUENCER_H

#include <vector>
#include <string>

#include "thru.h"
#include "multitrack.h"
#include "filereadmultitrack.h"
#include "sequencer.h"
#include "smpte.h"


///
/// A multipurpose MIDIProcessor implementing muting, soloing, rechannelizing, velocity scaling and transposing.
/// Moreover, you can set a custom MIDIProcessor pointer which extra-processes messages.
/// The AdvancedSequencer class contains an independent MIDISequencerTrackProcessor for every MIDI Track,
/// and allow you to set muting, transposing etc. by dedicated methods without directly dealing with this:
/// the only useful function for the user is probably the extra processing hook.
/// However, you could subclass this if you want to get new features.
///
class MIDISequencerTrackProcessor : public MIDIProcessor {
    public:
        /// The constructor. Default is no processing (MIDI messages leave the processor unchanged).
                        MIDISequencerTrackProcessor();
        /// The destructor does nothing.
        virtual         ~MIDISequencerTrackProcessor() {}

        // Copy constructor and assignment operator generated by the compiler

        /// Resets all values to default state (no processing at all).
        virtual void    Reset();
        /// Sets the extra processor for the track. The user processing is done before all internal
        /// processing, and if the user Process() function returns _false_ it is not done at all.
        /// If you want to eliminate an already set processor call this with 0 as parameter
        void            SetExternalProcessor(MIDIProcessor* proc)
                                                { extra_proc = proc; }
        /// Processes message msg, changing its parameters according to the state of the processor
        virtual bool    Process ( MIDITimedMessage *msg );

        bool            mute;                   ///< track is muted
        int             solo;                   ///< NO_SOLO, SOLOED, NOT_SOLOED
        unsigned int    velocity_scale;         ///< current velocity scale value for note ons, 100=normal
        int             rechannel;              ///< rechannelization value, or -1 for none
        int             transpose;              ///< amount to transpose note values
        MIDIProcessor   *extra_proc;            ///< extra midi processing for this track

        enum { NO_SOLO, SOLOED, NOT_SOLOED };
};


///
/// An enhanced MIDISequencer, able to directly load and play MIDI files and many more.
/// These are the improvements:
/// + There is no need to manually add it to the MIDIManager queue (the constructor does it)
/// + Has methods for loading/unloading a MIDI file into the internal MIDIMultiTrack
/// + Embeds a MIDIThru component wich can rechannelize and transpose incoming messages, so
///   you can play your keyboard while the sequencer is playing
/// + Embeds a MIDISequencerTrackProcessor for each track, allowing to transpose, rechannelize, scale velocity,
///   solo and mute the tracks
/// + Has improved methods for jumping from a time to another: if you start the sequencer from the middle of a
///   song it automatically sets appropriate MIDI controls, programs and sysex
///
class AdvancedSequencer : public MIDISequencer {
    public:
        /// Creates an AdvancedSequencer with 17 tracks (one for each channel plus the master track). Adds the
        /// sequencer to the MIDIManager queue of tick components, so you can immediately start to edit the
        /// MIDIMultiTrack or load MIDI files and play. It raises an exception if in your system there are no
        /// MIDI out ports; if in the system there are no MIDI in ports the embedded MIDIThru is not created
        /// and its features are disabled.
        /// \note If you create the object with this constructor the Internal multitrack is owned by the
        /// sequencer and will be deleted when you destroy it.
        /// \param n a pointer to a MIDISequencerGUINotifier. If you leave 0 the sequencer will not notify
        /// the GUI.
        /// \exception RtMidiError::INVALID_DEVICE if in the system are not present MIDI out ports.
                            AdvancedSequencer(MIDISequencerGUINotifier *n = 0);
        /// Creates an AdvancedSequencer from a given MIDIMultiTrack. Adds the sequencer to the MIDIManager queue
        /// of tick components, so you can immediately start to play. It raises an exception if in your system
        /// there are no MIDI out ports; if in the system there are no MIDI in ports the embedded MIDIThru is not
        /// created and its features are disabled.
        /// \note If you create the object with this constructor the Internal multitrack is **not** owned by the
        /// sequencer and won't be deleted when you destroy it.
        /// \param mlt the MIDIMultiTrack supplied by the user
        /// \param n a pointer to a MIDISequencerGUINotifier. If you leave 0 the sequencer will not notify
        /// the GUI.
        /// \exception RtMidiError::INVALID_DEVICE if in the system are not present MIDI out ports.
                            AdvancedSequencer(MIDIMultiTrack* mlt, MIDISequencerGUINotifier *n = 0);
        /// The destructor.
        virtual             ~AdvancedSequencer();

        /// Resets the status of the sequencer (does not empty the MIDIMultiTrack).
        /// Use this if you have modified the MIDIMultiTrack adding, moving or deleting tracks; this
        /// moves the time to 0 and resets all the processors.
        virtual void        Reset();
        /// Loads a MIDIFile into the internal MIDIMultiTrack. It can change the MIDIMultiTrack::clks_per_beat
        /// parameter according to the file signature. You can then play the MIDI content with the Play() method.
        /// \param fname the file name.
        /// \return **true** if the file has been loaded; if it fails returns **false** and leaves the multitrack
        /// in its previous status.
        virtual bool        Load(const char *fname);
        /// Copies the content of an external MIDIMultiTrack into the sequencer. It can change the
        /// MIDIMultiTrack::clks_per_beat parameter according to the multitrack signature. You can then play the
        /// MIDI content with the Play() method.
        /// \param tracks the MIDIMultiTrack to be copied.
        /// \return always returns **true**.
        virtual bool        Load(const MIDIMultiTrack* tracks);
        /// Clears the contents of the internal MIDIMultiTrack. Moreover it resets its MIDIMultiTrack::clks_per_beat
        /// parameter to \ref DEFAULT_CLKS_PER_BEAT.
        virtual void        UnLoad();

        /// Returns **true** if the internal MIDIMultiTrack is not empty.
        bool                IsLoaded() const                    { return file_loaded; }
        /// Returns the name of the loaded file (or an empty string if it is not yet defined).
        std::string         GetFileName()                       { return header.filename; }
        /// Returns the header of the loaded file.
        const MIDIFileHeader& GetFileHeader()                   { return header; }
        /// Returns the address of the MIDIThru tick component. This is NULL if in the system there are non MIDI
        /// in ports and the thru is disabled.
        MIDIThru*           GetMIDIThru()                       { return thru; }
        /// Returns a pointer to the MIDIThru tick component. This is NULL if in the system there are non MIDI
        /// in ports and the thru is disabled.
        const MIDIThru*     GetMIDIThru() const                 { return thru; }
        /// Returns **true** if MIDIThru is enabled (always **false** if the thru is not present).
        bool                GetMIDIThruEnable() const           { return thru ? thru->IsPlaying() : false; }
        /// Returns the output channel of the MIDIThru, -1 if the thru is not present.
        /// See \ref NUMBERING.
        int                 GetMIDIThruChannel() const          { return thru ? thru->GetOutChannel() : -1; }
        /// Returns the transpose amount of the MIDIThru, 0 if the thru is not present.
        int                 GetMIDIThruTranspose() const        { return thru ? thru_transposer->GetChannelTranspose(0) : 0; }
        /// Returns **true** if any track is soloed.
        bool                GetSoloMode() const;
        /// Returns **true** if a specific track is soloed
        /// \param trk_num the number of the track
        bool                GetTrackSolo(unsigned int trk_num) const
                                                { return (((MIDISequencerTrackProcessor *)track_processors[trk_num])->solo ==
                                                  MIDISequencerTrackProcessor::SOLOED); }
         /// Returns **true** if a specific track is muted
        /// \param trk_num the number of the track
        bool                GetTrackMute (unsigned int trk_num) const
                                                { return ((MIDISequencerTrackProcessor *)track_processors[trk_num])->mute; }
        /// Returns the number of measures of the sequencer.
        int                 GetNumMeasures() const              { return num_measures; }
        /// Returns the current measure number (first is 0).
        unsigned int        GetCurrentMeasure() const;
        /// Returns the number of current beat (first is 0).
        unsigned int        GetCurrentBeat() const;
        /// Returns the current MIDI time offset respect current beat.
        MIDIClockTime       GetCurrentBeatOffset() const;
        /// Returns the numerator of current time signature.
        int                 GetTimeSigNumerator() const;
        /// Returns the denominator of current time signature.
        int                 GetTimeSigDenominator() const;
        /// Return the number of sharps or flats of the current key signature.
        /// See MIDIMessage::GetKeySigSharpsFlats().
        int                 GetKeySigSharpsFlats() const;
        /// Returns the mode (major/minor) of the he current key signature.
        /// See MIDIMessage::GetKeySigMode().
        int                 GetKeySigMode() const;
        /// Returns the current marker text.
        std::string         GetCurrentMarker() const;
        /// Returns the name of the given track.
        std::string         GetTrackName(unsigned int trk_num) const;
        /// Returns the current MIDI volume for the given track (-1 if volume wasn't set at time 0).
        int                 GetTrackVolume(unsigned int trk_num) const;     // MIDI value or -1
        /// Returns the current MIDI program (patch) for the given track (-1 if the program wasn't set  at time 0).
        int                 GetTrackProgram (unsigned int trk_num) const;   // MIDI value or -1
        /// Returns the number of notes currently sounding on the given track (0 if the sequencer is not playing).
        int                 GetTrackNoteCount(unsigned int trk_num) const;
        /// Returns the velocity scale percentage for the given track.
        unsigned int        GetTrackVelocityScale(unsigned int trk_num) const;
        /// Returns the rechannelized channel for the given track (-1 if the track is not rechannelized).
        /// See \ref NUMBERING.
        int                 GetTrackRechannelize(unsigned int trk_num) const;
        /// If the track has channel messages all with same channel returns the channel, otherwise -1. See
        /// \ref NUMBERING.
        /// \note This is **not** const because it may reanalyze the track setting its status parameter.
        int                 GetTrackChannel(unsigned int trk_num);          // NOT const!!!
        /// Returns the transposing amount in semitones for the given track.
        int                 GetTrackTranspose(unsigned int trk_num) const;
        /// Returns the time offset (in MIDI ticks) assigned to the given track.
        int                 GetTrackTimeShift(unsigned int trk_num) const;
        /// Returns a pointer to the MIDISequencerTrackProcessor for the given track.
        MIDISequencerTrackProcessor*    GetTrackProcessor(unsigned int trk_num)
                                                { return (MIDISequencerTrackProcessor *)track_processors[trk_num]; }
        /// Returns a pointer to the MIDISequencerTrackProcessor for the given track.
        const MIDISequencerTrackProcessor* GetTrackProcessor(unsigned int trk_num) const
                                                { return (const MIDISequencerTrackProcessor *)track_processors[trk_num]; }
        /// Sets a name for the content of sequencer.
        void                SetFileName(std::string& fname)     { header.filename = fname; }
        /// Enables or disables the embedded MIDIthru. This has no effect if the thru is not present.
        /// \return **true** if the thru has been enabled/disabled, **false** otherwise.
        bool                SetMIDIThruEnable(bool on_off);
        /// Sets the out channel for MIDIthru. You can set -1 as parameter for leaving the channel of incoming messages
        /// unchanged. This has no effect if the thru is not present.
        /// See \ref NUMBERING.
        /// \return **true** if the channel has been set, **false** otherwise.
        bool                SetMIDIThruChannel(int chan);
        /// Sets a transpose amount in semitones for the messages coming from the MIDIThru.
        /// This has no effect if the thru is not present. See MIDIProcessorTransposer.
        /// \return **true** if the transpose has been set, **false** otherwise.
        bool                SetMIDIThruTranspose (int amt);
        /// Soloes the given track muting all others.
        /// \return **true** if _trk_num is a valid track number, **false** otherwise.
        bool                SetTrackSolo(unsigned int trk_num);
        /// Unsoloes the soloed track unmuting all others.
        void                UnSoloTrack();
        /// Mutes/unmutes the given track (it has no effect on others).
        /// \return **true** if _trk_num is a valid track number, **false** otherwise.
        bool                SetTrackMute(unsigned int trk_num, bool f);
        /// Unmutes all muted tracks (this has no effect on tracks muted by SoloTrack()).
        void                UnmuteAllTracks();
        /// Sets a track velocity scale in percentage for the given track.
        /// \return **true** if _trk_num is a valid track number, **false** otherwise.
        bool                SetTrackVelocityScale(unsigned int trk_num, unsigned int scale);
        /// Redirects all channel messages in the track on the given channel.
        /// This is done during playback by mean of a TrackProcessor, without changing original messages.
        /// Calling this with _chan_ = -1 disables the rechannelizing. See \ref NUMBERING
        /// \return **true** if _trk_num is a valid track number, **false** otherwise.
        bool                SetTrackRechannelize(unsigned int trk_num, int chan);
        /// Sets a transpose amount in semitones for the given track.
        /// See MIDIProcessorTransposer.
        /// \return **true** if _trk_num is a valid track number, **false** otherwise.
        bool                SetTrackTranspose(unsigned int trk_num, int amt);
        /// Sets the current time to the beginning of the song. This method is thread-safe and can be
        /// called during playback. Notifies the GUI a GROUP_ALL event to signify a full GUI reset.
        virtual void        GoToZero()                          { GoToTime(0); }
        /// Sets the current time to the given MIDI time. This is as MIDISequencer::GoToTime() but uses
        /// a faster algorithm and sends to the MIDI out ports all the appropriate sysex, patch, pitch bend
        /// and control change messages. Notifies the GUI a GROUP_ALL event to signify a full GUI reset.
        virtual bool        GoToTime(MIDIClockTime time_clk);
        /// Same as GoToTime(), but the time is given in milliseconds.
        virtual bool        GoToTimeMs(float time_ms);
        /// Sets the current time to the given measure and beat. This is as MIDISequencer::GoToMeasure() but uses
        /// a better algorithm and sends to the MIDI out ports all the appropriate sysex, patch, pitch bend and
        /// control change messages. Notifies the GUI a GROUP_ALL event to signify a full GUI reset.
        virtual bool        GoToMeasure(int measure, int beat = 0);


        /// Starts the sequencer playing from the current time.
        virtual void        Start();
        /// Stops the sequencer playing.
        virtual void        Stop();
        /// Sends a given MIDI message to an hardware port. This is thread-safe and can be called while playing.
        /// \param msg the MIDI message
        /// \param port the port id
        void                OutputMessage(MIDITimedMessage& msg, unsigned int port);
        /// Sets the parameters of the given SMPTE according to the loaded content. If the loaded
        /// file contains a MIDI SMPTE offset message, sets the parameters according to the offset and
        /// the frame rate of the message, otherwise set it to standard values (offset=0, frame=30FPS).
        /// \warning You cannot call this while the sequencer is playing.
        /// \return **true** if the SMPTE has been set, **false** otherwise.
        bool                SetSMPTE(SMPTE* s);
        /// This should be used to update the sequencer state after editing the multitrack.
        /// If you have added, deleted or edited events call this before moving time, getting events
        /// or playing. For changes in the track structure see InsertTrack(), DeleteTrack() and MoveTrack()).
        virtual void        UpdateStatus();

    protected:

        /// Internal use. It registers the state of the sequencer every MEASURES_PER_WARP measures, and create a
        /// std::vector of MIDISequencerState for a quicker jump from a time to another.
        void                ExtractWarpPositions();
        /// Internal use. When jumping from a time to another while the sequencer is playing, it examines all events
        /// between old and new time (or between start and new time, if lesser then old), sending to the ports the
        /// appropriate control, program, pitch bend and sysex messages in order to exactly reproduce the sequencer
        /// setting at the new time.
        void                CatchEventsBefore();
        /// Internal use. As above, but only on the given track (this is useful when a formerly muted track is unmuted,
        /// and needs to be set with appropriate controls, program etc.
        void                CatchEventsBefore(int trk_num);

        /// \cond EXCLUDED
        // The interval between measures in ExtractWarpPositions()
        static const int                    MEASURES_PER_WARP = 4;
        MIDIThru*                           thru;               // The embedded MIDI thru
        MIDIProcessorTransposer*            thru_transposer;    // Transposes thru note messages while playing
        int                                 num_measures;       // Number of measures
        MIDIFileHeader                      header;             // Stores the loaded file parameters
        int                                 file_loaded;        // True if the multitrack is not empty

        std::vector<MIDISequencerState>     warp_positions;     // Vector of MIDISequencerState objects for fast time moving
        /// \endcond

    private:
        bool                                owns_tracks;        // true if the multitrack is owned by the AdvancedSequencer
};


#endif // JDKSMIDI_ADVANCEDSEQUENCER_H

