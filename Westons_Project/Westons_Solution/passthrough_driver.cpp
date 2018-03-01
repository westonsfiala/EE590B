#include "passthrough_driver.h"

#include <memory>
#include <cassert>
#include <iostream>
#include <chrono>
#include "src/Audio Driver/audio_driver.h"

bool passthrough_driver::initializied_ = false;

sound_utilities::callback_data passthrough_driver::data_ = sound_utilities::callback_data();

/**
 * \brief Checks if the driver can be run at this time, and fills out the callback data.
 * \param data Callback data reference to fill.
 * \return If the driver can be run.
 */
bool passthrough_driver::init(sound_utilities::callback_data& data)
{
    if(!audio_driver::check_channels(1, 1))
    {
        return false;
    }

    // Setup the values to what we allow them to be.
    data.num_input_channels = 1;
    data.num_output_channels = 1;
    data.sample_rate = sound_utilities::default_sample_rate;

    // Looks good. Lets get our stuff setup.
    data_ = data;

    initializied_ = true;
    return true;
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
    assert(data->num_output_channels == 1);
    assert(initializied_);

    // Do some checks for time.
    const auto alloted_time = time_info->outputBufferDacTime - time_info->currentTime;
    const auto start_time = std::chrono::system_clock::now();

    // Get the parts we care about ready.
    auto* out = static_cast<float*>(output_buffer);
    const auto* input = static_cast<const float*>(input_buffer);

    uint64_t tracker = 0;
    for (unsigned int i = 0; i < frames_per_buffer; i++)
    {
        ++tracker;
        out[i] = input[i];
    }

    // Just a saftey to moke sure that we actually did fill up the channels.
    assert(tracker == frames_per_buffer * data->num_output_channels);

    // See that we fulfiled the time requirements.
    std::chrono::duration<double> elapsed_time = std::chrono::system_clock::now() - start_time;
    const auto elapsed_seconds = elapsed_time.count();
    assert(elapsed_seconds < alloted_time);

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
}

/**
 * \brief Gets the data pointer that needs to be passed to the callback function.
 * \return Data pointer for the callback function.
 */
void* passthrough_driver::get_data()
{
    return &data_;
}
