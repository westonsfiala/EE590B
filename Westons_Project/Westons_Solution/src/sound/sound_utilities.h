#pragma once

#include "portaudio.h"

#include <string>
#include <vector>

class sound_utilities
{
public:
    // Values that are nice to have.
    const static float pi; 
    const static float two_pi;

    // Values that we will use by default.
    const static uint32_t default_sample_rate;
    const static uint32_t table_size;

    const static float non_clip_volume;

    enum wave_type
    {
        sine,
        square,
        triangle,
        sawtooth
    };

    static wave_type from_string(const std::string& wave);

    static float clipped_output(const float& input);

    static float two_pi_wrapper(const float& input);

    static int phase_to_index(const float& phase, const uint32_t& max_index);

    static std::vector<float> sine_lookup(uint32_t num_samples);

    static std::vector<float> square_lookup(uint32_t num_samples);

    static std::vector<float> triangle_lookup(uint32_t num_samples);

    static std::vector<float> sawtooth_lookup(uint32_t num_samples);

    struct wave_tables
    {
        wave_tables()
        {
            samples_per_table = table_size;
            sine = sine_lookup(samples_per_table);
            square = square_lookup(samples_per_table);
            sawtooth = sawtooth_lookup(samples_per_table);
            triangle = triangle_lookup(samples_per_table);
        }

        uint32_t samples_per_table;
        std::vector<float> sine;
        std::vector<float> square;
        std::vector<float> sawtooth;
        std::vector<float> triangle;
    };

    const static wave_tables wave_lookup_tables;

    /**
    * \brief Struct used to hold information necessary for the operation of a port audio driver.
    */
    struct callback_data
    {
        callback_data()
        {
            num_input_channels = 0;
            num_output_channels = 0;
            sample_rate = 0;
        }

        callback_data(const int input_channels, const int output_channels, const int rate)
        {
            num_input_channels = input_channels;
            num_output_channels = output_channels;
            sample_rate = rate;
        }
        int num_input_channels;
        int num_output_channels;
        int sample_rate;
    };

    typedef void callback_processor();

    /**
    * \brief Struct used to contain information about a port audio callback.
    */
    struct callback_info
    {
        callback_info(PaStreamCallback* callback, const callback_data& call_data, void* callback_data_ptr, const std::string& callback_name, callback_processor* process_method)
        {
            m_callback = callback;
            m_callback_data = call_data;
            m_callback_data_ptr = callback_data_ptr;
            m_callback_name = callback_name;
            m_process_method = process_method;
        }
        PaStreamCallback* m_callback;
        callback_data m_callback_data;
        void* m_callback_data_ptr;
        std::string m_callback_name;
        callback_processor* m_process_method;
    };
};