#include "audio_driver.h"
#include <cassert>
#include <iostream>

/**
 * \brief Constructor for an audio driver that can have some number of input and output channels.
 */
audio_driver::audio_driver(const sound_utilities::callback_info info):
    m_running_(false), 
    m_input_params_(nullptr), 
    m_output_params_(nullptr),
    m_stream_(nullptr)
{
    // Make a shared pointer
    assert(info.m_callback_data_ptr != nullptr);
    m_data_ = info.m_callback_data_ptr;

    // One of these two must be non 0.
    assert(info.m_callback_data.num_input_channels >= 0);
    m_input_channels_ = info.m_callback_data.num_input_channels;

    assert(info.m_callback_data.num_output_channels >= 0);
    m_output_channels_ = info.m_callback_data.num_output_channels;

    assert(m_input_channels_ + m_output_channels_ != 0);

    // Need to have non-0 sample rate.
    assert(info.m_callback_data.sample_rate != 0);
    m_sample_rate_ = info.m_callback_data.sample_rate;

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
    if (m_running_)
    {
        return true;
    }

    // Initialize port audio to be ready to start streaming.
    if (error_detected(Pa_Initialize()))
    {
        return false;
    }
    
    // We should not have any parameters at this point.
    assert(!m_input_params_);
    if(m_input_params_)
    {
        delete m_input_params_;
        m_input_params_ = nullptr;
    }

    // When we have an input channel, set up the parameters.
    if (m_input_channels_ > 0)
    {
        // Save the pointer so that we can keep track of it till the driver goes away.
        m_input_params_ = new PaStreamParameters;

        // Use the default device. This is the system default set in the OS.
        const auto default_device_index = Pa_GetDefaultInputDevice();
        const auto default_device_info = Pa_GetDeviceInfo(default_device_index);

        // This should never be negative.
        assert(default_device_info->maxInputChannels > 0);
        m_input_channels_ = std::min(static_cast<uint32_t>(default_device_info->maxInputChannels),
                                        m_input_channels_);
        // Sanity check again.
        assert(m_input_channels_ > 0);

        // Set the params to the values that we expect.
        m_input_params_->device = default_device_index;
        m_input_params_->channelCount = m_input_channels_;
        m_input_params_->sampleFormat = paFloat32;
        m_input_params_->suggestedLatency = default_device_info->defaultHighInputLatency;
        m_input_params_->hostApiSpecificStreamInfo = nullptr;
    }
    // No input channels, input params need to be nullptr.
    else
    {
        m_input_params_ = nullptr;
    }

    // We should not have any parameters at this point.
    assert(!m_output_params_);
    if (m_output_params_)
    {
        delete m_output_params_;
        m_output_params_ = nullptr;
    }

    // When we have an output channel, set up the parameters.
    if (m_output_channels_ > 0)
    {
        // Save the pointer so that we can keep track of it till the driver goes away.
        m_output_params_ = new PaStreamParameters;

        // Use the default device. This is the aux jack.
        const auto default_device_index = Pa_GetDefaultOutputDevice();
        const auto default_device_info = Pa_GetDeviceInfo(default_device_index);

        // This should never be negative.
        assert(default_device_info->maxOutputChannels > 0);
        m_output_channels_ = std::min(static_cast<uint32_t>(default_device_info->maxOutputChannels),
                                        m_output_channels_);
        // Sanity check again.
        assert(m_output_channels_ > 0);

        // Set the params to the values that we expect.
        m_output_params_->device = default_device_index;
        m_output_params_->channelCount = m_output_channels_;
        m_output_params_->sampleFormat = paFloat32;
        m_output_params_->suggestedLatency = default_device_info->defaultHighOutputLatency;
        m_output_params_->hostApiSpecificStreamInfo = nullptr;
    }
        // No output channels, output params need to be nullptr.
    else
    {
        m_output_params_ = nullptr;
    }


    // Open the stream to the default hardware devices.
    const auto err = Pa_OpenStream(&m_stream_, m_input_params_, m_output_params_,
                                    m_sample_rate_, paFramesPerBufferUnspecified, paNoFlag,
                                    m_stream_callback_, m_data_);

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

    return true;
}

/**
 * \brief Stops the stream that has been started previously. If no stream has been started, does nothing.
 * \return If the stream was successfully stopped. If false is returned, get the error from get_error().
 */
bool audio_driver::stop()
{
    // Get rid of the input parameters.
    if (m_input_params_)
    {
        delete m_input_params_;
        m_input_params_ = nullptr;
    }

    // Get rid of the output parameters.
    if (m_output_params_)
    {
        delete m_output_params_;
        m_output_params_ = nullptr;
    }

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
 * \brief Checks if the default input and output devices have the required number of channels to work.
 * \param required_input Number of input channels that are required.
 * \param required_output Number of output channels that are required.
 * \return If the caller has enough channels.
 */
bool audio_driver::check_channels(const int32_t required_input, const int32_t required_output)
{
    // If we don't need to drive anything, don't waste time booting up.
    if (required_input <= 0 && required_output <= 0)
    {
        return true;
    }

    if(Pa_Initialize() != paNoError)
    {
        return false;
    }

    auto passed = true;

    if(required_input > 0)
    {
        // Get the default input device and see if we can capture with the given data.
        const auto input_device_index = Pa_GetDefaultInputDevice();
        const auto input_device = Pa_GetDeviceInfo(input_device_index);

        if (!input_device || input_device->maxInputChannels < required_input)
        {
            std::cout << "No input channels are available on the default capture device." << std::endl;
            passed = false;
        }
    }

    if(required_output > 0)
    {
        // Get the default device and see if we can play with the given data.
        const auto output_device_index = Pa_GetDefaultOutputDevice();
        const auto output_device = Pa_GetDeviceInfo(output_device_index);

        if (!output_device || output_device->maxOutputChannels < 0)
        {
            std::cout << "No output channels are available on the default playback device." << std::endl;
            passed = false;
        }
    }

    Pa_Terminate();

    return passed;
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
