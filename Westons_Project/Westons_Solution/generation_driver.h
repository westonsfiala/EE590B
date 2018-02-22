#pragma once

#include "src/sound/sound_utilities.h"

class generation_driver
{
public:
    static void init(const callback_data& data);

    static int callback(const void* input_buffer, void* output_buffer,
        unsigned long frames_per_buffer,
        const PaStreamCallbackTimeInfo* time_info,
        PaStreamCallbackFlags status_flags,
        void* user_data);

    static void processor();

    static void* get_data();

    struct generation_data
    {
        generation_data(const int& num_samples, const callback_data& call_data)
        {
            sine = sine_lookup(num_samples);
            square = square_lookup(num_samples);
            triangle = triangle_lookup(num_samples);
            sawtooth = sawtooth_lookup(num_samples);
            phase = 0.0f;
            data = call_data;
        }
        std::vector<float> sine;
        std::vector<float> square;
        std::vector<float> triangle;
        std::vector<float> sawtooth;
        float phase;
        callback_data data;
    };
};

