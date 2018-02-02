#include <iostream>
#include <cmath>

#include "portaudio.h"
#include "../audio_driver.h"
#include <vector>

static int num_input_channels;
static int num_output_channels;
static int sample_rate;

const static std::vector<float> sound_vector = {440.0, 500.0, 440.0, 220.0};
const static float milliseconds_per_beat = 1000.0;

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
        // Capture all of the input samples.
        //input[i];

        // Get the value of pi.
        const static float pi = std::acos(-1);

        // What frequency do we want?
        static auto current_sound = 0;
        static auto frequency = sound_vector[current_sound];

        static float num_milliseconds = 0;

        num_milliseconds += 1000.0 / sample_rate;

        if (num_milliseconds >= milliseconds_per_beat)
        {
            num_milliseconds = 0.0;
            current_sound++;

            if(current_sound >= static_cast<int>(sound_vector.size()))
            {
                current_sound = 0;
            }

            frequency = sound_vector[current_sound];
        }

        // advance the frequency everytime we go through this loop.
        data[0] += 2 * pi * frequency/ sample_rate;

        // Playback to the output.
        for(auto j = 0; j < num_output_channels; ++j)
        {
            out[num_output_channels*i+j] = std::sin(data[0]);
        }
    }
    return 0;
}

int main()
{
    std::cout << "Booting up Audio Driver" << std::endl;

    num_input_channels = 1;
    num_output_channels = 1;
    sample_rate = 44100;

    auto driver = audio_driver(num_input_channels, num_output_channels, sample_rate, patest_callback);

    if(!driver.start())
    {
        std::cerr << "Failed to start Audio Driver." << std::endl << "Error: " + driver.get_error() << std::endl;
        return 1;
    }

    std::cout << "Playing sound for 5 seconds" << std::endl;

    Pa_Sleep(10 * 1000);

    std::cout << "Stopping Audio Driver" << std::endl;

    if(!driver.stop())
    {
        std::cerr << "Failed to stop Audio Driver." << std::endl << "Error: " + driver.get_error() << std::endl;
        return 1;
    }

    std::cout << "Exiting Audio Driver" << std::endl;

    return 0;
}
