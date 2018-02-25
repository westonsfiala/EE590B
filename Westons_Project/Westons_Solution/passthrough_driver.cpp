#include "passthrough_driver.h"

#include <memory>
#include <cassert>
#include <iostream>

bool passthrough_driver::initializied_ = false;

sound_utilities::callback_data passthrough_driver::data_ = sound_utilities::callback_data();

float passthrough_driver::volume_ = 0.0f;

bool passthrough_driver::init(sound_utilities::callback_data& data)
{
    Pa_Initialize();

    auto failed = false;

    // Get the default device and see if we can play with the given data.
    const auto output_device_index = Pa_GetDefaultOutputDevice();
    const auto output_device = Pa_GetDeviceInfo(output_device_index);

    if(!output_device || output_device->maxOutputChannels == 0)
    {
        std::cout << "No output channels are available on the default playback device." << std::endl;
        failed = true;
    }

    // Get the default input device and see if we can capture with the given data.
    const auto input_device_index = Pa_GetDefaultInputDevice();
    const auto input_device = Pa_GetDeviceInfo(input_device_index);

    if(!input_device || input_device->maxInputChannels == 0)
    {
        std::cout << "No input channels are available on the default capture device." << std::endl;
        failed = true;
    }

    Pa_Terminate();

    if(failed)
    {
        return false;
    }

    // Setup the values to what we allow them to be.
    data.num_input_channels = 1;
    data.num_output_channels = 1;
    data.sample_rate = sound_utilities::default_sample_rate;

    // Looks good. Lets get our stuff setup.
    data_ = data;
    volume_ = 0.5f;

    initializied_ = true;
    return initializied_;
}

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
int passthrough_driver::callback(const void* input_buffer, void* output_buffer,
    const unsigned long frames_per_buffer,
    const PaStreamCallbackTimeInfo* time_info,
    PaStreamCallbackFlags status_flags,
    void* user_data)
{
    // stop warnings by casting to void.
    static_cast<void>(status_flags);
    static_cast<void>(time_info);

    // Make sure it isn't null.
    assert(user_data);

    // Get the data pointer.
    const auto data = static_cast<sound_utilities::callback_data*>(user_data);

    assert(data->num_input_channels == 1);
    assert(data->num_output_channels >= 1);

    // Make sure that volume_ is valid.
    assert(volume_ >= 0.0f && volume_ <= 1.0f);

    // Get the parts we care about ready.
    auto* out = static_cast<float*>(output_buffer);
    const auto* input = static_cast<const float*>(input_buffer);

    uint64_t tracker = 0;
    for (unsigned int i = 0; i < frames_per_buffer; i++)
    {
        const auto output_val = sound_utilities::clipped_output(input[i] * volume_);
        // Loop the input data to all of the output channels.
        for (auto j = 0; j < data->num_output_channels; ++j)
        {
            ++tracker;
            out[data->num_output_channels*i + j] = output_val;
        }
    }

    // Just a saftey to moke sure that we actually did fill up the channels.
    assert(tracker == frames_per_buffer * data->num_output_channels);
    return 0;
}

/**
* \brief Processor method for the passthrough mode. Waits for the user to enter anything into the terminal then quits.
*/
void passthrough_driver::processor()
{
    // If we were never initialized, quit.
    if (!initializied_)
    {
        std::cout << "Passthrough Driver was not initialized. Quitting driver." << std::endl;
        return;
    }

    std::cout << std::endl << "Started passthrough mode. The input audio will be played back to the output." << std::endl;
    std::cout << "To exit, enter any string" << std::endl;

    // Wait for any input, then exit.
    std::string read_string;
    std::cin >> read_string;

    std::cout << "Exiting passthrough mode." << std::endl;

    initializied_ = false;
}

/**
 * \brief Gets the data pointer that needs to be passed to the callback function.
 * \return Data pointer for the callback function.
 */
void* passthrough_driver::get_data()
{
    return &data_;
}
