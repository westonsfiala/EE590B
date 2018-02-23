#pragma once

#include <cmath>
#include <cassert>
#include <iostream>
#include <regex>
#include <memory>

#include "portaudio.h"
#include <complex>

// Get the value of pi.
const static float pi = static_cast<float>(std::acos(-1));
const static float two_pi = 2.0f * pi;
const static int default_sample_rate = 44100;

enum wave_type
{
    sine,
    square,
    triangle,
    sawtooth
};

inline wave_type from_string(const std::string& wave)
{
    if(wave == "sine")
    {
        return sine;
    }
    if(wave == "square")
    {
        return square;
    }
    if(wave == "triangle")
    {
        return triangle;
    }
    if(wave == "sawtooth")
    {
        return sawtooth;
    }
    throw std::runtime_error("No such type as: " + wave);
}

/**
* \brief Takes a float input and clips it between -1.0 & 1.0. If no clipping is needed, returns the input.
* \param input Float that needs to be checked for out of bounds input ranges.
* \return Clipped version of the input value.
*/
inline float clipped_output(const float& input)
{
    return std::max(std::min(1.0f, input), 0.0f);
}

/**
 * \brief Takes a float and ensures it is between 0 to two pi, by wrapping it to the correct value.
 * \param input input float to wrap to a value between 0 to two pi.
 * \return Value between 0 to two pi.
 */
inline float two_pi_wrapper(const float& input)
{
    // Copy the output.
    auto output = input;

    // Easier to deal with just positive values.
    if(output < 0)
    {
        output = -output;
    }

    if(output >= two_pi)
    {
        const int scale = output / two_pi;
        output -= scale*two_pi;
    }

    assert(output >= 0 && output < two_pi);
    return output;
}

inline int phase_to_index(const float& phase, const uint32_t& max_index)
{
    assert(phase >= 0 && phase < two_pi);
    return two_pi_wrapper(phase) / two_pi * max_index;
}

/**
 * \brief Generates a vector of floats that represents one period of a sine wave over the given number of samples.
 * \param num_samples number of samples to have in one period of the sine wave.
 * \return vector that is number of samples long with one period of a sine wave captured.
 */
inline std::vector<float> sine_lookup(const uint32_t num_samples)
{
    std::vector<float> sine_table;

    for(uint32_t i = 0; i < num_samples; ++i)
    {
        sine_table.push_back(std::sin(two_pi*i/num_samples));
    }
    return sine_table;
}

/**
* \brief Generates a vector of floats that represents one period of a square wave over the given number of samples.
* \param num_samples number of samples to have in one period of the square wave.
* \return vector that is number of samples long with one period of a square wave captured.
*/
inline std::vector<float> square_lookup(const uint32_t num_samples)
{
    std::vector<float> square_table;

    for (uint32_t i = 0; i < num_samples; ++i)
    {
        if(i < num_samples / 2)
        {
            square_table.push_back(1.0f);
        }
        else
        {
            square_table.push_back(-1.0f);
        }
    }
    return square_table;
}

/**
* \brief Generates a vector of floats that represents one period of a triangle wave over the given number of samples.
* \param num_samples number of samples to have in one period of the triangle wave.
* \return vector that is number of samples long with one period of a triangle wave captured.
*/
inline std::vector<float> triangle_lookup(const uint32_t num_samples)
{
    std::vector<float> triangle_table;

    for (uint32_t i = 0; i < num_samples; ++i)
    {
        const auto segment_length = num_samples / 4;
        // Segment 1, increasing from 0 to 1.
        if(i < segment_length)
        {
            const auto slope = 1.0f / segment_length;
            const auto constant = 0.0f;
            triangle_table.push_back(slope * i + constant);
        }
        // Segment 2, decreasing from 1 to -1.
        else if( i < segment_length * 3)
        {
            const auto slope = -1.0f / segment_length;
            const auto constant = 2.0f;
            triangle_table.push_back(slope * i + constant);
        }
        // Segment 3, increasing from -1 to 0.
        else
        {
            const auto slope = 1.0f / segment_length;
            const auto constant = -4.0f;
            triangle_table.push_back(slope * i + constant);
        }
    }
    return triangle_table;
}

/**
* \brief Generates a vector of floats that represents one period of a sawtooth wave over the given number of samples.
* \param num_samples number of samples to have in one period of the sawtooth wave.
* \return vector that is number of samples long with one period of a sawtooth wave captured.
*/
inline std::vector<float> sawtooth_lookup(const uint32_t num_samples)
{
    std::vector<float> sawtooth_table;

    for (uint32_t i = 0; i < num_samples; ++i)
    {
        const auto halfsamples = num_samples / 2.0f;
        sawtooth_table.push_back((i - halfsamples) / halfsamples);
    }
    return sawtooth_table;
}

/**
 * \brief Struct used to hold information necessary for the operation of a port audio driver.
 */
struct callback_data
{
    callback_data()
    {
        num_input_channels = 0;
        num_output_channels = 0;
        sample_rate = 0;
    }

    callback_data(const int input_channels, const int output_channels, const int rate)
    {
        num_input_channels = input_channels;
        num_output_channels = output_channels;
        sample_rate = rate;
    }
    int num_input_channels;
    int num_output_channels;
    int sample_rate;
};

typedef void callback_processor();

/**
 * \brief Struct used to contain information about a port audio callback.
 */
struct callback_info
{
    callback_info(PaStreamCallback* callback, const callback_data& call_data, void* callback_data_ptr, const std::string& callback_name, callback_processor* process_method)
    {
        m_callback = callback;
        m_callback_data = call_data;
        m_callback_data_ptr = callback_data_ptr;
        m_callback_name = callback_name;
        m_process_method = process_method;
    }
    PaStreamCallback* m_callback;
    callback_data m_callback_data;
    void* m_callback_data_ptr;
    std::string m_callback_name;
    callback_processor* m_process_method;
};