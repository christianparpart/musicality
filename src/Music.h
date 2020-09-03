#pragma once

#include <array>
#include <tuple>
#include <cmath>
#include <cstdio>

namespace musicality {

std::array<char const*, 12> constexpr octaveNames = {
    "A", "A#", "B", "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#"
};

using Octave = int;
using Semitone = int;
using Percentile = float; // -100.0 < percentile < +100.0

inline std::tuple<Octave, Semitone, Percentile> freq2semitone(float freq, float concertA)
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

}
