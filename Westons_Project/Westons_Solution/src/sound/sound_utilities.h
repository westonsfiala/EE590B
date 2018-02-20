#pragma once

#include <cmath>
#include <cassert>
#include <iostream>
#include <regex>
#include <memory>

#include "portaudio.h"

// global static variables that can be used in callback functions
static float frequency;
static float volume;

// Get the value of pi.
const static float pi = static_cast<float>(std::acos(-1));
const static float two_pi = 2.0f * pi;
const static int default_sample_rate = 44100;

/**
* \brief Takes a float input and clips it between -1.0 & 1.0. If no clipping is needed, returns the input.
* \param input Float that needs to be checked for out of bounds input ranges.
* \return Clipped version of the input value.
*/
inline float clipped_output(const float& input)
{
    return std::max(std::min(1.0f, input), 0.0f);
}

/**
 * \brief Struct used to hold information necessary for the operation of a port audio callback function.
 */
struct callback_data
{
    callback_data(const float data, const int input_channels, const int output_channels, const int rate)
    {
        user_data = data;
        num_input_channels = input_channels;
        num_output_channels = output_channels;
        sample_rate = rate;
    }
    float user_data;
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
    callback_info(PaStreamCallback* callback, std::shared_ptr<callback_data> callback_data_ptr, const std::string& callback_name, callback_processor* process_method)
    {
        m_callback = callback;
        m_callback_data_ptr = callback_data_ptr;
        m_callback_name = callback_name;
        m_process_method = process_method;
    }
    PaStreamCallback* m_callback;
    std::shared_ptr<callback_data> m_callback_data_ptr;
    std::string m_callback_name;
    callback_processor* m_process_method;
};

/**
 * \brief Callback that passes the input buffer into the output buffer. Number of input & output streams is supplied through user data.
 * \param input_buffer Buffer of values that has been captured by a device.
 * \param output_buffer Buffer of values that will be output to a physical port.
 * \param frames_per_buffer Number of frames in each buffer.
 * \param time_info The time that the input values were captured, and the time the output values will be played.
 * \param status_flags Status bits that detail the current state of the streams.
 * \param user_data Pointer to some data that the callback expects. default_callback_data.
 * \return If the passthrough worked correctly. 0 for success, !0 for failure.
 */
static int passthrough_callback(const void* input_buffer, void* output_buffer,
    const unsigned long frames_per_buffer,
    const PaStreamCallbackTimeInfo* time_info,
    PaStreamCallbackFlags status_flags,
    void* user_data)
{
    // stop warnings by casting to void.
    static_cast<void>(status_flags);
    static_cast<void>(time_info);

    // Get the data pointer.
    const auto data = static_cast<callback_data*>(user_data);

    assert(data->num_input_channels == 1);
    assert(data->num_output_channels >= 1);

    // Make sure that volume is valid.
    assert(volume >= 0.0f && volume <= 1.0f);

    // Get the parts we care about ready.
    auto* out = static_cast<float*>(output_buffer);
    auto* input = static_cast<const float*>(input_buffer);

    for (unsigned int i = 0; i < frames_per_buffer; i++)
    {
        // Loop the input data to all of the output channels.
        for(auto j = 0; j < data->num_output_channels; ++j)
        {
            out[data->num_output_channels*i + j] = clipped_output(input[i]);
        }
    }
    return 0;
}

/**
 * \brief Processor method for the passthrough mode. Waits for the user to enter anything into the terminal then quits.
 */
static void passthrough_processer()
{
    std::cout << std::endl << "Started passthrough mode. The input audio will be played back to the output." << std::endl;
    std::cout << "To exit, enter any string" << std::endl;

    // Wait for any input, then exit.
    std::string read_string;
    std::cin >> read_string;

    std::cout << "Exiting passthrough mode." << std::endl;
}

/**
* \brief Callback that generates a sine wave on the output channel.
* \param input_buffer Buffer of values that has been captured by a device.
* \param output_buffer Buffer of values that will be output to a physical port.
* \param frames_per_buffer Number of frames in each buffer.
* \param time_info The time that the input values were captured, and the time the output values will be played.
* \param status_flags Status bits that detail the current state of the streams.
* \param user_data Pointer to some data that the callback expects.
* \return If the passthrough worked correctly. 0 for success, !0 for failure.
*/
static int frequency_gen_callback(const void* input_buffer, void* output_buffer,
    const unsigned long frames_per_buffer,
    const PaStreamCallbackTimeInfo* time_info,
    PaStreamCallbackFlags status_flags,
    void* user_data)
{
    // stop warnings by casting to void.
    static_cast<void>(status_flags);
    static_cast<void>(time_info);
    static_cast<void>(input_buffer);

    // Get the data that we care about.
    const auto data = static_cast<callback_data*>(user_data);

    // Check for valid values.
    assert(data->num_input_channels == 0);
    assert(data->num_output_channels >= 1);

    auto* out = static_cast<float*>(output_buffer);

    for (unsigned int i = 0; i < frames_per_buffer; i++)
    {

        // Make sure nothing is wrong with volume.
        assert(volume >= 0.0f && volume <= 1.0f);

        // Advance the frequency in radians everytime we go through this loop.
        data->user_data += two_pi * frequency / static_cast<float>(data->sample_rate);

        // If we go over 2*pi drop back down. If you let it just count up, you start getting weird effects.
        // i.e. The frequency seems to change when nothing is modified, and it has noticable steps of frequency increase.
        if (data->user_data >= two_pi)
        {
            data->user_data -= two_pi;
        }

        // Playback to the output.
        for (auto j = 0; j < data->num_output_channels; ++j)
        {
            out[data->num_output_channels*i + j] = clipped_output(std::sin(data->user_data) * volume);
        }

    }
    return 0;
}

static void frequency_gen_processer()
{
    std::cout << std::endl << "Started Frequency Generator mode. A setable frequency sine wave will be played." << std::endl;


    const std::string set_frequency_string = "setFrequency:";
    std::cout << "To adjust frequency, enter '" << set_frequency_string << "'." << std::endl;
    frequency = 440.0f;

    const std::string set_volume_string = "setVolume:";
    std::cout << "To adjust volume, enter: '" << set_volume_string << "{0-100}'" << std::endl;
    volume = 1.0;

    std::cout << "To exit, set the frequency to 0" << std::endl;

    while (frequency != 0.0f)
    {
        // Wait for an input, then process it.
        std::string read_string;
        std::cin >> read_string;

        auto set_volume = false;
        auto set_frequency = false;

        // If we find the substring 'setVolume:' at the start of our string, we are setting the volume.
        if (read_string.find(set_volume_string) == 0)
        {
            set_volume = true;
        }
        else if(read_string.find(set_frequency_string) == 0)
        {
            set_frequency = true;
        }

        // stoi will crash if you send in bad values, filter them out. Removes all non-digits.
        read_string = std::regex_replace(read_string, std::regex("\\D"), "");

        // If the string is empty, don't do anything, just continue on.        
        if (read_string.empty())
        {
            continue;
        }

        // Get the parsed value from stoi. 
        int parsed_value;
        try
        {
            parsed_value = std::stoi(read_string);
        }
        // Catch all exceptions. If something bad slipped through the cracks, continue without changing anything.
        catch (...)
        {
            continue;
        }

        // Set the volume of the output
        if (set_volume)
        {
            volume = std::max(std::min(100.0f, static_cast<float>(parsed_value)), 0.0f) / 100.0f;

            assert(volume >= 0.0f && volume <= 100.0f);
            std::cout << "The new Volume is " << read_string << std::endl;
        }
        // Set the frequency of the generated signal.
        else if (set_frequency)
        {
            frequency = static_cast<float>(parsed_value);

            std::cout << "The new Frequency is " << read_string << std::endl;
        }
    }

    std::cout << "Exiting Frequency Generator mode." << std::endl;
}