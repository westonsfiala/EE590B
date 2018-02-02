#include "sound_data.h"
#include <cassert>
#include <algorithm>

/**
 * \brief Construct a frequency data with the given values.
 * \param frequency Frequency of the sound. If it is negative, takes the absolute value.
 * \param volume Volume that the sound should be played at. Valid range include 0 - 100. If provided value is higher or lower, clips to 100 or 0.
 * \param duration Duration of the sound in milliseconds. Must be non-negative.
 */
sound_data::sound_data(const double frequency, const double volume, const double duration)
{
    assert(frequency > 0.0);
    m_frequency_ = std::abs(frequency);

    assert(volume <= 100.0 && volume >= 0);
    m_volume_ = std::min(std::max(volume, 0.0) ,100.0);

    assert(duration >= 0.0);
    m_duration_ = std::max(duration, 0.0);
}

/**
 * \brief Gets the frequency of the sound.
 * \return Frequency of the sound.
 */
double sound_data::get_frequency() const
{
    return m_frequency_;
}

/**
 * \brief Gets the volume of the sound.
 * \return Volume of the sound.
 */
double sound_data::get_volume() const
{
    return m_volume_;
}

/**
 * \brief Gets the duration of the sound in milliseconds.
 * \return Duration of the sound in milliseconds.
 */
double sound_data::get_duration() const
{
    return m_duration_;
}
