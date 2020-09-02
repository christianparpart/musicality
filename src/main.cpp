/**
 * This file is part of the "musicality" project
 *   Copyright (c) 2020 Christian Parpart <christian@parpart.family>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <array>
#include <atomic>
#include <cmath>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <tuple>
#include <vector>

#include <portaudio.h>

// https://codereview.stackexchange.com/questions/148139/pitch-detection-library-basic-architecture?newreg=35d0275e30a94b1a973a6058ba1b88ac

using namespace std;

array<char const*, 12> constexpr octaveNames = {
    "A", "A#", "B", "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#"
};

using Octave = int;
using Semitone = int;
using Percentile = float; // -100.0 < percentile < +100.0

inline tuple<Octave, Semitone, Percentile> freq2semitone(float freq, float concertA)
{
    // TODO: this isn't quite working yet.
    float const semitoneF = octaveNames.size() * log2f(freq / concertA);
    Percentile const percentile = static_cast<float>(static_cast<int>(semitoneF * 100.0f) % 100) / 100.f;
    Octave const octave = log2f(freq / concertA) + 4;
    Semitone semitone = static_cast<Semitone>(semitoneF);

    // if (semitone < 0)
    // {
    //     // -2  -> 10 = 12 - 2
    //     // -11 ->  1 = 12 - 11
    //     // -12 ->  0 = 12 - 12
    //     // -13 -> 11 = 24 - 13
    //     // -14 -> 10 = 24 - 14 = 1 + int(semitoneF / 12) * 12
    //     semitone = int(1.0 - semitone / 12.0) * 12 - semitone;
    // }

    if (percentile < 0.5f)
        return {octave, semitone, percentile};
    else if (semitone < 12)
        return {octave, semitone + 1, -(1.0f - percentile) * 100.0f};
    else
        return {octave + 1, 0, 1.0f -(1.0f - percentile) * 100.0f};
}

void printNoteFromFreqency(float freq, float concertA)
{
    auto const [octave, semitone, percentile] = freq2semitone(freq, concertA);

    printf("Frequency %.2f: octave %d (%d) %-2s %+6.1f cents\n",
           freq,
           octave,
           semitone,
           octaveNames[abs(semitone) % 12],
           percentile
    );
}

/// Prints musical note of given parameters.
///
/// @param octave         the octave to play in, such as 1 for A1, or 4 for A4
/// @param semitoneOffset the semitone above the A, where 0=A, 1=A#, 2=B, 3=C, 4=C# and so on.
/// @param concertA       the concert-A (A4) to base all other musical notes on.
void printMusicalNote(unsigned octave, unsigned semitoneOffset, float concertA)
{
    float const freq = concertA * pow(2.0f, -4.0f + static_cast<float>(octave) + static_cast<float>(semitoneOffset) / static_cast<float>(octaveNames.size()));
    printf("%d: %-2s %8f (relative semitone: %d)\n",
           octave,
           octaveNames[semitoneOffset % octaveNames.size()],
           freq,
           get<1>(freq2semitone(freq, concertA))
    );
}

int old_main(int argc, char* argv[]) // {{{
{
    if (argc >= 3 && strcmp(argv[1], "-f") == 0)
    {
        float const freq = static_cast<float>(atof(argv[2]));
        float const concertA = argc == 4 ? static_cast<float>(atof(argv[3])) : 440.0f;

        printNoteFromFreqency(freq, concertA);
    }
    else
    {
        int const octaveBeg = argc >= 2 ? atoi(argv[1]) : 2;
        int const octaveEnd = argc >= 3 ? atoi(argv[2]) : octaveBeg;
        float const referenceA4 = argc == 4 ? static_cast<float>(atof(argv[3])) : 440.0f;

        for (int octave = octaveBeg; octave <= octaveEnd; ++octave)
            for (unsigned i = 0; i < octaveNames.size(); ++i)
                printMusicalNote(octave, i, referenceA4);

        printMusicalNote(octaveEnd + 1, 0, referenceA4);
    }

    return EXIT_SUCCESS;
} // }}}

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

PitchDetector* PitchDetector::instance = nullptr;

void signalHandler(int _signo)
{
    signal(_signo, SIG_DFL);
    printf("\nsignal caught: %d (%s)\n", _signo, strsignal(_signo));
    PitchDetector::instance->terminating = true;
}

int main(int argc, char* argv[])
{
    (void) argc;
    (void) argv;

    signal(SIGINT, &signalHandler);

    PitchDetector pd;
    pd.run();

    return 0;
}
