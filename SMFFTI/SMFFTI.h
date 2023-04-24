#pragma once

/*

SMFFTI - Simple MIDI Files From Text Input

See the User Guide for everything you need to know about using it.

v0.42	April 24, 2023
Added parameter +AutoMelodyDontUsePentatonic which, if set to 1, tells SMFFTI
to NOT use pentatonic scale notes for Auto-Melody. Also fixed bug for RCR that
failed to properly update the original command file with commented-out
previous chord progressions.

v0.41	April 6, 2023
Random Chord Replacement (RCR). Leverages some of the Auto-Chords processing
to allow individual chords in a progression to be randomly replaced, using the 
question mark (?) and a placeholder. Enable using ew command parameter 
+RandomChordReplacementKey.

v0.4	March 24, 2023
Auto-chords. Main routine for this is CMIDIHandler::CopyFileWithAutoChords.
New classes CChordBank and CAutoRhythm.

v0.31	October 24, 2022
Added command file parameter +AllMelodyNotes

v0.3	October 17, 2022
(1) +RootNoteOnly option added.
(2) Three new chord types: Diminished, Diminished 7th, Half-diminished.
(3) Auto-melody: For major/minor chords, biasing of note choice: Firstly root,
then fifth, then third, and finally the other two pentatonic notes. For the
suspended/diminished chords, pentatonic notes are NOT used, but biasing is applied.

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
