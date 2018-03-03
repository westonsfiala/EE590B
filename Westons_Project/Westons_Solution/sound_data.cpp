#include "sound_data.h"
#include <cassert>

/**
 * \brief Adds the given note to the sound.
 * \param new_note Note added to the sound.
 */
void sound_data::add_note(const note_data& new_note)
{
    m_notes.emplace_back(new_note);
    calculate_note_volume();
}

/**
 * \brief Removes all the notes with the given frequency from the sound.
 * \param frequency Frequency of the notes to remove from the sound.
 */
void sound_data::remove_notes(const float frequency)
{
    std::vector<note_data> reap_notes;

    // Iterate over our list and reap what needs reaping.
    for(const auto& note : m_notes)
    {
        if(note.m_frequency == frequency)
        {
            reap_notes.push_back(note);
        }
    }

    for(auto reap_note : reap_notes)
    {
        m_notes.remove(reap_note);
    }

    calculate_note_volume();
}

/**
 * \brief Processes all of the contained notes in the sound. If any notes duration drops below 0, it is removed.
 * \param sample_rate Sample rate of the sound. Used to calculate time advancement.
 * \param num_samples Number of samples processed since last process call. Used to calculate time advancement.
 */
void sound_data::process(const int sample_rate, const int num_samples)
{
    std::vector<note_data> reap_notes;

    // Go through all of the notes and advance their duration
    for (auto& note : m_notes)
    {
        // Only care about positive timed notes.
        if(note.m_duration > 0.0f)
        {
            // Subtract away the milliseconds that passed.
            note.m_duration -= 1000.0f * static_cast<float>(num_samples) / static_cast<float>(sample_rate);
            if(note.m_duration < 0.0f)
            {
                reap_notes.push_back(note);
            }
        }
    }

    // Remove them after the processing block.
    for(auto reap_note : reap_notes)
    {
        m_notes.remove(reap_note);
    }

    calculate_note_volume();
}

/**
 * \brief Calculates the volume that should be applied to all notes in this sound to ensure no clipping.
 */
void sound_data::calculate_note_volume()
{
    auto max_volume = 1.0f;

    auto volume_sum = 0.0f;
    for(auto note : m_notes)
    {
        volume_sum += note.m_volume;
    }

    // If we have more than 1.0 combined volume, then lower down the note volume.
    if(volume_sum > max_volume)
    {
        max_volume = max_volume / volume_sum;
    }

    // Should never be able to have these happen.
    assert(max_volume >= 0.0 && max_volume <= 1.0);

    m_note_volume = max_volume;
}
