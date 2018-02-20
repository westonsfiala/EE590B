#include "audio_driver.h"
#include <cassert>

/**
 * \brief Constructor for an audio driver that can have some number of input and output channels.
 */
audio_driver::audio_driver(callback_info info):
    m_running_(false),
    m_stream_(nullptr)
{
    // Make a shared pointer
    assert(info.m_callback_data_ptr != nullptr);
    m_data_ = info.m_callback_data_ptr;

    // One of these two must be non 0.
    assert(info.m_callback_data_ptr->num_input_channels >= 0);
    m_input_channels_ = info.m_callback_data_ptr->num_input_channels;

    assert(info.m_callback_data_ptr->num_output_channels >= 0);
    m_output_channels_ = info.m_callback_data_ptr->num_output_channels;

    assert(m_input_channels_ + m_output_channels_ != 0);

    // Need to have non-0 sample rate.
    assert(info.m_callback_data_ptr->sample_rate != 0);
    m_sample_rate_ = info.m_callback_data_ptr->sample_rate;

    // Need to have an actual callback.
    assert(info.m_callback != nullptr);
    m_stream_callback_ = info.m_callback;
}

/**
 * \brief Starts up the stream with the parameters that were supplied at construction. If a stream is already running, does nothing.
 * \return If the stream started successfully. If false in returned, get the error from get_error().
 */
bool audio_driver::start()
{
    // Don't do anything if we are already running.
    if (!m_running_)
    {
        // Initialize port audio to be ready to start streaming.
        if (error_detected(Pa_Initialize()))
        {
            return false;
        }

        // Open the stream to the default hardware devices.
        const auto err = Pa_OpenDefaultStream(&m_stream_, m_input_channels_, m_output_channels_, paFloat32,
                                              m_sample_rate_, paFramesPerBufferUnspecified, m_stream_callback_,
                                              m_data_.get());

        if (error_detected(err))
        {
            return false;
        }

        // Start the stream up.
        if (error_detected(Pa_StartStream(m_stream_)))
        {
            return false;
        }

        // We are up and running!
        m_running_ = true;
    }

    return true;
}

/**
 * \brief Stops the stream that has been started previously. If no stream has been started, does nothing.
 * \return If the stream was successfully stopped. If false is returned, get the error from get_error().
 */
bool audio_driver::stop()
{
    // If we are running, stop it.
    if (m_running_)
    {
        // Stops the stream, no more calls to the callback function will be made.
        if (error_detected(Pa_StopStream(m_stream_)))
        {
            return false;
        }

        // Closes the stream down and deletes it.
        if (error_detected(Pa_CloseStream(m_stream_)))
        {
            return false;
        }

        // Close down port audio.
        if (error_detected(Pa_Terminate()))
        {
            return false;
        }

        // We are no longer running.
        m_running_ = false;
    }

    return true;
}

/**
 * \brief Gets the error that was last reported on a failed start or stop.
 * \return Error that was last reported.
 */
std::string audio_driver::get_error() const
{
    return m_error_string_;
}

/**
 * \brief Checks if an error has been detected. If an error is detected stores the error message.
 * \param error Error code returned from a call to some Port Audio method.
 * \return If an error has been detected.
 */
bool audio_driver::error_detected(const PaError& error)
{
    // No error, no problem.
    if (error == paNoError)
    {
        m_error_string_ = "";
        return false;
    }

    m_error_string_ = Pa_GetErrorText(error);

    return true;
}
