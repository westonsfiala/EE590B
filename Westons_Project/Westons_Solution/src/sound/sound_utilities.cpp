#include "sound_utilities.h"

#include <cassert>
#include <cmath>
#include <stdexcept>

const float sound_utilities::pi = static_cast<float>(std::acos(-1));
const float sound_utilities::two_pi = 2.0f * pi;

const uint32_t sound_utilities::default_sample_rate = 44100;
const uint32_t sound_utilities::table_size = 1 << 12;

const sound_utilities::wave_tables sound_utilities::wave_lookup_tables = wave_tables();

sound_utilities::wave_type sound_utilities::from_string(const std::string& wave)
{
    if (wave == "sine")
    {
        return sine;
    }
    if (wave == "square")
    {
        return square;
    }
    if (wave == "triangle")
    {
        return triangle;
    }
    if (wave == "sawtooth")
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
float sound_utilities::clipped_output(const float& input)
{
    return std::max(std::min(1.0f, input), 0.0f);
}

/**
* \brief Takes a float and ensures it is between 0 to two pi, by wrapping it to the correct value.
* \param input input float to wrap to a value between 0 to two pi.
* \return Value between 0 to two pi.
*/
float sound_utilities::two_pi_wrapper(const float& input)
{
    // Copy the output.
    auto output = input;

    // Easier to deal with just positive values.
    if (output < 0)
    {
        output = -output;
    }

    // If we are above two pi, find out how many times over two pi we are. Then subtract away that many instances of two pi.
    if (output >= two_pi)
    {
        const auto scale = static_cast<uint32_t>(std::floor(output / two_pi));
        output -= static_cast<float>(scale) * two_pi;
    }

    assert(output >= 0 && output < two_pi);
    return output;
}

int sound_utilities::phase_to_index(const float& phase, const uint32_t& max_index)
{
    assert(phase >= 0 && phase < two_pi);
    return static_cast<int>(two_pi_wrapper(phase) / two_pi) * max_index;
}

/**
* \brief Generates a vector of floats that represents one period of a sine wave over the given number of samples.
* \param num_samples number of samples to have in one period of the sine wave.
* \return vector that is number of samples long with one period of a sine wave captured.
*/
std::vector<float> sound_utilities::sine_lookup(const uint32_t num_samples)
{
    std::vector<float> sine_table;

    for (uint32_t i = 0; i < num_samples; ++i)
    {
        const auto data = std::sin(two_pi * static_cast<float>(i) / static_cast<float>(num_samples));
        sine_table.push_back(data);
    }
    return sine_table;
}

/**
* \brief Generates a vector of floats that represents one period of a square wave over the given number of samples.
* \param num_samples number of samples to have in one period of the square wave.
* \return vector that is number of samples long with one period of a square wave captured.
*/
std::vector<float> sound_utilities::square_lookup(const uint32_t num_samples)
{
    std::vector<float> square_table;

    for (uint32_t i = 0; i < num_samples; ++i)
    {
        if (i < num_samples / 2)
        {
            const auto data = 1.0f;
            square_table.push_back(data);
        }
        else
        {
            const auto data = -1.0f;
            square_table.push_back(data);
        }
    }
    return square_table;
}

/**
* \brief Generates a vector of floats that represents one period of a triangle wave over the given number of samples.
* \param num_samples number of samples to have in one period of the triangle wave.
* \return vector that is number of samples long with one period of a triangle wave captured.
*/
std::vector<float> sound_utilities::triangle_lookup(const uint32_t num_samples)
{
    std::vector<float> triangle_table;

    for (uint32_t i = 0; i < num_samples; ++i)
    {
        const auto segment_length_float = static_cast<float>(num_samples) / 4.0f;
        const auto slope = 1.0f / segment_length_float;
        const auto segment_length = static_cast<uint32_t>(segment_length_float);
        // Segment 1, increasing from 0 to 1.
        if (i < segment_length)
        {
            const auto data = slope * static_cast<float>(i);
            triangle_table.push_back(data);
        }
            // Segment 2, decreasing from 1 to -1.
        else if (i < segment_length * 3)
        {
            const auto data = -slope * static_cast<float>(i) + 2.0f;
            triangle_table.push_back(data);
        }
            // Segment 3, increasing from -1 to 0.
        else
        {
            const auto data = slope * static_cast<float>(i) + -4.0f;
            triangle_table.push_back(data);
        }
    }
    return triangle_table;
}

/**
* \brief Generates a vector of floats that represents one period of a sawtooth wave over the given number of samples.
* \param num_samples number of samples to have in one period of the sawtooth wave.
* \return vector that is number of samples long with one period of a sawtooth wave captured.
*/
std::vector<float> sound_utilities::sawtooth_lookup(const uint32_t num_samples)
{
    std::vector<float> sawtooth_table;

    for (uint32_t i = 0; i < num_samples; ++i)
    {
        const auto halfsamples = static_cast<float>(num_samples) / 2.0f;
        const auto data = (static_cast<float>(i) - halfsamples) / halfsamples;
        sawtooth_table.push_back(data);
    }
    return sawtooth_table;
}
