#include "note_data.h"
#include <cassert>
#include "sound_utilities.h"

/**
 * \brief Construct a frequency data with the given values.
 * \param frequency Frequency of the sound. If it is negative, takes the absolute value.
 * \param phase_offset Phase offset of the sound, must be between 0 <-> two_pi
 * \param duration Duration of the sound in milliseconds. Negative valued duration means infinite play time.
 * \param wave Type of wave to be used by this sound
 */
note_data::note_data(const float frequency, const float phase_offset, const float duration, const wave_type wave)
{
    assert(frequency > 0.0f);
    m_frequency = frequency;

    assert(phase_offset <= two_pi && phase_offset >= 0.0f);
    m_phase_offset = phase_offset;

    m_duration = duration;

    assert(wave == wave_type::sine || wave == wave_type::square || wave == wave_type::sawtooth || wave == wave_type::triangle);
    m_wave = wave;

    m_current_phase = m_phase_offset;
}

bool note_data::operator==(const note_data& other) const
{
    if(m_frequency != other.m_frequency)
    {
        return false;
    }

    if(m_phase_offset != other.m_phase_offset)
    {
        return false;
    }

    if(m_duration != other.m_duration)
    {
        return false;
    }

    if(m_wave != other.m_wave)
    {
        return false;
    }

    if(m_current_phase != other.m_current_phase)
    {
        return false;
    }

    return true;
}

