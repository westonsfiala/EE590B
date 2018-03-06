#pragma once

#include "src/sound/sound_utilities.h"
#include "src/sound/note_data.h"

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
    static note_data calculate_note(uint8_t note, uint8_t volume);

    static sound_utilities::callback_data data_;
};
