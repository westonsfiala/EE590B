#include <iostream>
#include <cmath>
#include <cassert>
#include <regex>

#include "portaudio.h"
#include "../audio_driver.h"

static int num_input_channels;
static int num_output_channels;
static int sample_rate;

static float frequency;
static float volume;
static bool passthrough;

/**
 * \brief Takes a float input and clips it between -1.0 & 1.0. If no clipping is needed, returns the input.
 * \param input Float that needs to be checked for out of bounds input ranges. 
 * \return Clipped version of the input value.
 */
float clipped_output(const float& input)
{
    if(input > 1.0f)
    {
        return 1.0f;
    }

    if(input < -1.0f)
    {
        return -1.0f;
    }

    return input;
}

/* This routine will be called by the PortAudio engine when audio is needed.
It may called at interrupt level on some machines so don't do anything
that could mess up the system like calling malloc() or free().
*/
static int patest_callback(const void* input_buffer, void* output_buffer,
                           const unsigned long frames_per_buffer,
                           const PaStreamCallbackTimeInfo* time_info,
                           PaStreamCallbackFlags status_flags,
                           void* user_data)
{
    // stop warnings by casting to void.
    static_cast<void>(status_flags);
    static_cast<void>(time_info);

    // Get the parts we care about ready.
    const auto data = static_cast<float*>(user_data);
    auto* out = static_cast<float*>(output_buffer);
    auto* input = static_cast<const float*>(input_buffer);

    for (unsigned int i = 0; i < frames_per_buffer; i++)
    {
        assert(volume >= 0.0f && volume <= 1.0f);
        // If we are passing through, assign input values to the output channels.
        if(passthrough)
        {
            // Playback to the output.
            for (auto j = 0; j < num_output_channels; ++j)
            {
                out[num_output_channels*i + j] = clipped_output(input[i] * volume);
            }
        }
        // Not a passthrough, generate some sine waves.
        else
        {
            // Get the value of pi.
            const static auto pi = static_cast<float>(std::acos(-1));
            const static auto two_pi = 2.0f * pi;

            // Advance the frequency in radians everytime we go through this loop.
            *data += two_pi * frequency / static_cast<float>(sample_rate);

            // If we go over 2*pi drop back down. If you let it just count up, you start getting weird effects.
            // i.e. The frequency seems to change when nothing is modified, and it has noticable steps of frequency increase.
            if(*data >= two_pi)
            {
                *data -= two_pi;
            }

            // Playback to the output.
            for (auto j = 0; j < num_output_channels; ++j)
            {
                out[num_output_channels*i + j] = clipped_output(std::sin(*data) * volume);
            }
        }
        
    }
    return 0;
}

int main()
{
    std::cout << "Booting up Audio Driver" << std::endl;

    // Set up our static variables.
    num_input_channels = 1;
    num_output_channels = 1;
    sample_rate = 44100;

    // Initialize the program, and let the user know how to operate it.
    std::cout << "Starting Audio Driver in passthrough mode." << std::endl;
    passthrough = true;
    frequency = 440.0f;

    std::cout << "To switch between modes type 'passthrough' or 'generate'" << std::endl;

    std::cout << "To adjust volume, enter: 'setVolume:{0-100}'" << std::endl;
    volume = 1.0;

    std::cout << "Enter '0' to exit" << std::endl;

    auto driver = audio_driver(num_input_channels, num_output_channels, sample_rate, patest_callback);

    if(!driver.start())
    {
        std::cerr << "Failed to start Audio Driver." << std::endl << "Error: " + driver.get_error() << std::endl;
        return 1;
    }


    while(frequency != 0.0f)
    {
        std::string read_string;
        std::cin >> read_string;

        auto set_volume = false;

        // If we see the passthrough string, go to passthrough mode.
        if (read_string == "passthrough" && !passthrough)
        {
            std::cout << "Entering passthrough mode" << std::endl;
            passthrough = true;
            continue;
        }
        
        // If we see the generate string, go to frequency generation mode.
        if (read_string == "generate" && passthrough)
        {
            std::cout << "Entering frequency generation mode" << std::endl << "Please enter an integer frequency to generate (Hz)." << std::endl;
            passthrough = false;
            continue;
        }

        // If we find the substring 'setVolume:' at the start of our string, we are setting the volume.
        if (read_string.find("setVolume:") == 0)
        {
            set_volume = true;
        }

        // stoi will crash if you send in bad values, filter them out. Removes all non-digits.
        read_string = std::regex_replace(read_string, std::regex("\\D"), "");

        // If the string is empty, don't do anything, just continue on.        
        if(read_string.empty())
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
        if(set_volume)
        {
            volume = std::max(std::min(100.0f, static_cast<float>(parsed_value)), 0.0f) / 100.0f;

            assert(volume >= 0.0f && volume <= 100.0f);
            std::cout << "The new Volume is " << read_string << std::endl;
        }
        // Set the frequency of the generated signal.
        else
        {
            frequency = static_cast<float>(parsed_value);

            std::cout << "The new Frequency is " << read_string << std::endl;
        }
    }

    std::cout << "Stopping Audio Driver" << std::endl;

    if(!driver.stop())
    {
        std::cerr << "Failed to stop Audio Driver." << std::endl << "Error: " + driver.get_error() << std::endl;
        return 1;
    }

    std::cout << "Exiting Audio Driver" << std::endl;

    return 0;
}
