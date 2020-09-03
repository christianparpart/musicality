#pragma once

#include <atomic>
#include <vector>
#include <cstdio>

#include <portaudio.h>

class PitchDetector
{
  private:
    using Sample = float;

    int channelCount_ = 1;
    PaStream* stream_ = nullptr;

    std::atomic<unsigned> count_ = 0;
    long frameIndex_ = 0;
    std::vector<Sample> recordedSamples_;

  public:
    std::atomic<bool> terminating = false;

    PitchDetector() : PitchDetector(44100, 512, 1) {}

    PitchDetector(int _sampleRate, int _framesPerBuffer, int _numChannels) :
        channelCount_(_numChannels)
    {
        instance = this;

        Pa_Initialize();

        auto const numSeconds = 2;
        auto const numFrames = numSeconds * _sampleRate;
        auto const numSamples = numFrames * _numChannels;
        recordedSamples_.resize(numSamples, Sample{});

        PaStreamParameters inputParameters;
        inputParameters.device = Pa_GetDefaultInputDevice();
        inputParameters.channelCount = channelCount_;
        inputParameters.sampleFormat = paFloat32;
        inputParameters.suggestedLatency = Pa_GetDeviceInfo( inputParameters.device )->defaultLowInputLatency;
        inputParameters.hostApiSpecificStreamInfo = nullptr;

        PaStreamCallback sc;
        Pa_OpenStream(&stream_,
                      &inputParameters,
                      nullptr, // outputParameters
                      _sampleRate,
                      _framesPerBuffer,
                      paClipOff,
                      &PitchDetector::streamCallback,
                      this);
    }

    ~PitchDetector()
    {
        Pa_Terminate();
        instance = nullptr;
    }

    static int streamCallback(void const* input,
                               void* output,
                               unsigned long frameCount,
                               PaStreamCallbackTimeInfo const* timeInfo,
                               PaStreamCallbackFlags statusFlags,
                               void* userdata)
    {
        PitchDetector* pd = (PitchDetector*) userdata;
        return pd->input(input, output, frameCount, timeInfo, statusFlags);
    }

    int input(void const* inputBuffer,
              void* outputBuffer,
              unsigned long frameCount,
              PaStreamCallbackTimeInfo const* timeInfo,
              PaStreamCallbackFlags statusFlags)
    {
        (void) outputBuffer;
        (void) timeInfo;
        (void) statusFlags;

        count_ += frameCount;

        Sample const* rptr = (Sample const*) inputBuffer;

        for (unsigned long i = 0; i < frameCount; ++i)
        {
            Sample* wptr = &recordedSamples_[(frameIndex_ * channelCount_ + i) % recordedSamples_.size()];
            *wptr = *rptr;
            ++wptr;
            ++rptr;

            if (channelCount_ == 2)
                *wptr++ = *rptr++;
        }
        frameIndex_ += static_cast<long>(frameCount);

        return terminating.load() ? paComplete : paContinue;
    }

    void run()
    {
        Pa_StartStream(stream_);
        PaError err = paNoError;
        while ((err = Pa_IsStreamActive(stream_)) == 1)
        {
            Pa_Sleep(1000); // 1sec

            auto count = count_.load();
            while (!count_.compare_exchange_strong(count, 0))
                count = count_.load();

            printf("count: %u\n", count);
            fflush(stdout);
        }
        Pa_CloseStream(stream_);
    }

    static PitchDetector* instance;
};

