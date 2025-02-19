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


#include "../include/sequencer.h"
#include "../include/manager.h"     // goes here, for SetPort()





////////////////////////////////////////////////////////////////////////////
//              class MIDISequencerTrackState                             //
////////////////////////////////////////////////////////////////////////////


MIDISequencerTrackState::MIDISequencerTrackState() {
    Reset();
}

/*
MIDISequencerTrackState::MIDISequencerTrackState(const MIDISequencerTrackState &s) {
    //std::cout << "MIDISequencerTrackState copy constructor" << std::endl;
    program = s.program;
    bender_value = s.bender_value;
    track_name = s.track_name;
    notes_are_on = s.notes_are_on;
    note_matrix = s.note_matrix;
    memmove(control_values, s.control_values, C_ALL_NOTES_OFF);
    got_good_track_name = s.got_good_track_name;
}
*/


void MIDISequencerTrackState::Reset() {
    program = -1;
    for (unsigned int i = 0; i < C_ALL_NOTES_OFF; i++)
        control_values[i] = -1;
    track_name = "";
    notes_are_on = false;
    bender_value = 0;
    note_matrix.Reset();
    got_good_track_name = false;
}


////////////////////////////////////////////////////////////////////////////
//                      class MIDISequencerState                          //
////////////////////////////////////////////////////////////////////////////


int MIDISequencerState::metronome_mode = MIDISequencer::FOLLOW_MIDI_TIMESIG_MESSAGE;

MIDISequencerState::MIDISequencerState(MIDIMultiTrack *m, MIDISequencerGUINotifier *n) :
    notifier(n), multitrack(m), iterator(m) {
    Reset();
}


MIDISequencerState::~MIDISequencerState() {
    for (unsigned int i = 0; i < track_states.size(); i++)
        delete track_states[i];
}


MIDISequencerState::MIDISequencerState(const MIDISequencerState& s) :
    notifier(s.notifier), multitrack(s.multitrack), iterator(s.iterator),
    cur_clock(s.cur_clock), cur_time_ms(s.cur_time_ms), cur_beat(s.cur_beat),
    cur_measure(s.cur_measure), beat_length(s.beat_length), number_of_beats(s.number_of_beats),
    next_beat_time(s.next_beat_time), tempobpm(s.tempobpm), tempo_scale(100), timesig_numerator(s.timesig_numerator),
    timesig_denominator(s.timesig_denominator), keysig_sharpflat(s.keysig_sharpflat),
    keysig_mode(s. keysig_mode), marker_text(s.marker_text), last_event_track(s.last_event_track),
    last_beat_time(s.last_beat_time), ms_per_clock(s.ms_per_clock), last_time_ms(s.last_time_ms),
    last_tempo_change(s.last_tempo_change), count_in_time(s.count_in_time), playing_status(s.playing_status)
{
    track_states.resize(multitrack->GetNumTracks());
    // TODO: these were added only for a bug checking; eliminate, this should never happen
    if (track_states.size() != s.track_states.size())
        std::cout << "MIDISequencerState constructor - Warning: the two vectors have different sizes" << std::endl;
    for (unsigned int i = 0; i < track_states.size(); i++)
        track_states[i] = new MIDISequencerTrackState(*s.track_states[i]);
}


const MIDISequencerState& MIDISequencerState::operator= (const MIDISequencerState& s) {
    notifier = s.notifier;
    multitrack = s.multitrack;
    iterator.SetState(s.iterator.GetState());
    cur_clock = s.cur_clock;
    cur_time_ms = s.cur_time_ms;
    cur_beat = s.cur_beat;
    cur_measure = s.cur_measure;
    beat_length = s.beat_length;
    number_of_beats = s.number_of_beats;
    next_beat_time = s.next_beat_time;
    tempobpm = s.tempobpm;
    tempo_scale = s.tempo_scale;
    timesig_numerator = s.timesig_numerator;
    timesig_denominator = s.timesig_denominator;
    keysig_sharpflat = s.keysig_sharpflat;
    keysig_mode = s.keysig_mode;
    marker_text = s.marker_text;

    for (unsigned int i = 0; i < track_states.size(); i++)
        delete track_states[i];
    track_states.resize(multitrack->GetNumTracks());
    // TODO: see above
    if (track_states.size() != s.track_states.size())
        std::cout << "MIDISequencerState operator= - Warning: the two vectors have different sizes" << std::endl;
    for (unsigned int i = 0; i < track_states.size(); i++)
        track_states[i] = new MIDISequencerTrackState(*s.track_states[i]);
    last_event_track = s.last_event_track;
    last_beat_time = s.last_beat_time;
    ms_per_clock = s.ms_per_clock;
    last_time_ms = s.last_time_ms;
    last_tempo_change = s.last_tempo_change;
    count_in_time = s.count_in_time;
    playing_status = s.playing_status;

    return *this;
}


void MIDISequencerState::Reset() {
    iterator.Reset();
    cur_clock = 0;
    cur_time_ms = 0.0;
    cur_beat = 0;
    cur_measure = 0;
    beat_length = multitrack->GetClksPerBeat();
    number_of_beats = MIDI_DEFAULT_TIMESIG_NUMERATOR;
    next_beat_time = 0;
    tempobpm = MIDI_DEFAULT_TEMPO;
    tempo_scale = 100;
    timesig_numerator = number_of_beats = MIDI_DEFAULT_TIMESIG_NUMERATOR;
    timesig_denominator = MIDI_DEFAULT_TIMESIG_DENOMINATOR;
    keysig_sharpflat = MIDI_DEFAULT_KEYSIG_KEY;
    keysig_mode = MIDI_DEFAULT_KEYSIG_MODE;
    marker_text = "";
    if (multitrack->GetNumTracks() != track_states.size()) {
        for (unsigned int i = 0; i < track_states.size(); i++)
            delete track_states[i];
        track_states.resize(multitrack->GetNumTracks());
        for (unsigned int i = 0; i < track_states.size(); i++)
            track_states[i] = new MIDISequencerTrackState;
    }
    else
        for (unsigned int i = 0; i < track_states.size(); i++)
            track_states[i]->Reset();
    last_event_track = -1;
    last_beat_time = 0;
    ms_per_clock = 6000000.0 / (MIDI_DEFAULT_TEMPO * tempo_scale * multitrack->GetClksPerBeat());
    last_time_ms = 0;
    last_tempo_change = 0;
    count_in_time = 0;
    playing_status = 0;
}


bool MIDISequencerState::Process(MIDITimedMessage *msg) {
    // is the event a NoOp?
    if(msg->IsNoOp())
        return false;                           // ignore event.

    // we are counting in: send only beats to the notifier
    if (playing_status & MIDISequencer::COUNT_IN_PENDING) {
        // notify the GUI the beat number (or measure) event
        if(count_in_time == 0)
            Notify(MIDISequencerGUIEvent::GROUP_TRANSPORT,
                   MIDISequencerGUIEvent::GROUP_TRANSPORT_MEASURE);
        Notify(MIDISequencerGUIEvent::GROUP_TRANSPORT,
            MIDISequencerGUIEvent::GROUP_TRANSPORT_BEAT);
        count_in_time += beat_length;
        return true;
    }

    // set new time
    if (msg->GetTime() != cur_clock) {
        cur_clock = msg->GetTime();
        cur_time_ms = last_time_ms + (cur_clock - last_tempo_change) * ms_per_clock;
    }

    // is the event a beat marker?
    if (msg->IsBeatMarker()) {
        // update our beat count
        if ( last_beat_time != next_beat_time ) {               // if at start, we must not upgrade beat count
            // upgrade beat number
            cur_beat++;
            if(cur_beat >= number_of_beats) {
                // upgrade measure number
                cur_beat = 0;
                ++cur_measure;
            }
        }

        // set the status parameters
        last_beat_time = cur_clock;
        next_beat_time += beat_length;

        // now notify the GUI that the beat number (and eventually the measure) is changed
        if(cur_beat == 0)
            Notify(MIDISequencerGUIEvent::GROUP_TRANSPORT,
                   MIDISequencerGUIEvent::GROUP_TRANSPORT_MEASURE);
        Notify(MIDISequencerGUIEvent::GROUP_TRANSPORT,
            MIDISequencerGUIEvent::GROUP_TRANSPORT_BEAT);
    }

    // is the event a MIDI channel message?
    else if(msg->IsChannelMsg()) {
        MIDISequencerTrackState* const t_state = track_states[last_event_track];
        if( msg->GetType()==PITCH_BEND )        // is it a bender event?
            // yes, remember the bender wheel value
             t_state->bender_value = msg->GetBenderValue();
        else if( msg->IsControlChange() ) {     // is it a control change event?
            // don't monitor system channel messages
            if (msg->GetController() < C_ALL_NOTES_OFF) {
                t_state->control_values[msg->GetController()] = msg->GetControllerValue();
                // notify event
                if(msg->GetController() == C_MAIN_VOLUME)
                    NotifyTrack(MIDISequencerGUIEvent::GROUP_TRACK_VOLUME);
                else if(msg->GetController() == C_PAN)
                    NotifyTrack(MIDISequencerGUIEvent::GROUP_TRACK_PAN);
                else if(msg->GetController() == C_CHORUS_DEPTH)
                    NotifyTrack(MIDISequencerGUIEvent::GROUP_TRACK_CHR);
                else if( msg->GetController() == C_EFFECT_DEPTH )
                    NotifyTrack(MIDISequencerGUIEvent::GROUP_TRACK_REV);
            }
        }
        else if(msg->IsProgramChange()) {       // is it a program change event?
            // yes, update the current program change value
            t_state->program = msg->GetProgramValue();
            NotifyTrack(MIDISequencerGUIEvent::GROUP_TRACK_PROGRAM);
        }
        // pass the message to our note matrix to keep track of all notes on
        // on this track
        if(t_state->note_matrix.Process(msg)) {
            // did the "any notes on" status change?
            if((t_state->notes_are_on && t_state->note_matrix.GetTotalCount() == 0) ||
               (!t_state->notes_are_on && t_state->note_matrix.GetTotalCount() > 0) ) {
                // yes, toggle our notes_are_on flag
                t_state->notes_are_on = !(t_state->notes_are_on);
                // and notify the gui about the activity on this track
                NotifyTrack(MIDISequencerGUIEvent::GROUP_TRACK_NOTE);
            }
        }
        return true;
    }

    // is the event a meta-event?
    else if(msg->IsMetaEvent()) {
        MIDISequencerTrackState* const t_state = track_states[last_event_track];    // needed in META TRACK NAME
        if(msg->IsTempo()) {                    // is it a tempo event?
            tempobpm = msg->GetTempo();
            ms_per_clock = 6000000.0 / (tempobpm * tempo_scale * multitrack->GetClksPerBeat());
            last_time_ms = cur_time_ms;
            last_tempo_change = cur_clock;
            //if(tempobpm < 1 )
            //tempobpm=MIDI_DEFAULT_TEMPO;
            Notify(MIDISequencerGUIEvent::GROUP_CONDUCTOR,
                   MIDISequencerGUIEvent::GROUP_CONDUCTOR_TEMPO);
        }
        else if(msg->IsTimeSig()) {             // is it a time signature event?
            // set internal prameters for the timesig
            timesig_numerator = msg->GetTimeSigNumerator();
            timesig_denominator = msg->GetTimeSigDenominator();
            MIDIClockTime old_beat_length = beat_length;
            // now set the metronome beat length
            if (metronome_mode == MIDISequencer::FOLLOW_MIDI_TIMESIG_MESSAGE)
                // in this mode the metronome length follows the MIDI message setting
                beat_length = msg->GetSysEx()->GetData(2) * multitrack->GetClksPerBeat() / 24;
            else {
                // in this mode the metronome beat follows the timesig denominator
                beat_length = multitrack->GetClksPerBeat() * 4 / timesig_denominator;
                if (metronome_mode == MIDISequencer::FOLLOW_THEORETIC_VALUE && !(timesig_numerator % 3) &&
                    timesig_numerator != 3)
                    // in this mode the metronome is multiplied by 3 if this i s a compound time
                    beat_length *= 3;
            }
            // adjust the next beat time (this was set according to the old value)
            next_beat_time = next_beat_time + beat_length - old_beat_length;
            number_of_beats = multitrack->GetClksPerBeat() *    // quarter length in MIDI ticks
                              timesig_numerator *               // number of symbolic beats
                              4 / timesig_denominator /         // length of a symbolic beat in quarters
                              beat_length;                      // beat length in MIDI ticks

            // notify the GUI of the timesig change
            Notify(MIDISequencerGUIEvent::GROUP_CONDUCTOR,
                   MIDISequencerGUIEvent::GROUP_CONDUCTOR_TIMESIG);
        }
        else if(msg->IsKeySig()) {              // is it a key signature event?
            keysig_sharpflat = msg->GetKeySigSharpsFlats();
            keysig_mode = msg->GetKeySigMode();
            Notify(MIDISequencerGUIEvent::GROUP_CONDUCTOR,
                   MIDISequencerGUIEvent::GROUP_CONDUCTOR_KEYSIG);
        }
        else if ( msg->IsMarkerText()) {        // is it a marker event?
            marker_text = std::string((const char *)msg->GetSysEx()->GetBuffer(), msg->GetSysEx()->GetLength());
            Notify(MIDISequencerGUIEvent::GROUP_CONDUCTOR,
                   MIDISequencerGUIEvent::GROUP_CONDUCTOR_MARKER);
        }
        else if( ( msg->GetMetaType()==META_TRACK_NAME
                 || msg->GetMetaType()==META_INSTRUMENT_NAME
                 || (!t_state->got_good_track_name && msg->GetMetaType()==META_GENERIC_TEXT && msg->GetTime()==0) )
                 && msg->GetSysEx() ) {           // is it a track name event?
            t_state->got_good_track_name = true;
            int len = msg->GetSysEx()->GetLength();
            t_state->track_name = std::string((const char *)msg->GetSysEx()->GetBuffer(), len);
            NotifyTrack(MIDISequencerGUIEvent::GROUP_TRACK_NAME);
        }
        return true;
    }
    return false;
}


void MIDISequencerState::Notify(int group, int item) const {
    if (notifier) {
        MIDISequencerGUIEvent ev(group, 0, item);
        notifier->Notify(ev);
    }
}


void MIDISequencerState::NotifyTrack(int item) const {
    if (notifier) {
        MIDISequencerGUIEvent ev(MIDISequencerGUIEvent::GROUP_TRACK,
                                last_event_track,
                                item);
        notifier->Notify(ev);
    }
}


void MIDISequencerState::GoForwardNoEvent(MIDIClockTime t) {
    MIDIClockTime offs = t - cur_clock;
    cur_clock = t;
    cur_time_ms += offs * ms_per_clock;
}



////////////////////////////////////////////////////////////////////////////
//                        class MIDISequencer                             //
////////////////////////////////////////////////////////////////////////////


MIDISequencer::MIDISequencer (MIDIMultiTrack *m, MIDISequencerGUINotifier *n) :
    MIDITickComponent(PR_SEQ, StaticTickProc),
    repeat_play_mode(false),
    repeat_start_meas(0), repeat_end_meas(0),
    time_shift_mode(false),
    play_mode(PLAY_BOUNDED),
    track_processors(m->GetNumTracks(), 0),
    state (m, n) {
    // checks if the system has almost a MIDI out
    if (!MIDIManager::IsValidOutPortNumber(0))
        throw RtMidiError("MIDISequencer needs almost a MIDI out port in the system\n", RtMidiError::INVALID_DEVICE);
    if (n)
        n->SetSequencer(this);
    beat_marker_msg.SetBeatMarker();
}


MIDISequencer::~MIDISequencer() {
    Stop();
    for (unsigned int i = 0; i < track_processors.size(); ++i)
        if (track_processors[i])
            delete track_processors[i];
}


void MIDISequencer::Reset() {
    Stop();
    time_shift_mode = false;
    state.iterator.SetTimeShiftMode(false);         // before state.Reset() which uses GetShiftedTime()
    state.Reset();                                  // syncronizes the multitrack with the state and goes to zero
    for (unsigned int i = 0; i < track_processors.size(); ++i)
        if (track_processors[i])
            delete track_processors[i];
    if (track_processors.size() != GetNumTracks())
        track_processors.resize(GetNumTracks());
    for (unsigned int i = 0; i < GetNumTracks(); ++i) {
        track_processors[i] = 0;
        state.multitrack->GetTrack(i)->SetTimeShift(0);
    }
    play_mode = PLAY_BOUNDED;
    bool notifier_mode = false;
    if(state.notifier) {
        notifier_mode = state.notifier->GetEnable();
        state.notifier->SetEnable(false);
    }
    // examine all the events at this specific time
    // and update the track states to reflect this time
    ScanEventsAtThisTime();     // we can't call this before we have syncronized the track processors

    // re-enable the gui notifier if it was enabled previously
    if(state.notifier) {
        state.notifier->SetEnable(notifier_mode);
        // cause a full gui refresh now
        state.notifier->Notify(MIDISequencerGUIEvent::GROUP_ALL);
    }
}


MIDIClockTime MIDISequencer::GetCurrentMIDIClockTime() const {
    MIDIClockTime time = state.cur_clock;
    if (IsPlaying()) {
        float ms_offset = GetCurrentTimeMs() - state.cur_time_ms;
        //float ms_per_clock = 60000.0 / (GetTempoWithScale() * state.multitrack->GetClksPerBeat());
        // now calculated by the state
        time += (MIDIClockTime)(ms_offset / state.ms_per_clock);
    }
    return time;
}


float MIDISequencer::GetCurrentTimeMs() const {
    return IsPlaying() ?
        MIDITimer::GetSysTimeMs() - sys_time_offset + dev_time_offset :
        state.cur_time_ms;
}


bool MIDISequencer::SetRepeatPlay(int on_off, int start_meas, int end_meas) {
    bool ret = true;
    std::lock_guard<std::recursive_mutex> lock(proc_lock);
    // change start and end measures if needed
    if (start_meas != -1)
        repeat_start_meas = (unsigned)start_meas;
    if (end_meas != -1)
        repeat_end_meas = (unsigned)end_meas;
    if (repeat_start_meas >= repeat_end_meas) {
        repeat_play_mode = false;
        ret = false;
    }
    else if (on_off != -1)
        repeat_play_mode = (bool)on_off;
    return ret;
}


void MIDISequencer::SetCountIn(bool on_off) {
    std::lock_guard<std::recursive_mutex> lock(proc_lock);
    (state.playing_status &= ~COUNT_IN_ENABLED) |= on_off * COUNT_IN_ENABLED;
}


bool MIDISequencer::SetTempoScale(unsigned int scale) {
    if (scale == 0)
        return false;
    std::lock_guard<std::recursive_mutex> lock(proc_lock);
    state.tempo_scale = scale;
    if (IsPlaying()) {
        dev_time_offset = state.cur_time_ms = MIDItoMs(state.cur_clock);
        sys_time_offset = MIDITimer::GetSysTimeMs();
    }
    return true;
}


void MIDISequencer::SetTimeShiftMode(bool f) {
    std::lock_guard<std::recursive_mutex> lock(proc_lock);
    time_shift_mode = f;
    if (!IsPlaying())
        state.iterator.SetTimeShiftMode(f);
    // Resyncronize the iterator with new times
    MIDISequencer::UpdateStatus();
}


void MIDISequencer::SetState(MIDISequencerState* s) {
    std::lock_guard<std::recursive_mutex> lock(proc_lock);
    if (IsPlaying())
        MIDIManager::AllNotesOff(); // no need to reset note matrix, thy are overwritten by the new state
    state = *s;
    if (state.notifier)                             // cause a complete GUI refresh
        state.notifier->Notify(MIDISequencerGUIEvent::GROUP_ALL);
}


void MIDISequencer::SetPlayMode(int mode) {
    std::lock_guard<std::recursive_mutex> lock(proc_lock);
    play_mode = mode;
    MIDIClockTime cur_time = 0;
    // if we have set bounded play and are after the last event
    // we must go to last event time
    if (play_mode == PLAY_BOUNDED && !GetNextEventTime(&cur_time))
        GoToTime(state.multitrack->GetEndTime());
}


bool MIDISequencer::SetTrackOutPort(unsigned int trk_num, unsigned int port) {
    if (!state.multitrack->IsValidTrackNumber(trk_num) || !MIDIManager::IsValidOutPortNumber(port))
        return false;
    int channel = state.multitrack->GetTrack(trk_num)->GetChannel();
    std::lock_guard<std::recursive_mutex> lock(proc_lock);
    if (IsPlaying() && port != GetTrackOutPort(trk_num) && channel != -1) {
        MIDIManager::GetOutDriver(GetTrackOutPort(trk_num))->AllNotesOff(channel);
        GetTrackState(trk_num)->note_matrix.Reset();
    }
    state.multitrack->GetTrack(trk_num)->SetOutPort(port);
    return true;
}


bool MIDISequencer::SetTrackProcessor(unsigned int trk_num, MIDIProcessor* p) {
    if (!state.multitrack->IsValidTrackNumber(trk_num))
        return false;
    Stop();
    track_processors[trk_num] = p;
    return true;
}


bool MIDISequencer::SetTrackTimeShift(unsigned int trk_num, int offset) {
    if (!state.multitrack->IsValidTrackNumber(trk_num))
        return false;
    int channel = state.multitrack->GetTrack(trk_num)->GetChannel();
    std::lock_guard<std::recursive_mutex> lock(proc_lock);

    if (IsPlaying() && channel != -1) {
        MIDIManager::GetOutDriver(GetTrackOutPort(trk_num))->AllNotesOff(channel);
        GetTrackState(trk_num)->note_matrix.Reset();
    }
    state.multitrack->GetTrack(trk_num)->SetTimeShift(offset);      // calls UpdateStatus()
    proc_lock.unlock();
    return true;
}


bool MIDISequencer::InsertTrack(int trk_num) {
    if (trk_num == -1) trk_num = GetNumTracks();        // if trk_num = -1 (default) append track
    bool ret = false;
    std::lock_guard<std::recursive_mutex> lock(proc_lock);
    if (state.multitrack->InsertTrack(trk_num)) {
        track_processors.insert(track_processors.begin() + trk_num, 0);
        MIDIClockTime now = state.cur_clock;            // remember current time
        state.Reset();                                  // reset the state (syncs the iterator and the track states)
        UpdateStatus();                                 // syncs other sequencer parameters (needed in AdvancedSequencer()
        GoToTime(now);                                  // returns to current time
        if (state.notifier)                             // cause a complete GUI refresh
            state.notifier->Notify(MIDISequencerGUIEvent::GROUP_ALL);
        ret = true;
    }
    return ret;
}


bool MIDISequencer::DeleteTrack(int trk_num) {
    if (trk_num == -1) trk_num = GetNumTracks() - 1;    // if trk_num = -1 (default) append track
    bool ret = false;
    std::lock_guard<std::recursive_mutex> lock(proc_lock);
    int chan = state.multitrack->GetTrack(trk_num)->GetChannel();
    if (IsPlaying() && chan != -1) {
        MIDIManager::GetOutDriver(GetTrackOutPort(trk_num))->AllNotesOff(chan);
        GetTrackState(trk_num)->note_matrix.Reset();
    }
    if (state.multitrack->DeleteTrack(trk_num)) {
        if (track_processors[trk_num])
            delete track_processors[trk_num];
        track_processors.erase(track_processors.begin() + trk_num);
        MIDIClockTime now = state.cur_clock;            // remember current time
        state.Reset();                                  // reset the state (syncs the iterator and the track states)
        UpdateStatus();                                 // syncs other sequencer parameters (needed in AdvancedSequencer)
        GoToTime(now);                                  // returns to current time
        if (state.notifier)                             // cause a complete GUI refresh
            state.notifier->Notify(MIDISequencerGUIEvent::GROUP_ALL);
        ret = true;
    }
    return ret;
}


bool MIDISequencer::MoveTrack(int from, int to) {
    if (from == to) return true;                        // nothing to do
    bool ret = false;
    std::lock_guard<std::recursive_mutex> lock(proc_lock);
    if (state.multitrack->MoveTrack(from, to)) {        // checks if from and to are valid
        MIDIProcessor* temp_processor = track_processors[from];
        track_processors.erase(track_processors.begin() + from);
        if (from < to)
            to--;
        track_processors.insert(track_processors.begin() + to, temp_processor);
        MIDIClockTime now = state.cur_clock;            // remember current time
        state.Reset();                                  // reset the state (syncs the iterator)
        UpdateStatus();                                 // syncs other sequencer parameters (needed in AdvancedSequencer)
        GoToTime(now);                                  // returns to current time
        if (state.notifier)                             // cause a complete GUI refresh
            state.notifier->Notify(MIDISequencerGUIEvent::GROUP_ALL);
        ret = true;
    }
    return ret;
}


void MIDISequencer::GoToZero() {
    // temporarily disable the gui notifier
    bool notifier_mode = false;
    std::lock_guard<std::recursive_mutex> lock(proc_lock);
    if(state.notifier) {
        notifier_mode = state.notifier->GetEnable();
        state.notifier->SetEnable(false);
    }
    // go to time zero
    state.Reset();

    // examine all the events at this specific time
    // and update the track states to reflect this time
    ScanEventsAtThisTime();

    // re-enable the gui notifier if it was enabled previously
    if(state.notifier) {
        state.notifier->SetEnable(notifier_mode);
        // cause a full gui refresh now
        state.notifier->Notify(MIDISequencerGUIEvent::GROUP_ALL);
    }
    if (IsPlaying()) {
        // update real time parameters
        dev_time_offset = 0.0;
        sys_time_offset = MIDITimer::GetSysTimeMs();
        MIDIManager::AllNotesOff();
    }
}


bool MIDISequencer::GoToTime(MIDIClockTime time_clk) {
    bool ret = true;

    // save sequencer state in the case of failure
    MIDISequencerState old_state = state;
    // temporarily disable the gui notifier
    bool notifier_mode = false;
    std::lock_guard<std::recursive_mutex> lock(proc_lock);
    if(state.notifier) {
        notifier_mode = state.notifier->GetEnable();
        state.notifier->SetEnable(false);
    }

    // OLD VERSION if(time_clk < state.cur_clock || time_clk == 0)
    if(time_clk <= state.cur_clock)     // we must restart also if cur_clock is equal to cur_clock, as we could
                                        // already have got some event. Moreover this is good if we have edited the
                                        // multitrack
        // start from zero if desired time is before where we are
        state.Reset();

    MIDIClockTime t;
    int trk;
    MIDITimedMessage msg;

    while (state.cur_clock < time_clk) {
        if (!GetNextEventTime(&t)) {    // no other events and PLAY_BOUNDED: we can't reach time_clk and return false
            state = old_state;          // refresh initial state
            ret = false;
            break;
        }
        if (t < time_clk)               // next event is before time_clk
            GetNextEvent (&trk, &msg);  // get it and continue
        else if (t == time_clk) {       // next event is at the right time
            MIDIMultiTrackIteratorState istate(state.iterator.GetState());  // save the state of the iterator
            GetNextEvent(&trk, &msg);           // get the event (cur_time becomes time_clk)
            state.iterator.SetState(istate);    // restore the iterator state, so ev is the next event
        }
        else                            // next event is after time_clk : set cur_time to time_clk
            state.GoForwardNoEvent(time_clk);
    }

    if (ret) {                          // we have effectively moved time
        // examine all the events at this specific time
        // and update the track states to reflect this time
        ScanEventsAtThisTime();
        if (IsPlaying()) {
            // update real time parameters
            dev_time_offset = state.cur_time_ms;
            sys_time_offset = MIDITimer::GetSysTimeMs();
            MIDIManager::AllNotesOff();
        }
    }
    // re-enable the gui notifier if it was enabled previously
    if(state.notifier) {
        state.notifier->SetEnable(notifier_mode);
        // cause a full gui refresh now
        if (ret)
            state.notifier->Notify(MIDISequencerGUIEvent::GROUP_ALL);
    }
    return ret;
}


bool MIDISequencer::GoToTimeMs(float time_ms) {
    bool ret = true;

    // save sequencer state in the case of failure
    MIDISequencerState old_state = state;
    // temporarily disable the gui notifier
    bool notifier_mode = false;
    std::lock_guard<std::recursive_mutex> lock(proc_lock);
    if(state.notifier) {
        notifier_mode = state.notifier->GetEnable();
        state.notifier->SetEnable(false);
    }

    // OLD VERSION if(time_ms < state.cur_time_ms || time_ms == 0.0)
    if(time_ms <= state.cur_time_ms)    // we must restart also if cur_clock is equal to cur_clock, as we could
                                        // already have got some event. Moreover this is good if we have edited the
                                        // multitrack
        // start from zero if desired time is before where we are
        state.Reset();

    float t;
    int trk;
    MIDITimedMessage msg;

    while (state.cur_time_ms < time_ms) {
        if (!GetNextEventTimeMs(&t)) {  // no other events and PLAY_BOUNDED: we can't reach time_ms and return false
            state = old_state;          // refresh initial state
            ret = false;
            break;
        }
        if (t < time_ms)                        // next event is before time_ms
            GetNextEvent (&trk, &msg);          // get it and continue
        else if (abs (t - time_ms) < 0.001){    // next event is at right time
            MIDIMultiTrackIteratorState istate(state.iterator.GetState());  // save the state of the iterator
            GetNextEvent(&trk, &msg);           // get the event
            state.iterator.SetState(istate);    // restore the iterator state, so ev is the next event
        }
        else {                          // next event is after time_ms : set cur_time to time_ms
            float offset_ms = time_ms - state.cur_time_ms;
            //float ms_per_clock = 6000000.0 / (state.tempobpm *    // see MIDIToMs()
            //        tscale * state.multitrack->GetClksPerBeat());
            state.cur_clock += (MIDIClockTime)(offset_ms / state.ms_per_clock);
            state.cur_time_ms = time_ms;
        }
    }

	if (ret) {                          // we have effectively moved time
        // examine all the events at this specific time
        // and update the track states to reflect this time
        ScanEventsAtThisTime();
        if (IsPlaying()) {
            // update real time parameters
            dev_time_offset = state.cur_time_ms;
            sys_time_offset = MIDITimer::GetSysTimeMs();
            MIDIManager::AllNotesOff();
        }
    }
    // re-enable the gui notifier if it was enabled previously
    if(state.notifier) {
        state.notifier->SetEnable( notifier_mode );
        // cause a full gui refresh now
        if (ret)
            state.notifier->Notify(MIDISequencerGUIEvent::GROUP_ALL);
    }
    return ret;
}


// This is simpler because every measure has as first event a beat event!
bool MIDISequencer::GoToMeasure (unsigned int measure, unsigned int beat) {
    bool ret = true;

    // save sequencer state in the case of failure
    MIDISequencerState old_state = state;
    // temporarily disable the gui notifier
    bool notifier_mode = false;
    std::lock_guard<std::recursive_mutex> lock(proc_lock);
    if (state.notifier) {
        notifier_mode = state.notifier->GetEnable();
        state.notifier->SetEnable (false);
    }

//    OLD VERSION if (measure < state.cur_measure ||
//         // ADDED FOLLOWING LINE:  this failed in this case!!!
//        (measure == state.cur_measure && beat < state.cur_beat) ||
//        (measure == 0 && beat == 0))
    if (measure < state.cur_measure ||
        (measure == state.cur_measure && beat <= state.cur_beat))
        state.Reset();

    int trk;
    MIDITimedMessage msg;

        // iterate thru all the events until cur-measure and cur_beat are
        // where we want them.
    if (measure > 0 || beat > 0) {          // if meas == 0 && beat == 0 nothing to do
        while(1) {
            if (!GetNextEvent(&trk, &msg)) {// no other events and PLAY_BOUNDED: we can't reach our time and return false
                state = old_state;          // refresh initial state
                ret = false;
                break;
            }
            if (msg.IsBeatMarker()) {       // there must be a beat marker at right time
                if(state.cur_measure == measure && state.cur_beat >= beat)
                    break;
            }
        }
    }

    if (ret) {                          // we have effectively moved time
        // examine all the events at this specific time
        // and update the track states to reflect this time
        ScanEventsAtThisTime();
        if (IsPlaying()) {
            // update real time parameters
            dev_time_offset = state.cur_time_ms;
            sys_time_offset = MIDITimer::GetSysTimeMs();
            MIDIManager::AllNotesOff();
        }
    }
    // re-enable the gui notifier if it was enabled previously
    if(state.notifier) {
        state.notifier->SetEnable( notifier_mode );
        // cause a full gui refresh now
        if (ret)
            state.notifier->Notify(MIDISequencerGUIEvent::GROUP_ALL);
    }
    return ret;
}


bool MIDISequencer::GetNextEvent(int *trk_num, MIDITimedMessage *msg) {
    MIDIClockTime t;
    bool ret = false;

    std::lock_guard<std::recursive_mutex> lock(proc_lock);
    // ask the iterator for the current event time
    if (state.iterator.GetNextEventTime(&t)) {
        // now auto set by state
        // move current time forward one event
        //MIDIClockTime new_clock;
        //GetNextEventTime(&new_clock);
        //if (new_clock != state.cur_clock) {
        //    state.cur_time_ms = MIDItoMs(new_clock);
        //    state.cur_clock = new_clock;
        //}

        // is the next beat marker before this event?
        if(state.next_beat_time <= t) {
            // yes, this is a beat event now.
            // say this event came on track 0, the conductor track
            state.last_event_track = *trk_num = 0;

            // put current info into beat marker message
            beat_marker_msg.SetTime(state.next_beat_time);
            *msg = beat_marker_msg;
            state.Process(msg);
        }
        else    {   // this event comes before the next beat
            MIDITimedMessage *msg_ptr;

            if(state.iterator.GetNextEvent(trk_num, &msg_ptr)) {
                state.last_event_track = *trk_num;

                // copy the event so Process can modify it
                *msg = *msg_ptr;

                if ((track_processors[*trk_num] && !track_processors[*trk_num]->Process(msg)) ||
                     !state.Process(msg))
                    msg->Clear();
            }
        }
        ret = true;
    }
    // we are after the last event, but must continue to play sending
    // only beat markers
    else if (play_mode == PLAY_UNBOUNDED) {
        // move current time forward one event
        //MIDIClockTime new_clock;
        //GetNextEventTime(&new_clock);
        //double new_time_ms = MIDItoMs(new_clock);

        // must set cur_clock AFTER GetNextEventTimeMs() is called
        // since GetNextEventTimeMs() uses cur_clock to calculate
        //state.cur_clock = new_clock;
        //state.cur_time_ms = new_time_ms;

        // the event is surely a beat marker
        // say this event came on track 0, the conductor track
        state.last_event_track = *trk_num = 0;

        // put current info into beat marker message
        beat_marker_msg.SetTime(state.next_beat_time);
        *msg = beat_marker_msg;

        state.Process(msg);
        ret = true;
    }
    return ret;
}


bool MIDISequencer::GetNextEventTime(MIDIClockTime *time_clk) {
    std::lock_guard<std::recursive_mutex> lock(proc_lock);
    // ask the iterator for the current event time
    bool ret = state.iterator.GetNextEventTime(time_clk);

    if(ret) {
        // if we have an event in the future, check to see if it is
        // further in time than the next beat marker
        if((*time_clk) >= state.next_beat_time)
            // ok, the next event is a beat - return the next beat time
            *time_clk = state.next_beat_time;
    }
    else if (play_mode == PLAY_UNBOUNDED)
        *time_clk = state.next_beat_time;
    return ret;
}


bool MIDISequencer::GetNextEventTimeMs(float *time_ms) {
    MIDIClockTime t;
    std::lock_guard<std::recursive_mutex> lock(proc_lock);
    bool ret = GetNextEventTime(&t);

    if(ret || play_mode == PLAY_UNBOUNDED) {
        MIDIClockTime offset = t - state.cur_clock;
        *time_ms = state.cur_time_ms + offset * state.ms_per_clock;
    }
    return ret;
}

/*
float MIDISequencer::MIDItoMs(MIDIClockTime t) {
    proc_lock.lock();
    int ev_num = 0;
    MIDIClockTime base_t = 0, delta_t = 0, now_t = 0;
    double ms_time = 0.0;
    MIDITimedMessage* msg;
    // we initialize this variable in the case of no tempo signature at the beginning
    // it wil be changed (see below) at first tempo change message
    float ms_per_clock = 6000000.0 / (MIDI_DEFAULT_TEMPO * (float)tempo_scale *
                                       state.multitrack->GetClksPerBeat());
    // if the multitrack has no tracks it is sufficient a multiplication
    if (state.multitrack->GetNumTracks() == 0) {
        proc_lock.unlock();
        return ms_per_clock * t;
    }
    MIDITrack* track = state.multitrack->GetTrack(0);
    while (now_t < t) {
        if (!track->IsValidEventNum(ev_num) || track->GetEventAddress(ev_num)->GetTime() >= t) {
            // next message doesn't exists or is at t or after t
            now_t = t;
            // calculate delta_time in MIDI clocks
            delta_t = now_t - base_t;
            // and add it in msecs to ms_time
            ms_time += (delta_t * ms_per_clock);
        }
        else {
            msg = track->GetEventAddress(ev_num);
            now_t = msg->GetTime();
            // if we have a tempo change must calculate the time
            // in msecs between the last tempo change and now
            if (msg->IsTempo()) {
                // calculate delta time in MIDI clocks
                delta_t = now_t - base_t;
                // and add it in msecs to ms_time
                ms_time += (delta_t * ms_per_clock);

                // calculate new milliseconds per clock: this comes from
                //  -true_bpm = old_tempo * tempo_scale / 100
                //  -clocks_per_sec = true_bpm * clks_per_beat / 60
                //  -clocks_per_ms = clocks_per_sec / 1000
                //  -ms_per_clock = 1 / clocks_per_ms
                ms_per_clock = 6000000.0 / (msg->GetTempo() *
                                (double)tempo_scale * state.multitrack->GetClksPerBeat());

                // update variables for next tempo change (or now_t == t)
                base_t = now_t;

            }
        }
        ev_num++;
    }
    proc_lock.unlock();
    return ms_time;
}
*/


float MIDISequencer::MIDItoMs(MIDIClockTime t) {
    if (t == 0)
        return 0.0;
    MIDIMultiTrackIterator iter(state.multitrack);
    MIDIClockTime last_tempo_t = 0, delta_t = 0, now_t = 0;
    float ms_time = 0.0;
    int trk_num;
    MIDITimedMessage* msg;

    std::lock_guard<std::recursive_mutex> lock(proc_lock);
    // enable only tracks with meta events in the iterator
    for (unsigned int i = 0; i < GetNumTracks(); i++) {
        int trk_status = GetTrack(i)->GetStatus();
        if (!(trk_status & MIDITrack::HAS_MAIN_META))
            iter.SetEnable(i, false);
    }

    // we initialize this variable in the case of no tempo signature at the beginning
    // it wil be changed (see below) at first tempo change message
    float ms_per_clock = 6000000.0 / (MIDI_DEFAULT_TEMPO * (float)state.tempo_scale *
                                       GetClksPerBeat());

    // look for tempo events
    while (iter.GetNextEvent(&trk_num, &msg)) {
        now_t = msg->GetTime();
        // calculate delta_time in MIDI clocks
        delta_t = now_t - last_tempo_t;
        float delta_ms = delta_t * ms_per_clock;
        if (ms_time + delta_ms >= t)
            break;

        // if we have a tempo change must calculate the time
        // in msecs between the last tempo change and now
        if (msg->IsTempo()) {
            // and add it in msecs to ms_time
            ms_time += delta_ms;
            last_tempo_t = now_t;

            // calculate new milliseconds per clock: this comes from
            //  -true_bpm = old_tempo * tempo_scale / 100
            //  -clocks_per_sec = true_bpm * clks_per_beat / 60
            //  -clocks_per_ms = clocks_per_sec / 1000
            //  -ms_per_clock = 1 / clocks_per_ms
            ms_per_clock = 6000000.0 / (msg->GetTempo() *
                           (float)state.tempo_scale * GetClksPerBeat());
        }
    }
    ms_time += (t - last_tempo_t) * ms_per_clock;
    return ms_time;
}



/*
MIDIClockTime MIDISequencer::MeasToMIDI(unsigned int meas, unsigned int beat, unsigned int offset) {
    unsigned int t_meas = 0, t_beat = 0;
    MIDIClockTime delta_t = 0, now_t = 0,  last_timesig_t = 0;
    MIDIClockTime time = 0;
    int trk_num;
    MIDITimedMessage* msg;
    // we initialize this variables in the case of no tempo signature at the beginning
    // they will be changed (see below) at first time change message
    MIDIClockTime clks_per_beat = GetClksPerBeat();
    MIDIClockTime clks_per_meas = clks_per_beat * MIDI_DEFAULT_TIMESIG_NUMERATOR;

    proc_lock.lock();
    // if the multitrack has no tracks it is sufficient a multiplication
    if (state.multitrack->GetNumTracks() == 0)
        return meas * clks_per_meas + beat * clks_per_beat + offset;
    MIDIMultiTrackIterator iter(state.multitrack);
    while (t_meas < meas || (t_meas == meas && t_beat < beat)) {
        if (!iter.GetNextEvent(&trk_num, &msg)) {
            time += delta_t;
            break;
        }
        now_t = msg->GetTime();
        delta_t = now_t - last_timesig_t;
        if (delta_t >= clks_per_meas) {
            unsigned int new_measures = delta_t / clks_per_meas;
            t_meas += new_measures;
            delta_t -= (clks_per_meas * new_measures);
            time = last_timesig_t = now_t;
        }
        if (msg->IsTimeSig()) {
            // now we must calculate the new values for
            // clks_per_meas and clks_per_beat
            int metro_mode = MIDISequencerState::metronome_mode;
            unsigned char num = msg->GetTimeSigNumerator();
            unsigned char den = msg->GetTimeSigDenominator();
            // in this mode the metronome length follows the MIDI message setting
            if (metro_mode == MIDISequencer::FOLLOW_MIDI_TIMESIG_MESSAGE)
                clks_per_beat = msg->GetSysEx()->GetData(2) * GetClksPerBeat() / 24;
            else {
                // in this mode the metronome beat follows the timesig denominator
                clks_per_beat = GetClksPerBeat() * 4 / den;
                if (metro_mode == MIDISequencer::FOLLOW_THEORETIC_VALUE && !(num % 3) && num != 3)
                    // in this mode the metronome is multiplied by 3 if this i s a compound time
                    clks_per_beat *= 3;
            }
            clks_per_meas = GetClksPerBeat() *      // quarter length in MIDI ticks
                            num *                   // number of symbolic beats
                            4 / den;                // length of a symbolic beat in quarters
        }
    }
    proc_lock.unlock();
    return time;
}
*/



MIDIClockTime MIDISequencer::MeasToMIDI(unsigned int meas, unsigned int beat, unsigned int offset) {

    if (meas == 0 && beat == 0 && offset == 0)
        return 0;
    MIDIMultiTrackIterator iter(state.multitrack);
    unsigned int t_meas = 0;
    MIDIClockTime last_timesig_t = 0, delta_t = 0, now_t = 0;
    MIDIClockTime time = 0;
    int trk_num;
    MIDITimedMessage* msg;

    std::lock_guard<std::recursive_mutex> lock(proc_lock);
    // enable only tracks with meta events in the iterator
    for (unsigned int i = 0; i < GetNumTracks(); i++) {
        int trk_status = GetTrack(i)->GetStatus();
        if (!(trk_status & MIDITrack::HAS_MAIN_META))
            iter.SetEnable(i, false);
    }

    // we initialize this variables in the case of no tempo signature at the beginning
    // they will be changed (see below) at first time change message
    MIDIClockTime clks_per_beat = GetClksPerBeat();
    MIDIClockTime clks_per_meas = clks_per_beat * MIDI_DEFAULT_TIMESIG_NUMERATOR;

    // look for timesig events
    while (iter.GetNextEvent(&trk_num, &msg)) {
        now_t = msg->GetTime();
        // calculate the delta time from the last time change
        delta_t = now_t - last_timesig_t;
        // this is a new measure
        if (delta_t >= clks_per_meas) {
            unsigned int new_measures = delta_t / clks_per_meas;
            // we are arrived to our measure: exit the loop
            if (t_meas + new_measures >= meas)
                break;
            // otherwise add new measures and continue
            t_meas += new_measures;
            delta_t -= (clks_per_meas * new_measures);
            time = last_timesig_t = now_t;
        }
        if (msg->IsTimeSig()) {
            // now we must calculate the new values for
            // clks_per_meas and clks_per_beat
            int metro_mode = MIDISequencerState::metronome_mode;
            unsigned char num = msg->GetTimeSigNumerator();
            unsigned char den = msg->GetTimeSigDenominator();
            // in this mode the metronome length follows the MIDI message setting
            if (metro_mode == MIDISequencer::FOLLOW_MIDI_TIMESIG_MESSAGE)
                clks_per_beat = msg->GetSysEx()->GetData(2) * GetClksPerBeat() / 24;
            else {
                // in this mode the metronome beat follows the timesig denominator
                clks_per_beat = GetClksPerBeat() * 4 / den;
                if (metro_mode == MIDISequencer::FOLLOW_THEORETIC_VALUE && !(num % 3) && num != 3)
                    // in this mode the metronome is multiplied by 3 if this i s a compound time
                    clks_per_beat *= 3;
            }
            clks_per_meas = GetClksPerBeat() *      // quarter length in MIDI ticks
                            num *                   // number of symbolic beats
                            4 / den;                // length of a symbolic beat in quarters
        }
    }
    time += (clks_per_meas * (meas - t_meas) + clks_per_beat * beat + offset);
    return time;
}








// Inherited from MIDITICK

void MIDISequencer::Start() {
    if (!IsPlaying()) {         // TODO: this is different from AdvancedSequencer one: what is correct?
        std::lock_guard<std::recursive_mutex> lock(proc_lock);      // could be called during autostop
        std::cout << "\t\tEntered in MIDISequencer::Start() ..." << std::endl;
        MIDIManager::OpenOutPorts();
        state.iterator.SetTimeShiftMode(true);
        if (GetCountInEnable()) {
            CountInPrepare();
            state.Notify (MIDISequencerGUIEvent::GROUP_TRANSPORT,
                          MIDISequencerGUIEvent::GROUP_TRANSPORT_COUNTIN);
        }
        else
            state.Notify (MIDISequencerGUIEvent::GROUP_TRANSPORT,
                          MIDISequencerGUIEvent::GROUP_TRANSPORT_START);
        SetDevOffset((tMsecs)GetCurrentTimeMs());
        MIDITickComponent::Start();
        std::cout << "\t\t ... Exiting from MIDISequencer::Start()" << std::endl;
    }
}


void MIDISequencer::Stop() {
    if (IsPlaying()) {
        std::lock_guard<std::recursive_mutex> lock(proc_lock);
        std::cout << "\t\tEntered in MIDISequencer::Stop() ..." << std::endl;
        // waits until the timer thread has stopped
        MIDITickComponent::Stop();
        // resets the autostop flag
        state.playing_status &= ~AUTO_STOP_PENDING;
        state.iterator.SetTimeShiftMode(time_shift_mode);
        MIDIManager::AllNotesOff();
        MIDIManager::CloseOutPorts();
        state.Notify (MIDISequencerGUIEvent::GROUP_TRANSPORT,
                      MIDISequencerGUIEvent::GROUP_TRANSPORT_STOP);
        std::cout << "\t\t ... Exiting from MIDISequencer::Stop()" << std::endl;
    }
}


void MIDISequencer::StaticTickProc(tMsecs sys_time, void* pt) {
    MIDISequencer* seq_pt = static_cast<MIDISequencer *>(pt);
    seq_pt->TickProc(sys_time);
}

/* OLD VERSION (trouble with repeatde play)
void MIDISequencer::TickProc(tMsecs sys_time) {
    float next_event_time = 0.0;
    int msg_track;
    MIDITimedMessage msg;

    //std::cout << "MIDISequencer::TickProc; sys_time_offset " << sys_time_offset << " sys_time " << sys_time
    //     << " dev_time_offset " << dev_time_offset << std::endl;

    proc_lock.lock();

    //static unsigned int times;
    //times++;
    //if (!(times % 100))
        //std::cout << "MIDISequencer::TickProc() " << times << " times" << std::endl;

    // if we are in repeat mode, repeat if we hit end of the repeat region
    if(repeat_play_mode && GetCurrentMeasure() >= repeat_end_meas) {
        // yes we hit the end of our repeat block
        // shut off all notes on
        MIDIManager::AllNotesOff();
        // now move the sequencer to our start position
        GoToMeasure(repeat_start_meas);
        // our current raw system time is now the new system time offset
        sys_time_offset = sys_time;
        // sys_time = 0;

        // the sequencer time offset now must be reset to the
        // time in milliseconds of the sequence start point
         dev_time_offset = (tMsecs)GetCurrentTimeMs();
    }
    // find all events that exist before or at this time,
    // but only if we have space in the output queue to do so!
    // also limit ourselves to 100 midi events max.
    int output_count = 100;
    tMsecs cur_time = sys_time - sys_time_offset + dev_time_offset;
    while(
        (GetNextEventTimeMs(&next_event_time) || play_mode == PLAY_UNBOUNDED)
        && (next_event_time <= cur_time
        && (--output_count) > 0 ) {
        // found an event! get it!
        if(GetNextEvent(&msg_track, &msg) && !msg.IsMetaEvent())
            // tell the driver to send this message now
            MIDIManager::GetOutDriver(GetTrackOutPort(msg_track))->OutputMessage(msg);
    }
    // auto stop at end of sequence
    MIDIClockTime tmp;
    if (!(repeat_play_mode && GetCurrentMeasure() >= repeat_end_meas) &&
        !GetNextEventTime(&tmp) && (play_mode == PLAY_BOUNDED)) {
        // no events left
        std::thread(StaticStopProc, this).detach();
        std::cout << "Stopping the sequencer: StaticStopProc called" << std::endl;
    }
    proc_lock.unlock();
}
*/

// NEW VERSION
void MIDISequencer::TickProc(tMsecs sys_time) {
    float next_event_time = 0.0;
    int msg_track;
    MIDITimedMessage msg;

    std::lock_guard<std::recursive_mutex> lock(proc_lock);
    //std::cout << "MIDISequencer::TickProc; sys_time_offset " << sys_time_offset << " sys_time " << sys_time
    //     << " dev_time_offset " << dev_time_offset << std::endl;

    // check if already autostopped
    if (state.playing_status & AUTO_STOP_PENDING) {
        std::cout << "MIDISequencer::TickProc called after Auto Stop" << std::endl;
        return;
    }

    if (sys_time < sys_time_offset) {
        std::cout << "WARNING! sys_time = " << sys_time << " sys_time_offset = " << sys_time_offset << std::endl;
        std::cout << "This causes an error when starting from the beginning" << std::endl;
        sys_time_offset = sys_time;
    }

    //static unsigned int times;
    //if (!(times % 100)) {
    //    std::cout << "MIDISequencer::TickProc() " << times << " times" << std::endl;
    //    std::cout << "sys_time_offset = " << sys_time_offset << "  sys_time = " << sys_time << std::endl;
    //}
    //times++;

    // check if we we are counting in
    if (state.playing_status & COUNT_IN_PENDING) {
        MIDIClockTime clocks = (MIDIClockTime)((sys_time - sys_time_offset) / state.ms_per_clock);
        //std::cout << "clocks = " << clocks << "     count_in_time = " << state.count_in_time << std::endl;
        if (clocks >= state.count_in_time) {
            if (state.count_in_time != state.beat_length * state.number_of_beats) {
                state.Process(&beat_marker_msg);    // increments state.count_in_time
                return;
            }
            else {
                // ends count in
                state.playing_status &= ~COUNT_IN_PENDING;
                // updates the start time of the sequencer
                sys_time_offset = sys_time;
                state.Notify (MIDISequencerGUIEvent::GROUP_TRANSPORT,
                          MIDISequencerGUIEvent::GROUP_TRANSPORT_START);
            }
        }
        else
            return;
    }
    // find current time
    tMsecs cur_time = sys_time - sys_time_offset + dev_time_offset;
    // find all events that exist before or at this time,
    // limit ourselves to 100 midi events max.
    int output_count = 100;
    while(
        (GetNextEventTimeMs(&next_event_time) || play_mode == PLAY_UNBOUNDED)
        && (next_event_time <= cur_time
        && (--output_count) > 0 )) {
        // found an event! get it!
        if(GetNextEvent(&msg_track, &msg)) {
            // as the beat marker is the 1st event of a measure, we must check here if we have
            // reached the loop end
            if (msg.IsBeatMarker() && repeat_play_mode && GetCurrentMeasure() == repeat_end_meas) {
                // yes we hit the end of our repeat block: shut off all notes on
                MIDIManager::AllNotesOff();
                // now move the sequencer to our start position
                GoToMeasure(repeat_start_meas);
                // our current raw system time is now the new system time offset
                sys_time_offset = sys_time;
                // the sequencer time offset now must be reset to the
                // time in milliseconds of the sequence start point
                dev_time_offset = (tMsecs)GetCurrentTimeMs();
                break;
            }
            else if (!msg.IsMetaEvent() && !msg.IsBeatMarker())
                // otherwise tell the driver to send this message now
                MIDIManager::GetOutDriver(GetTrackOutPort(msg_track))->OutputMessage(msg);
        }
    }
    // auto stop at end of sequence
    MIDIClockTime tmp;
    if (!(repeat_play_mode && state.cur_measure >= repeat_end_meas) &&
        !GetNextEventTime(&tmp) && (play_mode == PLAY_BOUNDED)) {
        // no events left
        std::cout << "Auto stopping the sequencer: StaticStopProc called at time " << GetCurrentMIDIClockTime() << std::endl;
        //<< "GetNextEventTime() returned " << retval << std::endl;
        state.playing_status |= AUTO_STOP_PENDING;      // must be here, not in StaticStopProc
        //times = 0;      // only for log, comment if you don't need
        std::thread(StaticStopProc, this).detach();
    }
}


void MIDISequencer::ScanEventsAtThisTime() {
    // save the current iterator state
    MIDIMultiTrackIteratorState istate( state.iterator.GetState() );
    int prev_measure = state.cur_measure;
    int prev_beat = state.cur_beat;
    MIDIClockTime orig_clock = state.cur_clock;
    float orig_time_ms = state.cur_time_ms;

    // process all messages up to and including this time only
    MIDIClockTime t = 0;
    int trk;
    MIDITimedMessage msg;
    while( GetNextEventTime(&t) && t == orig_clock && GetNextEvent(&trk, &msg)) {
        ;  // cycle through all events at this time
    }

    // restore the iterator state
    state.iterator.SetState(istate);
    state.cur_measure = prev_measure;
    state.cur_beat = prev_beat;
    state.cur_clock = orig_clock;
    state.cur_time_ms = orig_time_ms;
    if (state.cur_clock == state.last_beat_time)
        state.next_beat_time = state.cur_clock;
}


void MIDISequencer::CountInPrepare() {
    if (state.playing_status & COUNT_IN_ENABLED) {
        std::cout << "Setting count in" << std::endl;
        (state.playing_status &= ~COUNT_IN_PENDING) |= COUNT_IN_PENDING;
        state.count_in_time = 0;
        beat_marker_msg.SetTime(0);
    }
}
