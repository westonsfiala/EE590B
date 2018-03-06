#include "midi_driver.h"
#include "sound_data.h"
#include "src/rtmidi/RtMidi.h"
#include "src/Audio Driver/audio_driver.h"

#include <cassert>
#include <iostream>
#include <queue>
#include <cmath>
#include <map>


sound_utilities::callback_data midi_driver::data_ = sound_utilities::callback_data();

static bool midi_initialized = false;

static bool midi_callback_active = false;

// The volume and sound that will be processed by the callback.
static sound_data midi_sound = sound_data();

static sound_utilities::wave_type midi_current_wave = sound_utilities::sine;
static bool dynamic_note_volume = true;

// Values needed to process the midi bytes.
static const uint8_t channel_one_key_press = 144;

static const uint8_t non_dynamic_volume_key = 0;
static const uint8_t sine_wave_key = 64;
static const uint8_t square_wave_key = 65;
static const uint8_t sawtooth_wave_key = 66;
static const uint8_t triangle_wave_key = 67;

static const uint8_t quit_midi_key = 81;

static const uint8_t channel_two_key_press = 145;
static const uint8_t channel_two_key_release = 129;

static const uint8_t middle_note_value = 60;
static const float middle_note_frequency = 440.0f;

static const uint8_t max_volume_value = 127;

static std::vector<float> frequencies_vector;

// Half step value.
static const float twelth_root_two = std::exp2(1.0f / 12.0f);

/**
* \brief Checks if the driver can be run at this time, and fills out the callback data.
* \param data Callback data reference to fill.
* \return If the driver can be run.
*/
bool midi_driver::init(sound_utilities::callback_data& data)
{
    if (!audio_driver::check_channels(0, 1))
    {
        return false;
    }

    RtMidiIn* midi_reader;
    // Get our reader setup.
    try
    {
        midi_reader = new RtMidiIn();
    }
    catch (RtMidiError& error)
    {
        error.printMessage();
        return false;
    }

    // See if there are any ports we can work with.
    if (midi_reader->getPortCount() == 0)
    {
        std::cout << "No MIDI channels are available." << std::endl;
        return false;
    }

    delete midi_reader;

    // Setup the values to what we allow them to be.
    data.num_input_channels = 0;
    data.num_output_channels = 1;
    data.sample_rate = sound_utilities::default_sample_rate;

    // Get our data pointer ready.
    data_ = data;

    // We are not in the callback, so it is false.
    midi_callback_active = false;

    // Fill out the frequency vector. No need to calculate this on the fly all the time.
    for (auto i = 0; i <= 127; ++i)
    {
        const auto frequency_modifier = std::pow(twelth_root_two, i - middle_note_value);
        frequencies_vector.push_back(middle_note_frequency * frequency_modifier);
    }

    // Say that we have been initialized.
    midi_initialized = true;

    return midi_initialized;
}

int midi_driver::callback(const void* input_buffer, void* output_buffer, unsigned long frames_per_buffer,
                          const PaStreamCallbackTimeInfo* time_info, PaStreamCallbackFlags status_flags,
                          void* user_data)
{
    // stop warnings by casting to void.
    static_cast<void>(status_flags);
    static_cast<void>(time_info);
    static_cast<void>(input_buffer);

    // Get the data that we care about.
    const auto data = static_cast<sound_utilities::callback_data*>(user_data);

    // Check for valid values.
    assert(data->num_input_channels == 0);
    assert(data->num_output_channels >= 1);
    assert(midi_initialized);

    auto* out = static_cast<float*>(output_buffer);

    uint64_t tracker = 0;
    midi_callback_active = true;
    for (unsigned int i = 0; i < frames_per_buffer; i++)
    {
        auto play_val = 0.0f;

        // Go through all of the notes in the sound, get their value, and advance their phase.
        for (auto& note : midi_sound.m_notes)
        {
            const auto note_volume = midi_sound.m_note_volume * note.m_volume;
            const auto note_index = sound_utilities::phase_to_index(note.m_current_phase, sound_utilities::table_size);

            switch (note.m_wave)
            {
            case sound_utilities::sine:
                play_val += sound_utilities::wave_lookup_tables.sine[note_index] * note_volume;
                break;
            case sound_utilities::square:
                play_val += sound_utilities::wave_lookup_tables.square[note_index] * note_volume;
                break;
            case sound_utilities::triangle:
                play_val += sound_utilities::wave_lookup_tables.triangle[note_index] * note_volume;
                break;
            case sound_utilities::sawtooth:
                play_val += sound_utilities::wave_lookup_tables.sawtooth[note_index] * note_volume;
                break;
            default:
                assert(false); // We should never hit default.
                break;
            }

            // Advance the phase.
            note.m_current_phase = sound_utilities::two_pi_wrapper(
                note.m_current_phase + sound_utilities::two_pi * note.m_frequency / static_cast<float>(data->sample_rate
                ));
        }

        // Apply the master volume.
        play_val = sound_utilities::clipped_output(play_val * sound_utilities::non_clip_volume);

        // Playback to the output.
        for (auto j = 0; j < data->num_output_channels; ++j)
        {
            ++tracker;
            out[data->num_output_channels * i + j] = play_val;
        }
    }

    // Just a saftey to moke sure that we actually did fill up the channels.
    assert(tracker == frames_per_buffer * data->num_output_channels);

    midi_callback_active = false;
    return 0;
}

void midi_driver::processor()
{
    // If we were never initialized, quit.
    if (!midi_initialized)
    {
        std::cout << "Midi Driver was not initialized. Quitting driver." << std::endl;
        return;
    }

    RtMidiIn* midi_reader;
    // Get our reader setup.
    try
    {
        midi_reader = new RtMidiIn();
    }
    catch (RtMidiError& error)
    {
        error.printMessage();
        return;
    }

    // Get a map of all the port indicies to the names of those ports.
    std::map<uint32_t, std::string> midi_port_names;

    // Get the names of all the ports.
    for (uint32_t i = 0; i < midi_reader->getPortCount(); i++)
    {
        try
        {
            midi_port_names.emplace(i, midi_reader->getPortName(i));
        }
        catch (RtMidiError& error)
        {
            // Don't tell them about a port that is bad.
            error.printMessage();
        }
    }

    if (midi_port_names.empty())
    {
        std::cout << "No Midi ports exist." << std::endl;
        return;
    }

    // Prompt the user to continue.
    std::cout << "Please select an available midi port to use by entering its associated number:" << std::endl;

    for (const auto& key_value_pair : midi_port_names)
    {
        const auto key = key_value_pair.first;
        const auto value = key_value_pair.second;
        std::cout << "[" << key << "]: " << value << std::endl;
    }

    const std::string exit_string = "exit";

    std::cout << "Enter '" << exit_string << "' to exit driver" << std::endl << std::endl;

    auto proceed = false;
    auto quit = false;

    while (!proceed)
    {
        std::string read_string;
        std::cin >> read_string;

        // Catch the exit condition
        if (read_string == exit_string)
        {
            proceed = true;
            quit = true;
            continue;
        }

        // Get the parsed value from stoi. 
        int parsed_value;
        try
        {
            parsed_value = std::stoi(read_string);
        }
            // Catch all exceptions. If something bad slipped through the cracks, continue without changing anything.
        catch (...)
        {
            std::cout << "Could not parse the given string to an integer: " << read_string << std::endl;
            continue;
        }

        // See if we actually have a port with the provided index.
        if (midi_port_names.count(parsed_value) != 0)
        {
            try
            {
                midi_reader->openPort(parsed_value);
                proceed = true;
            }
            catch (RtMidiError& error)
            {
                error.printMessage();
            }
        }
        else
        {
            std::cout << "No midi port exists with the given index: " << parsed_value << std::endl;
        }
    }

    // We won't always be ready to deal with the messages, so get a buffer.
    std::queue<std::vector<uint8_t>> waiting_messages;

    // Lets do some processing.
    while (!quit)
    {
        // Getting the message is a non-blocking check.
        std::vector<uint8_t> message;
        midi_reader->getMessage(&message);

        // Get how many bytes we have.
        const auto n_bytes = message.size();

        // We have a message, add it to the queue.
        if (n_bytes > 0)
        {
            assert(n_bytes == 3);
            waiting_messages.push(message);
            for (uint32_t i = 0; i < n_bytes; i++)
                std::cout << "Byte " << i << " = " << static_cast<int>(message[i]) << ", ";
            std::cout << std::endl;
        }

        if (!waiting_messages.empty())
        {
            // So long as the callback is not active, we can add and remove notes freely.
            while (!static_cast<volatile bool>(midi_callback_active) && !waiting_messages.empty())
            {
                // Get the message at the start of the queue.
                auto read_message = waiting_messages.front();
                waiting_messages.pop();

                const auto action = read_message[0];
                const auto note = read_message[1];
                const auto modifier = read_message[2];

                // Channel 2 key press is an add note action.
                if (action == channel_two_key_press)
                {
                    midi_sound.add_note(calculate_note(note, modifier));
                }
                    // Channel 2 key release is a remove note action.
                else if (action == channel_two_key_release)
                {
                    midi_sound.remove_notes(frequencies_vector[note]);
                }
                    // Channel 1 key press is a modify state action.
                else if (action == channel_one_key_press)
                {
                    if (note == non_dynamic_volume_key)
                    {
                        dynamic_note_volume = !dynamic_note_volume;
                    }
                    else if (note == sine_wave_key)
                    {
                        midi_current_wave = sound_utilities::sine;
                    }
                    else if (note == square_wave_key)
                    {
                        midi_current_wave = sound_utilities::square;
                    }
                    else if (note == sawtooth_wave_key)
                    {
                        midi_current_wave = sound_utilities::sawtooth;
                    }
                    else if (note == triangle_wave_key)
                    {
                        midi_current_wave = sound_utilities::triangle;
                    }
                    else if (note == quit_midi_key)
                    {
                        quit = true;
                    }
                }
            }
        }
    }

    midi_reader->closePort();

    delete midi_reader;
}

void* midi_driver::get_data()
{
    return &data_;
}

/**
 * \brief Calculates the note that corresponds to the note given values.
 * \param note Note value that determines what frequency will be played.
 * \param volume Volume that the note should be played at.
 * \return Note that corresponds to the given values.
 */
note_data midi_driver::calculate_note(const uint8_t note, const uint8_t volume)
{
    assert(note < frequencies_vector.size());

    auto volume_float = 1.0f;
    if (dynamic_note_volume)
    {
        volume_float = static_cast<float>(volume) / max_volume_value;
    }
    assert(volume_float >= 0.0f && volume_float <= 1.0f);

    // Make a note that will last forever.
    return note_data(frequencies_vector[note], 0.0, -1, volume_float, midi_current_wave);
}
