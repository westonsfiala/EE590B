#include "midi_driver.h"
#include <iostream>
#include "src/rtmidi/RtMidi.h"
#include <cassert>
#include <chrono>
#include <thread>
#include "src/Audio Driver/audio_driver.h"


bool midi_driver::initializied_ = false;;

sound_utilities::callback_data midi_driver::data_ = sound_utilities::callback_data();

// Map that keeps track of the different notes that are currently being played.
std::map<int, std::shared_ptr<note_data>> midi_driver::note_map_ = std::map<int, std::shared_ptr<note_data>>();

// Midi reader for processing inputs;
std::shared_ptr<RtMidiIn> midi_driver::midi_reader_ = nullptr;

// The volume and sound that will be processed by the callback.
float midi_driver::volume_ = 0.0f;
sound_data midi_driver::sound_ = sound_data();

/**
* \brief Checks if the driver can be run at this time, and fills out the callback data.
* \param data Callback data reference to fill.
* \return If the driver can be run.
*/
bool midi_driver::init(sound_utilities::callback_data& data)
{
    if(!audio_driver::check_channels(0,1))
    {
        return false;
    }

    // Get our reader setup.
    try {
        midi_reader_ = std::make_shared<RtMidiIn>();
    }
    catch (RtMidiError &error) {
        error.printMessage();
        return false;
    }
    
    // See if there are any ports we can work with.
    if(midi_reader_->getPortCount() == 0)
    {
        std::cout << "No MIDI channels are available." << std::endl;
        return false;
    }

    // Setup the values to what we allow them to be.
    data.num_input_channels = 0;
    data.num_output_channels = 1;
    data.sample_rate = sound_utilities::default_sample_rate;

    // Get our data pointer ready.
    data_ = data;

    // Notes are so loud by themselves at max volume. Drop that down!
    volume_ = 0.1f;

    // Say that we have been initialized.
    initializied_ = true;

    return initializied_;
}

int midi_driver::callback(const void* input_buffer, void* output_buffer, unsigned long frames_per_buffer,
    const PaStreamCallbackTimeInfo* time_info, PaStreamCallbackFlags status_flags, void* user_data)
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
    assert(initializied_);

    auto* out = static_cast<float*>(output_buffer);

    uint64_t tracker = 0;
    for (unsigned int i = 0; i < frames_per_buffer; i++)
    {
        // Make sure nothing is wrong with volume_.
        assert(volume_ >= 0.0f && volume_ <= 1.0f);

        auto play_val = 0.0f;

        // Go through all of the notes in the sound, get their value, and advance their phase.
        for (auto& note : sound_.m_notes)
        {
            const auto note_volume = sound_.m_note_volume;

            switch (note.m_wave)
            {
            case sound_utilities::sine:
                play_val += sound_utilities::wave_lookup_tables.sine[sound_utilities::phase_to_index(note.m_current_phase, sound_utilities::table_size)] * note_volume;
                break;
            case sound_utilities::square:
                play_val += sound_utilities::wave_lookup_tables.square[sound_utilities::phase_to_index(note.m_current_phase, sound_utilities::table_size)] * note_volume;
                break;
            case sound_utilities::triangle:
                play_val += sound_utilities::wave_lookup_tables.triangle[sound_utilities::phase_to_index(note.m_current_phase, sound_utilities::table_size)] * note_volume;
                break;
            case sound_utilities::sawtooth:
                play_val += sound_utilities::wave_lookup_tables.sawtooth[sound_utilities::phase_to_index(note.m_current_phase, sound_utilities::table_size)] * note_volume;
                break;
            default:
                assert(false); // We should never hit default.
                break;
            }

            // Advance the phase.
            note.m_current_phase = sound_utilities::two_pi_wrapper(note.m_current_phase + sound_utilities::two_pi * note.m_frequency / static_cast<float>(data->sample_rate));
        }

        // Apply the master volume_.
        play_val = sound_utilities::clipped_output(play_val * volume_);

        // Playback to the output.
        for (auto j = 0; j < data->num_output_channels; ++j)
        {
            ++tracker;
            out[data->num_output_channels*i + j] = play_val;
        }
    }

    // Just a saftey to moke sure that we actually did fill up the channels.
    assert(tracker == frames_per_buffer * data->num_output_channels);

    return 0;
}

void midi_driver::processor()
{
    // If we were never initialized, quit.
    if (!initializied_)
    {
        std::cout << "Midi Driver was not initialized. Quitting driver." << std::endl;
        return;
    }

    // This should be true if initialized is true.
    assert(midi_reader_);

    // Get a map of all the port indicies to the names of those ports.
    std::map<uint32_t, std::string> midi_port_names;

    // Get the names of all the ports.
    for (uint32_t i = 0; i < midi_reader_->getPortCount(); i++) {
        try {
            midi_port_names.emplace(i, midi_reader_->getPortName(i));
        }
        catch (RtMidiError &error) {
            // Don't tell them about a port that is bad.
            error.printMessage();
        }
    }

    if(midi_port_names.empty())
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
        if(midi_port_names.count(parsed_value) != 0)
        {
            try
            {
                midi_reader_->openPort(parsed_value);
                proceed = true;
            }
            catch (RtMidiError &error)
            {
                error.printMessage();
            }
        }
        else
        {
            std::cout << "No midi port exists with the given index: " << parsed_value << std::endl;
        }
    }

    while (!quit) {
        std::vector<uint8_t> message;
        const auto stamp = midi_reader_->getMessage(&message);
        const auto n_bytes = message.size();
        for (uint32_t i = 0; i < n_bytes; i++)
            std::cout << "Byte " << i << " = " << static_cast<int>(message[i]) << ", ";
        if (n_bytes > 0)
            std::cout << "stamp = " << stamp << std::endl;

        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    midi_reader_->closePort();
}

void* midi_driver::get_data()
{
    return &data_;
}
