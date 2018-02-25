#pragma once

#include "src/sound/sound_utilities.h"
#include "src/sound/note_data.h"
#include <memory>
#include <map>
#include "src/rtmidi/RtMidi.h"
#include "sound_data.h"

class midi_driver
{
public:
    static bool init(sound_utilities::callback_data& data);

    static int callback(const void* input_buffer, void* output_buffer,
        unsigned long frames_per_buffer,
        const PaStreamCallbackTimeInfo* time_info,
        PaStreamCallbackFlags status_flags,
        void* user_data);

    static void processor();

    static void* get_data();

private:
    static bool initializied_;

    static sound_utilities::callback_data data_;

    // Map that keeps track of the different notes that are currently being played.
    static std::map<int, std::shared_ptr<note_data>> note_map_;

    // Midi reader for processing inputs;
    static std::shared_ptr<RtMidiIn> midi_reader_;

    // The volume and sound that will be processed by the callback.
    static float volume_;
    static sound_data sound_;
};

