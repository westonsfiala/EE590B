#include <cstdio>
#include <wiringPi.h>

#include "portaudio.h"

typedef struct
{
    float left_phase;
    float right_phase;
}

pa_test_data;

/* This routine will be called by the PortAudio engine when audio is needed.
It may called at interrupt level on some machines so don't do anything
that could mess up the system like calling malloc() or free().
*/
static int patest_callback(const void* input_buffer, void* output_buffer,
                          const unsigned long frames_per_buffer,
                          const PaStreamCallbackTimeInfo* time_info,
                          PaStreamCallbackFlags status_flags,
                          void* user_data)
{
    static_cast<void>(status_flags);
    static_cast<void>(time_info);
    /* Cast data passed through stream to our structure. */
    auto data = static_cast<pa_test_data*>(user_data);
    auto* out = static_cast<float*>(output_buffer);
    (void)input_buffer; /* Prevent unused variable warning. */

    for (unsigned int i = 0; i < frames_per_buffer; i++)
    {
        *out++ = data->left_phase; /* left */
        *out++ = data->right_phase; /* right */
        /* Generate simple sawtooth phaser that ranges between -1.0 and 1.0. */
        data->left_phase += 0.01f;
        /* When signal reaches top, drop back down. */
        if (data->left_phase >= 1.0f) data->left_phase -= 2.0f;
        /* higher pitch so we can distinguish left and right. */
        data->right_phase += 0.03f;
        if (data->right_phase >= 1.0f) data->right_phase -= 2.0f;
    }
    return 0;
}

#define SAMPLE_RATE (44100)
static pa_test_data data;

int main()
{
    wiringPiSetup();

    printf("Hello World\n");

    auto err = Pa_Initialize();
    if (err != paNoError) goto error;

    PaStream* stream;
    /* Open an audio I/O stream. */
    err = Pa_OpenDefaultStream(&stream,
                               0, /* no input channels */
                               2, /* stereo output */
                               paFloat32, /* 32 bit floating point output */
                               SAMPLE_RATE,
                               256, /* frames per buffer, i.e. the number
                    of sample frames that PortAudio will
                    request from the callback. Many apps
                    may want to use
                    paFramesPerBufferUnspecified, which
                    tells PortAudio to pick the best,
                    possibly changing, buffer size.*/
                               patest_callback, /* this is your callback function */
                               &data); /*This is a pointer that will be passed to
                your callback*/

    if (err != paNoError) goto error;

    err = Pa_StartStream(stream);
    if (err != paNoError) goto error;

    /* Sleep for several seconds. */
    Pa_Sleep(5 * 1000);

    err = Pa_StopStream(stream);
    if (err != paNoError) goto error;

    err = Pa_CloseStream(stream);
    if (err != paNoError) goto error;

error:
    err = Pa_Terminate();
    if (err != paNoError)
    {
        printf("PortAudio error: %s\n", Pa_GetErrorText(err));
        return 1;
    }

    return 0;
}
