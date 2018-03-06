#pragma once
#include "src/sound/note_data.h"
#include <list>

class sound_data
{
public:
    void add_note(const note_data& new_note);

    void remove_notes(float frequency);

    void process(int sample_rate, int num_samples);

    // List of all the notes currently in the sound.
    std::list<note_data> m_notes;

    float m_note_volume;

private:
    void calculate_note_volume();
};
