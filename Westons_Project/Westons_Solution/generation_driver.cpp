#include "generation_driver.h"
#include "sound_data.h"

#include <sstream>

// Our data pointer.
static std::shared_ptr<generation_driver::generation_data> m_data_ptr;

// The precision that the built in raspberry pi audio output is capable of.
static int lookup_table_size(1 << 11);

// values used in the callback.
static float volume;
static sound_data sound;

void generation_driver::init(const callback_data& data)
{
    m_data_ptr = std::make_shared<generation_data>(lookup_table_size, data);

    // Notes are so loud by themselves at max volume. Drop that down!
    volume = 0.1f;
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
    const auto data = static_cast<generation_data*>(user_data);

    // Check for valid values.
    assert(data->data.num_input_channels == 0);
    assert(data->data.num_output_channels >= 1);

    auto* out = static_cast<float*>(output_buffer);

    for (unsigned int i = 0; i < frames_per_buffer; i++)
    {
        // Make sure nothing is wrong with volume.
        assert(volume >= 0.0f && volume <= 1.0f);

        auto play_val = 0.0f;

        // Go through all of the notes in the sound and advance their current phase.
        for(auto note : sound.m_notes)
        {
            const auto note_volume = sound.get_note_volume();

            switch (note->m_wave)
            {
            case sine:
                play_val += data->sine[phase_to_index(note->m_current_phase, lookup_table_size)] * note_volume;
                note->m_current_phase = two_pi_wrapper(note->m_current_phase + two_pi * note->m_frequency / static_cast<float>(data->data.sample_rate));
                break;
            case square:
                play_val += data->square[phase_to_index(note->m_current_phase, lookup_table_size)] * note_volume;
                note->m_current_phase = two_pi_wrapper(note->m_current_phase + two_pi * note->m_frequency / static_cast<float>(data->data.sample_rate));
                break;
            case triangle:
                play_val += data->triangle[phase_to_index(note->m_current_phase, lookup_table_size)] * note_volume;
                note->m_current_phase = two_pi_wrapper(note->m_current_phase + two_pi * note->m_frequency / static_cast<float>(data->data.sample_rate));
                break;
            case sawtooth:
                play_val += data->sawtooth[phase_to_index(note->m_current_phase, lookup_table_size)] * note_volume;
                note->m_current_phase = two_pi_wrapper(note->m_current_phase + two_pi * note->m_frequency / static_cast<float>(data->data.sample_rate));
                break;
            default:
                assert(false); // We should never hit default.
                break;
            }
        }

        // Process all the notes every sample.
        sound.process(data->data.sample_rate, 1);

        // Apply the master volume.
        play_val = clipped_output(play_val * volume);

        // Playback to the output.
        for (auto j = 0; j < data->data.num_output_channels; ++j)
        {
            out[data->data.num_output_channels*i + j] = play_val;
        }

    }
    return 0;
}

void generation_driver::processor()
{
    std::cout << std::endl << "Started Frequency Generator mode." << std::endl;

    // string that will find {0 or 1 -}{1+ digits}{0 or 1 period}{0+ digits}
    const std::string float_regex_string = R"(-?\d+\.?\d*)";

    // Various regex values.
    const std::string start_of_string_regex_string = "^";
    const std::string end_of_string_regex_string = "$";
    const std::string or_match_regex_string = "|";

    const std::string set_volume_string = "setVolume:";

    // regex that tests for setting the volume.
    const std::regex set_volume_regex(start_of_string_regex_string + set_volume_string // string identifier
        + float_regex_string + end_of_string_regex_string); // volume

    // Tell them how to adjust the volume.
    std::cout << "To adjust volume, enter: '" << set_volume_string << "{0.0 <-> 100.0}'" << std::endl;
    std::cout << "Example. setVolume:10.0" << std::endl;

    // strings needed for adding a note.
    const std::string add_note_string = "addNote:";
    const char add_note_separator = ':';

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

        // Check for setting the volume.
        if(std::regex_match(read_string, set_volume_regex))
        {
            // Match, lets adjust volume.

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
                std::cout << "Unable to change the volume, could not convert '" << volume_string << "' to float." << std::endl;
                continue;
            }

            if(new_volume < 0.0f)
            {
                std::cout << "Cannot have negative volume, setting it to 0.0." << std::endl;
                new_volume = 0.0f;
            }

            if (new_volume > 100.0f)
            {
                std::cout << "Cannot have volume above 100.0, setting it to 100.0." << std::endl;
                new_volume = 100.0f;
            }

            // Update the volume and tell the user.
            volume = new_volume / 100.0f;
            std::cout << "Set Volume to : " << volume << std::endl;

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
            wave_type wave;

            try
            {
                frequency = std::stof(frequency_string);
                phase = std::stof(phase_string);
                duration = std::stof(duration_string);
                wave = from_string(wave_string);
            }
            catch (...)
            {
                std::cout << "Unable to parse add note string" << std::endl;
                continue;
            }

            // Make the new note, tell the user about it, then add it.
            const auto new_note = std::make_shared<note_data>(frequency, phase, duration, wave);

            std::cout << "Adding a new note with Frequency: " << frequency 
            << ", Phase Offset: " << phase 
            << ", Duration: " << duration 
            << ", Wave Type: " << wave_string << std::endl;

            sound.add_note(new_note);
            continue;
        }

        std::cout << "Unable to match the string '" << read_string << "' to any existing functions" << std::endl;
    }

    std::cout << "Exiting Frequency Generator mode." << std::endl;
}

/**
* \brief Gets the data pointer that needs to be passed to the callback function.
* \return Data pointer for the callback function.
*/
void* generation_driver::get_data()
{
    assert(m_data_ptr);
    return m_data_ptr.get();
}
