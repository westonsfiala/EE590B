#include <iostream>
#include <cassert>

// Port Audio Includes
// All credit to: http://www.portaudio.com/
#include "Audio Driver/audio_driver.h"

// RtMidi
// All credit to: http://www.music.mcgill.ca/~gary/rtmidi/
#include "rtmidi/RtMidi.h"

#include "sound/sound_utilities.h"
#include "../passthrough_driver.h"
#include "../generation_driver.h"
#include "../midi_driver.h"

int main()
{
    std::cout << "Starting Up!" << std::endl;

    std::cout << std::endl << "Booting up Audio Driver" << std::endl;

    // Get vector of all the callbacks that have been constructed.
    std::vector<sound_utilities::callback_info> available_callbacks;

    // Passthrough
    auto pass_call_data = sound_utilities::callback_data();
    if (passthrough_driver::init(pass_call_data))
    {
        const auto passthrough_info = sound_utilities::callback_info(passthrough_driver::callback, pass_call_data,
                                                                     passthrough_driver::get_data(), "Passthrough",
                                                                     passthrough_driver::processor);
        available_callbacks.push_back(passthrough_info);
    }
    else
    {
        std::cout << "Passthrough Driver could not be initialized and will be disabled." << std::endl;
    }

    // Frequency Generator
    auto gen_call_data = sound_utilities::callback_data();
    if (generation_driver::init(gen_call_data))
    {
        const auto frequency_gen_info = sound_utilities::callback_info(generation_driver::callback, gen_call_data,
                                                                       generation_driver::get_data(),
                                                                       "Frequency Generation",
                                                                       generation_driver::processor);
        available_callbacks.push_back(frequency_gen_info);
    }
    else
    {
        std::cout << "Frequency Generation Driver could not be initialized and will be disabled." << std::endl;
    }

    // Midi Reader
    auto midi_call_data = sound_utilities::callback_data();
    if (midi_driver::init(midi_call_data))
    {
        const auto midi_gen_info = sound_utilities::callback_info(midi_driver::callback, midi_call_data,
                                                                  midi_driver::get_data(), "Midi player",
                                                                  midi_driver::processor);
        available_callbacks.push_back(midi_gen_info);
    }
    else
    {
        std::cout << "Midi Driver could not be initialized and will be disabled." << std::endl;
    }

    auto quit = false;

    // If we have no drivers, quit out.
    if (available_callbacks.empty())
    {
        std::cout << "No drivers are currently enabled, exiting program" << std::endl;
        quit = true;

        // Assert here so that it will break instead of just exiting.
        assert(false);
    }
    else
    {
        // Initialize the program, and let the user know how to operate it.
        std::cout << "Starting Audio Driver program." << std::endl;
    }

    // Until promted to exit, try to run.
    while (!quit)
    {
        std::cout << "Please select an available mode to use by entering its associated number:" << std::endl;

        for (auto i = 0; i < static_cast<int>(available_callbacks.size()); ++i)
        {
            std::cout << "[" << i << "]: " << available_callbacks[i].m_callback_name << std::endl;
        }

        const std::string exit_string = "exit";

        std::cout << "Enter '" << exit_string << "' to exit program" << std::endl << std::endl;

        std::string read_string;
        std::cin >> read_string;

        // Catch the exit condition
        if (read_string == exit_string)
        {
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
            continue;
        }

        // If we have a parsed value in a valid range, start up that callback!
        if (parsed_value >= 0 && parsed_value < static_cast<int>(available_callbacks.size()))
        {
            const auto selected_callback = available_callbacks[parsed_value];

            // Construct the driver
            auto driver = audio_driver(selected_callback);

            // Start it up!
            if (!driver.start())
            {
                std::cerr << "Failed to start [" << selected_callback.m_callback_name << "]" << std::endl << "Error: " +
                    driver.get_error() << std::endl;
                continue;
            }

            // Go into the method that processes the callback.
            selected_callback.m_process_method();

            // Stop the driver.
            if (!driver.stop())
            {
                std::cerr << "Failed to stop [" << selected_callback.m_callback_name << "]" << std::endl << "Error: " +
                    driver.get_error() << std::endl;
                continue;
            }
        }

        std::cout << "Stopping Audio Driver" << std::endl;
    }

    std::cout << "Exiting Audio Driver" << std::endl;

    return 0;
}
