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
#include <cmath>
#include <cstdio>
#include <cstdlib>

using namespace std;

array<char const*, 12> constexpr octaveNames = {
    "A", "A#", "B", "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#"
};    

/// Prints musical note of given parameters.
///
/// @param octave         the octave to play in, such as 1 for A1, or 4 for A4
/// @param semitoneOffset the semitone above the A, where 0=A, 1=A#, 2=B, 3=C, 4=C# and so on.
/// @param concertA       the concert-A (A4) to base all other musical notes on.
void printMusicalNote(unsigned octave, unsigned semitoneOffset, float concertA)
{
    float const freq = concertA * pow(2.0f, -4 + static_cast<int>(octave) + semitoneOffset / static_cast<float>(octaveNames.size()));
    printf("%d: %-2s %8.2f\n", octave, octaveNames[semitoneOffset % octaveNames.size()], freq);
}

int main(int argc, char* argv[])
{
    int const octaveBeg = argc >= 2 ? atoi(argv[1]) : 2;
    int const octaveEnd = argc >= 3 ? atoi(argv[2]) : octaveBeg;
    float const referenceA4 = argc == 4 ? atof(argv[3]) : 440.0f;

    for (int octave = octaveBeg; octave <= octaveEnd; ++octave)
        for (unsigned i = 0; i < octaveNames.size(); ++i)
            printMusicalNote(octave, i, referenceA4);

    printMusicalNote(octaveEnd + 1, 0, referenceA4);

    return EXIT_SUCCESS;
}
