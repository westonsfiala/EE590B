#pragma once

#include "sound_utilities.h"

/**
 * \brief Simple struct used to store information about playing a sound.
 */
struct note_data
{
    explicit note_data(float frequency, float phase_offset, float duration, float volume,
                       sound_utilities::wave_type wave);
    note_data(const note_data& other) = default;

    bool operator==(const note_data& other) const;

    float m_frequency;
    float m_phase_offset;
    float m_duration;
    float m_volume;
    float m_current_phase;
    sound_utilities::wave_type m_wave;
};
