#pragma once
#include "src/sound/note_data.h"
#include <list>
#include <memory>

class sound_data
{
public:
    void add_note(std::shared_ptr<note_data> new_note);

    void remove_note(const std::shared_ptr<note_data>& remove_note);

    float get_note_volume() const;

    void process(int sample_rate, int num_samples);

    // List of all the notes currently in the sound.
    std::list<std::shared_ptr<note_data>> m_notes;
};

