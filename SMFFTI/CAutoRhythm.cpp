#include "pch.h"
#include "CAutoRhythm.h"

CAutoRhythm::CAutoRhythm(uint32_t nShortNotePrefPercent)
{
	_eng.seed (_rdev());

	/*
	Note lengths of 16 (32nds) and less are considered *short* notes, while longer
	ones are considered *long* notes. nShortNotePrefPercent specifies the level of
	short note bias; it must be in the range 0 - 100. If 0, there will be no short
	notes. If 100, there be be _only_ short notes.
	*/

	if (nShortNotePrefPercent > 100)
		nShortNotePrefPercent = 100;

	std::vector<uint32_t> vShorterNotes;
	vShorterNotes.push_back (4);
	vShorterNotes.push_back (5);
	vShorterNotes.push_back (6);
	vShorterNotes.push_back (7);
	vShorterNotes.push_back (8);
	vShorterNotes.push_back (9);
	vShorterNotes.push_back (10);
	vShorterNotes.push_back (11);
	vShorterNotes.push_back (12);
	vShorterNotes.push_back (13);
	vShorterNotes.push_back (14);
	vShorterNotes.push_back (15);
	vShorterNotes.push_back (16);

	std::vector<uint32_t> vLongerNotes;
	vLongerNotes.push_back (17);
	vLongerNotes.push_back (18);
	vLongerNotes.push_back (19);
	vLongerNotes.push_back (20);
	vLongerNotes.push_back (21);
	vLongerNotes.push_back (22);
	vLongerNotes.push_back (23);
	vLongerNotes.push_back (24);
	vLongerNotes.push_back (25);
	vLongerNotes.push_back (26);
	vLongerNotes.push_back (27);
	vLongerNotes.push_back (28);
	vLongerNotes.push_back (29);
	vLongerNotes.push_back (30);
	vLongerNotes.push_back (31);
	vLongerNotes.push_back (32);

	std::uniform_int_distribution<uint32_t> randShortNoteLen (0, vShorterNotes.size() - 1);
	std::uniform_int_distribution<uint32_t> randLongNoteLen (0, vLongerNotes.size() - 1);

	// Add short note lengths.
	for (uint32_t j = 0; j < nShortNotePrefPercent; j++)
	{
		uint32_t nLen = vShorterNotes[randShortNoteLen (_eng)];
		_vNoteLens.push_back (nLen);
	}

	// Add long note lengths.
	uint32_t nLongNotePrefPercent = 100 - nShortNotePrefPercent;
	for (uint32_t j = 0; j < nLongNotePrefPercent; j++)
	{
		uint32_t nLen = vLongerNotes[randLongNoteLen (_eng)];
		_vNoteLens.push_back (nLen);
	}
}

std::string CAutoRhythm::GetPattern (uint32_t& nNumNotes, uint32_t nPatternLen)
{
	std::string sPattern (nPatternLen, ' ');

	std::uniform_int_distribution<uint32_t> randNoteLen (0, _vNoteLens.size() - 1);

	// Init index of sPattern.
	uint32_t i = 0;

	uint32_t nCharsLeft = sPattern.size();

	uint32_t nNoteCount = 0;

	// Loop over the sFourBar string
	while (i < sPattern.size())
	{
		// Get random note length.
		uint32_t nLen = _vNoteLens[randNoteLen (_eng)];

		if (nLen > nCharsLeft)
			nLen = nCharsLeft;

		// Update the output string.
		for (uint32_t n = 0; n < nLen; n++)
			sPattern[i++] = n == 0 ? '+' : '#';

		nNoteCount++;

		// Ensure we are note-aligned. The greater number of 1/4 notes
		// tends toward more gaps, which I prefer.
		std::vector<uint8_t> vNoteAlign;
		vNoteAlign.push_back (4);	// 1/8ths
		vNoteAlign.push_back (4);	// 1/8ths
		vNoteAlign.push_back (4);	// 1/8ths
		vNoteAlign.push_back (4);	// 1/8ths
		vNoteAlign.push_back (8);	// 1/4 notes
		vNoteAlign.push_back (8);	// 1/4 notes
		vNoteAlign.push_back (8);	// 1/4 notes
		vNoteAlign.push_back (8);	// 1/4 notes
		vNoteAlign.push_back (8);	// 1/4 notes
		vNoteAlign.push_back (8);	// 1/4 notes
		vNoteAlign.push_back (16);	// 1/2 notes
		std::uniform_int_distribution<uint32_t> randNoteAlign (0, vNoteAlign.size() - 1);
		uint32_t noteAlign = vNoteAlign[randNoteAlign (_eng)];
		uint32_t j = i % noteAlign;
		if (j != 0)
			i = i - j + noteAlign;	// Advance to next note alignment.

		nCharsLeft = nPatternLen - i;
	}

	nNumNotes = nNoteCount;

	return sPattern;
}

// Randomizer static variable declaration
std::default_random_engine CAutoRhythm::_eng;
