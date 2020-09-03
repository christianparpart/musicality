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

#include "PitchDetector.h"

// https://codereview.stackexchange.com/questions/148139/pitch-detection-library-basic-architecture?newreg=35d0275e30a94b1a973a6058ba1b88ac

using namespace std;

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
