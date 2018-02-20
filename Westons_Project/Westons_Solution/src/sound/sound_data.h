
#pragma once
/**
 * \brief Simple struct used to store information about playing a sound.
 */
struct sound_data
{
    explicit sound_data(float frequency, float phase_offset, float volume, float duration);
    ~sound_data() = default;

    float get_frequency() const;
    float get_phase_offset() const;
    float get_volume() const;
    float get_duration() const;

private:
    float m_frequency_;
    float m_phase_offset_;
    float m_volume_;
    float m_duration_;
};
