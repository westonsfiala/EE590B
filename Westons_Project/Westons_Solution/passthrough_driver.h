#pragma once

#include "src/sound/sound_utilities.h"

class passthrough_driver
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
    static sound_utilities::callback_data data_;
};
