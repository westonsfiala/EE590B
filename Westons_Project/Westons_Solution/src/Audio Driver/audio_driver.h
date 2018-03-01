#pragma once
#include <portaudio.h>
#include <string>
#include <memory>
#include "../sound/sound_utilities.h"

/**
 * \brief Class used to interact with the Port Audio library.
 */
class audio_driver
{
public:
    explicit audio_driver(sound_utilities::callback_info info);
    ~audio_driver() = default;

    bool start();

    bool stop();

    std::string get_error() const;

    static bool check_channels(int32_t required_input, int32_t required_output);

private:

    bool error_detected(const PaError& error);

    bool m_running_;

    uint32_t m_input_channels_;
    uint32_t m_output_channels_;
    uint32_t m_sample_rate_;

    PaStreamCallback* m_stream_callback_;

    PaStreamParameters* m_input_params_;
    PaStreamParameters* m_output_params_;

    PaStream* m_stream_;

    void* m_data_;

    std::string m_error_string_;
};
