#include "generation_driver.h"

// Our data pointer.
static std::shared_ptr<generation_driver::generation_data> m_data_ptr;

// The precision that the built in raspberry pi audio output is capable of.
static int lookup_table_size(1 << 11);

// values used in the callback.
static float frequency;
static float volume;

void generation_driver::init(const callback_data& data)
{
    m_data_ptr = std::make_shared<generation_data>(lookup_table_size, data);
    frequency = 440.0f;
    volume = 0.25f;
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
int generation_driver::callback(const void* input_buffer, void* output_buffer,
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
    const auto data = static_cast<generation_data*>(user_data);

    // Check for valid values.
    assert(data->data.num_input_channels == 0);
    assert(data->data.num_output_channels >= 1);

    auto* out = static_cast<float*>(output_buffer);

    for (unsigned int i = 0; i < frames_per_buffer; i++)
    {
        // Make sure nothing is wrong with volume.
        assert(volume >= 0.0f && volume <= 1.0f);

        // Advance the frequency in radians everytime we go through this loop.
        data->phase = two_pi_wrapper(data->phase + two_pi * frequency / static_cast<float>(data->data.sample_rate));

        // Playback to the output.
        for (auto j = 0; j < data->data.num_output_channels; ++j)
        {
            const auto lookup_data = data->sine[phase_to_index(data->phase, lookup_table_size)];
            out[data->data.num_output_channels*i + j] = clipped_output(lookup_data * volume);
        }

    }
    return 0;
}

void generation_driver::processor()
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
        else if (read_string.find(set_frequency_string) == 0)
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

/**
* \brief Gets the data pointer that needs to be passed to the callback function.
* \return Data pointer for the callback function.
*/
void* generation_driver::get_data()
{
    assert(m_data_ptr);
    return m_data_ptr.get();
}
