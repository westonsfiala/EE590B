#include "passthrough_driver.h"

static std::shared_ptr<callback_data> m_data_ptr;

static float volume;

void passthrough_driver::init(callback_data data)
{
    m_data_ptr = std::make_shared<callback_data>(data);
    volume = 1.0f;
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
        for (auto j = 0; j < data->num_output_channels; ++j)
        {
            out[data->num_output_channels*i + j] = clipped_output(input[i]);
        }
    }
    return 0;
}

/**
* \brief Processor method for the passthrough mode. Waits for the user to enter anything into the terminal then quits.
*/
void passthrough_driver::processor()
{
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
    assert(m_data_ptr);
    return m_data_ptr.get();
}
