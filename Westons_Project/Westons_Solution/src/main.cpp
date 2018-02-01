#include <iostream>

#include "portaudio.h"
#include "../audio_driver.h"

struct port_data
{
    float output_channel_0;
};

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
        out[i] = input[i]; 

        /* Generate simple sawtooth phaser that ranges between -1.0 and 1.0. */
        data[0] += 0.01f;
        /* When signal reaches top, drop back down. */
        if (data[0] >= 1.0f)
        {
            data[0] -= 2.0f;
        }
    }
    return 0;
}

int main()
{
    std::cout << "Booting up Audio Driver" << std::endl;

    const auto num_input_channels = 1;
    const auto num_output_channels = 1;
    const auto sample_rate = 44100;

    auto driver = audio_driver(num_input_channels, num_output_channels, sample_rate, patest_callback);

    if(!driver.start())
    {
        std::cerr << "Failed to start Audio Driver." << std::endl << "Error: " + driver.get_error() << std::endl;
        return 1;
    }

    std::cout << "Playing sound for 5 seconds" << std::endl;

    Pa_Sleep(5 * 1000);

    std::cout << "Stopping Audio Driver" << std::endl;

    if(!driver.stop())
    {
        std::cerr << "Failed to stop Audio Driver." << std::endl << "Error: " + driver.get_error() << std::endl;
        return 1;
    }

    std::cout << "Exiting Audio Driver" << std::endl;

    return 0;
}
