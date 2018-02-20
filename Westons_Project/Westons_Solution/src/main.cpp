#include <iostream>
#include <cassert>

// Port Audio Includes
// All credit to: http://www.portaudio.com/
#include "Audio Driver/audio_driver.h"

// RtMidi
// All credit to: http://www.music.mcgill.ca/~gary/rtmidi/
#include "rtmidi/RtMidi.h"

#include "sound/sound_utilities.h"

int main()
{
    std::cout << "Starting Up!" << std::endl;

    std::cout << "Finding MIDI ports" << std::endl;

    // Test rtmidi
    std::shared_ptr<RtMidiIn> midiin = nullptr;
    std::shared_ptr<RtMidiOut> midiout = nullptr;
    // RtMidiIn constructor
    try {
        midiin = std::make_shared<RtMidiIn>();
    }
    catch (RtMidiError &error) {
        error.printMessage();
        exit(EXIT_FAILURE);
    }

    // Check inputs.
    unsigned int nPorts = midiin->getPortCount();
    std::cout << std::endl << "There are " << nPorts << " MIDI input sources available." << std::endl;
    std::string portName;
    for (unsigned int i = 0; i<nPorts; i++) {
        try {
            portName = midiin->getPortName(i);
        }
        catch (RtMidiError &error) {
            error.printMessage();
            break;
        }
        std::cout << "  Input Port #" << i + 1 << ": " << portName << std::endl;
    }
    // RtMidiOut constructor
    try {
        midiout = std::make_shared<RtMidiOut>();
    }
    catch (RtMidiError &error) {
        error.printMessage();
        exit(EXIT_FAILURE);
    }

    // Check outputs.
    nPorts = midiout->getPortCount();
    std::cout << std::endl << "There are " << nPorts << " MIDI output ports available." << std::endl;
    for (unsigned int i = 0; i<nPorts; i++) {
        try {
            portName = midiout->getPortName(i);
        }
        catch (RtMidiError &error) {
            error.printMessage();
            break;
        }
        std::cout << "  Output Port #" << i + 1 << ": " << portName << std::endl;
    }
    std::cout << "Ending MIDI Query" << std::endl;

    // End rtmidi

    std::cout << std::endl << "Booting up Audio Driver" << std::endl;

    // Set up our static variables.
    frequency = 440.0f;

    // Get vector of all the callbacks that have been constructed.
    std::vector<callback_info> available_callbacks;

    // Passthrough
    auto passthrough_info = callback_info(passthrough_callback, std::make_shared<callback_data>(0.0, 1, 1, default_sample_rate), "Passthrough", passthrough_processer);
    available_callbacks.push_back(passthrough_info);

    // Frequency Generator
    auto frequency_gen_info = callback_info(frequency_gen_callback, std::make_shared<callback_data>(0.0, 0, 1, default_sample_rate), "Frequency Generation", frequency_gen_processer);
    available_callbacks.push_back(frequency_gen_info);

    // Initialize the program, and let the user know how to operate it.
    std::cout << "Starting Audio Driver program." << std::endl;

    auto quit = false;

    // Until promted to exit, try to run.
    while (!quit)
    {
        std::cout << "Please select an available mode to use by entering its associated number:" << std::endl;

        for(auto i = 0; i < static_cast<int>(available_callbacks.size()); ++i)
        {
            std::cout << "[" << i << "]: " << available_callbacks[i].m_callback_name << std::endl;
        }

        const std::string exit_string = "exit";

        std::cout << "Enter '" << exit_string << "' to exit program" << std::endl << std::endl;

        std::string read_string;
        std::cin >> read_string;

        // Catch the exit condition
        if(read_string == exit_string)
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
        if(parsed_value >= 0 && parsed_value < static_cast<int>(available_callbacks.size()))
        {
            auto selected_callback = available_callbacks[parsed_value];

            // Construct the driver
            auto driver = audio_driver(selected_callback);

            // Start it up!
            if (!driver.start())
            {
                std::cerr << "Failed to start [" << selected_callback.m_callback_name << "]" << std::endl << "Error: " + driver.get_error() << std::endl;
                return 1;
            }

            // Go into the method that processes the callback.
            selected_callback.m_process_method();

            // Stop the driver.
            if(!driver.stop())
            {
                std::cerr << "Failed to stop [" << selected_callback.m_callback_name << "]" << std::endl << "Error: " + driver.get_error() << std::endl;
                return 1;
            }
        }

        std::cout << "Stopping Audio Driver" << std::endl;
    }

    std::cout << "Exiting Audio Driver" << std::endl;

    return 0;
}
