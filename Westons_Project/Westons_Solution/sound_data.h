
#pragma once
/**
 * \brief Simple struct used to store information about playing a sound.
 */
struct sound_data
{
    explicit sound_data(double frequency, double volume, double duration);
    ~sound_data() = default;

    double get_frequency() const;
    double get_volume() const;
    double get_duration() const;

private:
    double m_frequency_;
    double m_volume_;
    double m_duration_;
};
