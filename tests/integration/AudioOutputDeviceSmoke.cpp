#include "audio/AudioEngine.h"

#include <chrono>
#include <exception>
#include <iostream>
#include <thread>

int main()
{
    try
    {
        RBX::Audio::Engine engine({.sampleRate = 48000, .channels = 2});
        engine.startOutputDevice();
        if (!engine.outputDeviceStarted())
            return 1;
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        engine.restartOutputDevice();
        if (!engine.outputDeviceStarted())
            return 1;
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        engine.stopOutputDevice();
        return engine.outputDeviceStarted() ? 1 : 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
