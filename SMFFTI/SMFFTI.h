#pragma once

/*

SMFFTI - Simple MIDI Files From Text Input

See the User Manual for everything you need to know about using it.

v0.45	December 5, 2023
(1) Allow text on same line after comment block start, eg. "(# hello"; similarly 
allow text before comment block end, eg. "goodbye #)".
(2) Fix: +AutoChordsMinorChordBias and +AutoChordsMinorChordBias were not 
actually working! _vAutoChordsMinorChordBias & _vAutoChordsMajorChordBias 
needed to be cleared if you specify custom values in your command file.
(3) Modified a few parameter default values.
(4) Set Parameter (-p) feature.
(5) Demarcation of parameter data and music data. That is, music data (ie. chord
progression info) must come at the END of the file.
(6) Fix to +AllMelodyNotes: When set, +AutoMelody, +RandNoteStartOffet and
+RandNoteEndOffset all disabled.
(7) Fix to +TransposeThreshold: The root note also transposed down, and transposition
also occurs when +AutoMelody is set.
(8) RCR Change: Now it checks when chosen chords have been tried (and rejected)
using the history lines. To make this work, also implemented concept of "state"
values that can be updated in the SMFFTI command file. These are known as
System Parameters (eg. +SYS_RCRHistoryCount) and are not intended to be manually
updated by the user.

v0.44	September 6, 2023
T2O4GU Facility to read MIDI file and convert chord data to SMFFTI command-line
format (ConvertMIDIToSMFFTI).
T2P7E7 SMFFTI Command File Wizard. Console UI handling, implemented to provide
easy generation of basic SMFFTI command file.

v0.43	July 11, 2023
T2015A For both Auto-Chords (-ac mode) and Random Chord Replacement (RCR), it is
now possible to specify Modal Interchange (MI). For example, if the main key is 
C Minor, then MI can choose chords from the corresponding C Major key.
New parameter +ModalInterchangeChancePercentage has been added, which can be set
in range 0 - 100. Zero means no MI; 100 means *always* use chords from the opposite
key.

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
(-grm) mode to create a bunch of randomized melodies. (4) "M:" melody lines in command files for 
specifying fixed melodies.


*/
#include "resource.h"
#include "CMIDIHandler.h"
#include "CMyUI.h"

void DoStuff (int argc, char* argv[]);

void PrintUsage();
void PrintError (std::string sMsg);
