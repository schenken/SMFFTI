#pragma once

/*

SMFFTI - Simple MIDI Files From Text Input

See the User Guide for everything you need to know about using it.


v0.2	July 30, 2022
Features added to the initial version: (1) Auto-Rhythm (-ar) mode to create randomized rhythms.
(2) +AutoMelody command parameter to generate randomized melody. (3) Generate Randomized Melodies
(-grm) mode to create a bunch of randomized melodies. (4) "M:" melody lines in commend files for 
specifying fixed melodies.


*/
#include "resource.h"
#include "CMIDIHandler.h"

void DoStuff (int argc, char* argv[]);

void PrintUsage();
void PrintError (std::string sMsg);
