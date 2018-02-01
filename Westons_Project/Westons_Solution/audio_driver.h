#pragma once
#include <portaudio.h>
#include <string>
#include <memory>

class audio_driver
{
public:
    audio_driver(uint32_t input_channels, uint32_t output_channels, uint32_t sample_rate,
                PaStreamCallback* stream_callback);
    ~audio_driver() = default;

    bool start();

    bool stop();

    std::string get_error() const;

private:

    bool error_detected(const PaError& error);

    bool m_running_;

    uint32_t m_input_channels_;
    uint32_t m_output_channels_;
    uint32_t m_sample_rate_;

    PaStreamCallback* m_stream_callback_;

    PaStream* m_stream_;

    std::shared_ptr<float*> m_data_;

    std::string m_error_string_;
};
