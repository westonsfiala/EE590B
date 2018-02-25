#include "sound_data.h"
#include <cassert>

/**
 * \brief Adds the given note to the sound.
 * \param new_note Note added to the sound.
 */
void sound_data::add_note(std::shared_ptr<note_data> new_note)
{
    assert(new_note);
    m_notes.emplace_back(new_note);
}

/**
 * \brief Removes the given note from the sound.
 * \param remove_note Note to remove from the sound.
 */
void sound_data::remove_note(const std::shared_ptr<note_data>& remove_note)
{
    assert(remove_note);
    m_notes.remove(remove_note);
}

/**
 * \brief Gets the note volume_ to apply to all notes. 
 * \return Note volume_ to apply to all played notes. Value is between 0.0 <-> 1.0.
 */
float sound_data::get_note_volume() const
{
    auto max_volume = 1.0f;

    // If we have more than 1 note, diminish the volume_ of all notes.
    if(!m_notes.empty())
    {
        // Don't want to divide by 0.
        assert(m_notes.size());
        max_volume = max_volume / static_cast<float>(m_notes.size());
    }

    // Should never be able to have these happen.
    assert(max_volume >= 0.0 && max_volume <= 1.0);

    return max_volume;
}

/**
 * \brief Processes all of the contained notes in the sound. If any notes duration drops below 0, it is removed.
 * \param sample_rate Sample rate of the sound. Used to calculate time advancement.
 * \param num_samples Number of samples processed since last process call. Used to calculate time advancement.
 */
void sound_data::process(const int sample_rate, const int num_samples)
{
    // Get a vector that will hold the notes we will reap.
    std::vector<std::shared_ptr<note_data>> notes_to_reap;

    // Go through all of the notes and advance their duration
    for(const auto& note : m_notes)
    {
        assert(note);
        // Only care about positive timed notes.
        if(note->m_duration > 0.0f)
        {
            // Subtract away the milliseconds that passed.
            note->m_duration -= 1000.0f * static_cast<float>(num_samples) / static_cast<float>(sample_rate);
            if(note->m_duration < 0.0f)
            {
                notes_to_reap.push_back(note);
            }
        }
    }

    // For every note that we want to reap, reap it.
    for(const auto& reap_note : notes_to_reap)
    {
        m_notes.remove(reap_note);
    }
}
