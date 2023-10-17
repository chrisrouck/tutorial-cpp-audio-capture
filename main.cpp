#include <stdlib.h>
#include <stdio.h>
#include <cstring>
#include <cmath>

#include <portaudio.h>
#include <fftw3.h>

#define SAMPLE_RATE 44100.0
#define FRAMES_PER_BUFFER 512
#define NUM_CHANNELS 2

#define SPECTRO_FREQ_START 20
#define SPECTRO_FREQ_END 20000

typedef struct {
    double* in;
    double* out;
    fftw_plan p;
    int startIndex;
    int spectroSize;
} streamCallbackData;

static streamCallbackData* spectroData;

static void checkErr(PaError err) {
    if (err != paNoError) {
        printf("PortAudio error: %s\n", Pa_GetErrorText(err));
        exit(EXIT_FAILURE);
    }
}

static inline float max(float a, float b) {
    return a > b ? a : b;
}

static inline float min(float a, float b) {
    return a < b ? a : b;
}

static int streamCallback(
    const void* inputBuffer, void* outputBuffer, unsigned long framesPerBuffer,
    const PaStreamCallbackTimeInfo* timeInfo, PaStreamCallbackFlags statusFlags,
    void* userData
) {
    float* in = (float*)inputBuffer;
    (void)outputBuffer;
    streamCallbackData* callbackData = (streamCallbackData*)userData;

    int dispSize = 100;
    printf("\r");

    for (unsigned long i = 0; i < framesPerBuffer; i++) {
        callbackData->in[i] = in[i * NUM_CHANNELS];
    }

    fftw_execute(callbackData->p);

    for (int i = 0; i < dispSize; i++) {
        double proportion = std::pow(i / (double)dispSize, 4);
        double freq = callbackData->out[(int)(callbackData->startIndex
            + proportion * callbackData->spectroSize)];

        if (freq < 0.125) {
            printf("▁");
        } else if (freq < 0.25) {
            printf("▂");
        } else if (freq < 0.375) {
            printf("▃");
        } else if (freq < 0.5) {
            printf("▄");
        } else if (freq < 0.625) {
            printf("▅");
        } else if (freq < 0.75) {
            printf("▆");
        } else if (freq < 0.875) {
            printf("▇");
        } else {
            printf("█");
        }
    }

    fflush(stdout);

    return 0;
}

int main() {
    PaError err;
    err = Pa_Initialize();
    checkErr(err);

    spectroData = (streamCallbackData*)malloc(sizeof(streamCallbackData));
    spectroData->in = (double*)malloc(sizeof(double) * FRAMES_PER_BUFFER);
    spectroData->out = (double*)malloc(sizeof(double) * FRAMES_PER_BUFFER);
    if (spectroData->in == NULL || spectroData->out == NULL) {
        printf("Could not allocate spectro data\n");
        exit(EXIT_FAILURE);
    }
    spectroData->p = fftw_plan_r2r_1d(
        FRAMES_PER_BUFFER, spectroData->in, spectroData->out,
        FFTW_R2HC, FFTW_ESTIMATE
    );
    double sampleRatio = FRAMES_PER_BUFFER / SAMPLE_RATE;
    spectroData->startIndex = std::ceil(sampleRatio * SPECTRO_FREQ_START);
    spectroData->spectroSize = min(
        std::ceil(sampleRatio * SPECTRO_FREQ_END),
        FRAMES_PER_BUFFER / 2.0
    ) - spectroData->startIndex;

    int numDevices = Pa_GetDeviceCount();
    printf("Number of devices: %d\n", numDevices);

    if (numDevices < 0) {
        printf("Error getting device count.\n");
        exit(EXIT_FAILURE);
    } else if (numDevices == 0) {
        printf("There are no available audio devices on this machine.\n");
        exit(EXIT_SUCCESS);
    }

    const PaDeviceInfo* deviceInfo;
    for (int i = 0; i < numDevices; i++) {
        deviceInfo = Pa_GetDeviceInfo(i);
        printf("Device %d:\n", i);
        printf("  name: %s\n", deviceInfo->name);
        printf("  maxInputChannels: %d\n", deviceInfo->maxInputChannels);
        printf("  maxOutputChannels: %d\n", deviceInfo->maxOutputChannels);
        printf("  defaultSampleRate: %f\n", deviceInfo->defaultSampleRate);
    }

    int device = 0;

    PaStreamParameters inputParameters;
    PaStreamParameters outputParameters;

    memset(&inputParameters, 0, sizeof(inputParameters));
    inputParameters.channelCount = NUM_CHANNELS;
    inputParameters.device = device;
    inputParameters.hostApiSpecificStreamInfo = NULL;
    inputParameters.sampleFormat = paFloat32;
    inputParameters.suggestedLatency = Pa_GetDeviceInfo(device)->defaultLowInputLatency;

    PaStream* stream;
    err = Pa_OpenStream(
        &stream,
        &inputParameters,
        NULL,
        SAMPLE_RATE,
        FRAMES_PER_BUFFER,
        paNoFlag,
        streamCallback,
        spectroData
    );
    checkErr(err);

    err = Pa_StartStream(stream);
    checkErr(err);

    Pa_Sleep(30 * 1000);

    err = Pa_StopStream(stream);
    checkErr(err);

    err = Pa_CloseStream(stream);
    checkErr(err);

    err = Pa_Terminate();
    checkErr(err);

    fftw_destroy_plan(spectroData->p);
    fftw_free(spectroData->in);
    fftw_free(spectroData->out);
    free(spectroData);

    printf("\n");

    return EXIT_SUCCESS;
}
