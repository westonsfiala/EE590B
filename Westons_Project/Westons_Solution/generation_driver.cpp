#include "generation_driver.h"
#include "sound_data.h"
#include "src/Audio Driver/audio_driver.h"

#include <sstream>
#include <regex>
#include <memory>
#include <cassert>
#include <iostream>
#include <chrono>

bool generation_driver::initializied_ = false;;

// Our data pointer.
sound_utilities::callback_data generation_driver::data_ = sound_utilities::callback_data();

// values used in the callback.
float generation_driver::volume_ = 0.0f;
sound_data generation_driver::sound_ = sound_data();

float phase = 0.0f;

/**
* \brief Checks if the driver can be run at this time, and fills out the callback data.
* \param data Callback data reference to fill.
* \return If the driver can be run.
*/
bool generation_driver::init(sound_utilities::callback_data& data)
{
    if(!audio_driver::check_channels(0,1))
    {
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

/**
* \brief Callback that generates a sine wave on the output channel.
* \param input_buffer Buffer of values that has been captured by a device.
* \param output_buffer Buffer of values that will be output to a physical port.
* \param frames_per_buffer Number of frames in each buffer.
* \param time_info The time that the input values were captured, and the time the output values will be played.
* \param status_flags Status bits that detail the current state of the streams.
* \param user_data Pointer to some data that the callback expects.
* \return If the passthrough worked correctly. 0 for success, !0 for failure.
*/
int generation_driver::callback(const void* input_buffer, void* output_buffer,
    const unsigned long frames_per_buffer,
    const PaStreamCallbackTimeInfo* time_info,
    PaStreamCallbackFlags status_flags,
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
    assert(initializied_);

    // Do some checks for time.
    const auto alloted_time = time_info->outputBufferDacTime - time_info->currentTime;
    const auto start_time = std::chrono::system_clock::now();

    auto* out = static_cast<float*>(output_buffer);

    uint64_t tracker = 0;
    for (unsigned int i = 0; i < frames_per_buffer; i++)
    {
        // Make sure nothing is wrong with volume_.
        assert(volume_ >= 0.0f && volume_ <= 1.0f);

        auto play_val = 0.0f;

        // Go through all of the notes in the sound and advance their current phase.
        for(auto& note : sound_.m_notes)
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

        // Process all the notes every sample.
        sound_.process(data->sample_rate, 1);

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

    // See that we fulfiled the time requirements.
    std::chrono::duration<double> elapsed_time = std::chrono::system_clock::now() - start_time;
    const auto elapsed_seconds = elapsed_time.count();
    assert(elapsed_seconds < alloted_time);

    return 0;
}

void generation_driver::processor()
{
    // If we were never initialized, quit.
    if (!initializied_)
    {
        std::cout << "Generation Driver was not initialized. Quitting driver." << std::endl;
        return;
    }

    std::cout << std::endl << "Started Frequency Generator mode." << std::endl;

    // string that will find {0 or 1 -}{1+ digits}{0 or 1 period}{0+ digits}
    const std::string float_regex_string = R"(-?\d+\.?\d*)";

    // Various regex values.
    const std::string start_of_string_regex_string = "^";
    const std::string end_of_string_regex_string = "$";
    const std::string or_match_regex_string = "|";

    const std::string set_volume_string = "setVolume:";

    // regex that tests for setting the volume_.
    const std::regex set_volume_regex(start_of_string_regex_string + set_volume_string // string identifier
        + float_regex_string + end_of_string_regex_string); // volume_

    // Tell them how to adjust the volume_.
    std::cout << "To adjust volume_, enter: '" << set_volume_string << "{0.0 <-> 100.0}'" << std::endl;
    std::cout << "Example. setVolume:10.0" << std::endl;

    // strings needed for adding a note.
    const std::string add_note_string = "addNote:";
    const auto add_note_separator = ':';

    // Strings for the wave types.
    const std::string sine_string = "sine";
    const std::string square_string = "square";
    const std::string sawtooth_string = "sawtooth";
    const std::string triangle_string = "triangle";

    // regex string for finding the allowed wave types.
    const std::string valid_wave_regex_string = sine_string + or_match_regex_string // sine
    + square_string + or_match_regex_string // square
    + sawtooth_string + or_match_regex_string // sawtooth
    + triangle_string; // triangle

    const std::regex add_note_regex(start_of_string_regex_string + add_note_string // string identifier
        + float_regex_string + add_note_separator // frequency
        + float_regex_string + add_note_separator // phase
        + float_regex_string + add_note_separator // duration
        + valid_wave_regex_string + end_of_string_regex_string); // wave

    // Tell them how to add a note.
    std::cout << "To add a note, enter: '"
        << add_note_string << "{Frequency in Hz}"
        << add_note_separator << "{Phase offset in radians}"
        << add_note_separator << "{Duration in milliseconds}"
        << add_note_separator << "{Wave Type}" << std::endl;

    std::cout << "Allowed Wave Types: [" << sine_string << "/" << square_string << "/" << sawtooth_string << "/" << triangle_string << "]" << std::endl;

    std::cout << "Example. addNote:440.0:0.0:1000.0:sine" << std::endl;

    // Regex for exiting.
    const std::regex exit_regex(start_of_string_regex_string + "exit" + end_of_string_regex_string);

    // Tell them how to exit.
    std::cout << "To exit, enter 'exit'" << std::endl;

    auto quit = false;

    while (!quit)
    {
        // Wait for an input, then process it.
        std::string read_string;
        std::cin >> read_string;

        // Check for exiting.
        if(std::regex_match(read_string, exit_regex))
        {
            // Match, lets exit.
            quit = true;
            continue;
        }

        // Check for setting the volume_.
        if(std::regex_match(read_string, set_volume_regex))
        {
            // Match, lets adjust volume_.

            // Get rid of everything but the float.
            const auto volume_string = std::regex_replace(read_string, std::regex(set_volume_string), "");

            float new_volume;

            try
            {
                new_volume = std::stof(volume_string);
            }
            // If anything goes wrong. Quit.
            catch(...)
            {
                std::cout << "Unable to change the volume_, could not convert '" << volume_string << "' to float." << std::endl;
                continue;
            }

            if(new_volume < 0.0f)
            {
                std::cout << "Cannot have negative volume_, setting it to 0.0." << std::endl;
                new_volume = 0.0f;
            }

            if (new_volume > 100.0f)
            {
                std::cout << "Cannot have volume_ above 100.0, setting it to 100.0." << std::endl;
                new_volume = 100.0f;
            }

            // Update the volume_ and tell the user.
            volume_ = new_volume / 100.0f;
            std::cout << "Set Volume to : " << volume_ << std::endl;

            continue;
        }

        // Check for adding a note.
        if(std::regex_match(read_string, add_note_regex))
        {
            // Match, lets add a note.

            // Get rid of the first part
            const auto note_string = std::regex_replace(read_string, std::regex(add_note_string), "");

            auto note_string_stream = std::stringstream(note_string);

            std::vector<std::string> string_vec;
            std::string token;

            // Get all the different tokens.
            while(std::getline(note_string_stream, token, add_note_separator))
            {
                string_vec.push_back(token);
            }

            // There should be exactly 4 strings extracted this way.
            assert(string_vec.size() == 4);

            // Don't continue if something is wrong.
            if(string_vec.size() != 4)
            {
                std::cout << "Unable to parse add note string" << std::endl;
                continue;
            }

            const auto frequency_string = string_vec[0];
            const auto phase_string = string_vec[1];
            const auto duration_string = string_vec[2];
            const auto wave_string = string_vec[3];

            float frequency;
            float phase;
            float duration;
            sound_utilities::wave_type wave;

            try
            {
                frequency = std::stof(frequency_string);
                phase = std::stof(phase_string);
                duration = std::stof(duration_string);
                wave = sound_utilities::from_string(wave_string);
            }
            catch (...)
            {
                std::cout << "Unable to parse add note string" << std::endl;
                continue;
            }

            // Make the new note, tell the user about it, then add it.
            const auto new_note = note_data(frequency, phase, duration, wave);

            std::cout << "Adding a new note with Frequency: " << frequency 
            << ", Phase Offset: " << phase 
            << ", Duration: " << duration 
            << ", Wave Type: " << wave_string << std::endl;

            sound_.add_note(new_note);
            continue;
        }

        std::cout << "Unable to match the string '" << read_string << "' to any existing functions" << std::endl;
    }

    std::cout << "Exiting Frequency Generator mode." << std::endl;

    sound_.m_notes.clear();
}

/**
* \brief Gets the data pointer that needs to be passed to the callback function.
* \return Data pointer for the callback function.
*/
void* generation_driver::get_data()
{
    return &data_;
}
