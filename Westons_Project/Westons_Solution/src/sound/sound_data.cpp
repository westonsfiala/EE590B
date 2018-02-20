#include "sound_data.h"
#include <cassert>
#include "sound_utilities.h"

/**
 * \brief Construct a frequency data with the given values.
 * \param frequency Frequency of the sound. If it is negative, takes the absolute value.
 * \param volume Volume that the sound should be played at. Valid range include 0 - 100. If provided value is higher or lower, clips to 100 or 0.
 * \param duration Duration of the sound in milliseconds. Must be non-negative.
 */
sound_data::sound_data(const float frequency, const float phase_offset, const float volume, const float duration)
{
    assert(frequency > 0.0f);
    m_frequency_ = frequency;

    assert(volume <= 100.0f && volume >= 0.0f);
    m_volume_ = volume;

    assert(phase_offset <= two_pi && phase_offset >= 0.0f);
    m_phase_offset_ = phase_offset;

    assert(duration >= 0.0f);
    m_duration_ = duration;
}

/**
 * \brief Gets the frequency of the sound.
 * \return Frequency of the sound.
 */
float sound_data::get_frequency() const
{
    return m_frequency_;
}

/**
 * \brief Gets the phase offset of the sound.
 * \return Phase offset of the sound.
 */
float sound_data::get_phase_offset() const
{
    return m_phase_offset_;
}

/**
 * \brief Gets the volume of the sound.
 * \return Volume of the sound.
 */
float sound_data::get_volume() const
{
    return m_volume_;
}

/**
 * \brief Gets the duration of the sound in milliseconds.
 * \return Duration of the sound in milliseconds.
 */
float sound_data::get_duration() const
{
    return m_duration_;
}
