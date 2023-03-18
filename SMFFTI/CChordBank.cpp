#include "pch.h"
#include "CChordBank.h"

// Constructor that builds everything.
CChordBank::CChordBank(std::string sNote)
: _sKey (sNote)
{
	_eng.seed (_rdev());

	auto it = std::find (_vChromaticScale.begin(), _vChromaticScale.end(), _sKey);
	_iChord = std::distance (_vChromaticScale.begin(), it);

	BuildMajorChordVariations();
	BuildMinorChordVariations();
	BuildDimChordVariations();

	_vChordVarChance.assign (100, false);
	for (uint8_t i = 0; i < _iChordVarAmount; i++)
		_vChordVarChance[i] = true;

	int ak = 1;
}

int CChordBank::BuildMinor (uint8_t nRootPercent, uint8_t nOtherMinorPercent, uint8_t nMajorPercent)
{
	// Minor key intervals: T, S, T, T, S, T, T
	//
	// So what we do is build the list according to the percentages specified.
	// If the total percentage is less than 100, the remaining percentage is alloted to the dim chord.
	// If the total percentage is greater than 100, an error value is returned.

	int result = 1;

	// Too many percent.
	if (nRootPercent + nOtherMinorPercent + nMajorPercent > 100)
		return 0;

	uint8_t iChordCount = 0;

	// Root chord
	for (uint32_t i = 0; i < nRootPercent; i++)
	{
		_vChords.push_back (Chord (_vChromaticScale[_iChord], ChordType::Minor));
		iChordCount++;
	}

	// Other minor chords
	for (uint32_t i = 0; i < nOtherMinorPercent; i++)
	{
		if (i % 2 == 0)
			_vChords.push_back (Chord (_vChromaticScale[_iChord + 5], ChordType::Minor));
		else
			_vChords.push_back (Chord (_vChromaticScale[_iChord + 7], ChordType::Minor));

		iChordCount++;
	}

	// Major chords
	for (uint32_t i = 0; i < nMajorPercent; i++)
	{
		if (i % 3 == 0)
			_vChords.push_back (Chord (_vChromaticScale[_iChord + 3], ChordType::Major));
		else if (i % 3 == 1)
			_vChords.push_back (Chord (_vChromaticScale[_iChord + 8], ChordType::Major));
		else if (i % 3 == 2)
			_vChords.push_back (Chord (_vChromaticScale[_iChord + 10], ChordType::Major));

		iChordCount++;
	}

	// Diminished chords
	while (iChordCount < 100)
	{
		_vChords.push_back (Chord (_vChromaticScale[_iChord + 2], ChordType::Diminished));
		iChordCount++;
	}

	ASSERT (_vChords.size() == 100);

	return result;
}

int CChordBank::BuildMajor (uint8_t nRootPercent, uint8_t nOtherMajorPercent, uint8_t nMinorPercent)
{
	// Major key intervals: T, T, S, T, T, T, S

	int result = 1;

	// Too many percent.
	if (nRootPercent + nOtherMajorPercent + nMinorPercent > 100)
		return 0;

	uint8_t iChordCount = 0;

	// Root chord
	for (uint32_t i = 0; i < nRootPercent; i++)
	{
		_vChords.push_back (Chord (_vChromaticScale[_iChord], ChordType::Major));
		iChordCount++;
	}

	// Other major chords
	for (uint32_t i = 0; i < nOtherMajorPercent; i++)
	{
		if (i % 2 == 0)
			_vChords.push_back (Chord (_vChromaticScale[_iChord + 5], ChordType::Major));
		else
			_vChords.push_back (Chord (_vChromaticScale[_iChord + 7], ChordType::Major));

		iChordCount++;
	}

	// Minor chords
	for (uint32_t i = 0; i < nMinorPercent; i++)
	{
		if (i % 3 == 0)
			_vChords.push_back (Chord (_vChromaticScale[_iChord + 2], ChordType::Minor));
		else if (i % 3 == 1)
			_vChords.push_back (Chord (_vChromaticScale[_iChord + 4], ChordType::Minor));
		else if (i % 3 == 2)
			_vChords.push_back (Chord (_vChromaticScale[_iChord + 9], ChordType::Minor));

		iChordCount++;
	}

	// Diminished chords
	while (iChordCount < 100)
	{
		_vChords.push_back (Chord (_vChromaticScale[_iChord + 11], ChordType::Diminished));
		iChordCount++;
	}

	ASSERT (_vChords.size() == 100);

	return result;
}

void CChordBank::SetRandomChord()
{
	// Init randomizer for choosing chords.
	std::uniform_int_distribution<uint32_t> randChord (0, _vChords.size() - 1);

	_iRandChord = randChord (_eng);
	Chord chord = _vChords[_iRandChord];

	// Save the crucial items
	_chord = chord.sNote;
	_chordType = chord.iChordType;

	std::vector<std::string>* pv;

	// Init randomizer for chord variations.
	switch (_chordType)
	{
	case ChordType::Major:
		pv = &_vMajorChordVariations;
		break;
	case ChordType::Minor:
		pv = &_vMinorChordVariations;
		break;
	case ChordType::Diminished:
		pv = &_vDimChordVariations;
		break;
	default:
		pv = &_vMajorChordVariations;
		break;
	}

	// See if we should get a chord variation:
	// iChordVarChance represents the chance percentage of using a chord variation.
	std::uniform_int_distribution<uint32_t> rand (0, 99);
	uint32_t iChordVarChance = rand (_eng);
	if (!_vChordVarChance[iChordVarChance])
	{
		// No, so use basic chord.
		// NB. For dim chords, even if no variation specified, we will still
		// always use a dim7, since it sounds better.
		_chordVariation = (*pv)[0];
		return;
	}

	std::uniform_int_distribution<uint32_t> randChordVariation (0, pv->size() - 1);
	uint32_t iChordVar = randChordVariation (_eng);
	_chordVariation = (*pv)[iChordVar];
}

int32_t CChordBank::BuildMajorChordVariations()
{
	int result = 0;

	std::vector<std::string>& v = _vMajorChordVariations;

	v.push_back ("");
	v.push_back ("");
	v.push_back ("");
	v.push_back ("");

	v.push_back ("7");
	v.push_back ("7");
	v.push_back ("maj7");
	v.push_back ("maj7");
	v.push_back ("maj7");
	v.push_back ("maj7");
	v.push_back ("9");
	v.push_back ("9");
	v.push_back ("maj9");
	v.push_back ("maj9");

	v.push_back ("add9");
	v.push_back ("add9");
	v.push_back ("add9");
	v.push_back ("add9");
	v.push_back ("add9");
	v.push_back ("add9");
	v.push_back ("add9");
	v.push_back ("add9");
	v.push_back ("add9");
	v.push_back ("add9");
	v.push_back ("add9");
	v.push_back ("add9");
	v.push_back ("add9");
	v.push_back ("add9");
	v.push_back ("add9");
	v.push_back ("add9");
	v.push_back ("add9");
	v.push_back ("add9");
	v.push_back ("add9");
	v.push_back ("add9");
	v.push_back ("add9");
	v.push_back ("add9");
	v.push_back ("add9");
	v.push_back ("add9");
	v.push_back ("add9");
	v.push_back ("add9");

	// Sus chords are quite nice
	v.push_back ("sus2");
	v.push_back ("7sus2");
	v.push_back ("sus4");
	v.push_back ("7sus4");

	// Modal interchange - oooh! eg. C becomes Cm.
	v.push_back ("m");
	v.push_back ("m");

	return result;
}

int32_t CChordBank::BuildMinorChordVariations()
{
	int result = 0;

	std::vector<std::string>& v = _vMinorChordVariations;

	v.push_back ("m");
	v.push_back ("m");
	v.push_back ("m");
	v.push_back ("m");

	v.push_back ("m7");
	v.push_back ("m7");
	v.push_back ("m7");
	v.push_back ("m7");
	v.push_back ("m7");
	v.push_back ("m7");
	v.push_back ("m7");
	v.push_back ("m7");
	v.push_back ("m7");
	v.push_back ("m7");
	v.push_back ("m7");
	v.push_back ("m7");
	v.push_back ("m7");
	v.push_back ("m7");
	v.push_back ("m7");
	v.push_back ("m7");
	v.push_back ("m7");
	v.push_back ("m7");
	v.push_back ("m7");
	v.push_back ("m7");
	v.push_back ("m7");
	v.push_back ("m7");
	v.push_back ("m7");
	v.push_back ("m7");

	v.push_back ("9");
	v.push_back ("m9");

	v.push_back ("madd9");
	v.push_back ("madd9");

	// Sus chords are quite nice
	v.push_back ("sus2");
	v.push_back ("7sus2");
	v.push_back ("sus4");
	v.push_back ("7sus4");

	// Modal interchange - oooh! eg. Cm becomes C.
	v.push_back ("");
	v.push_back ("");

	return result;
}

int32_t CChordBank::BuildDimChordVariations()
{
	int result = 0;

	std::vector<std::string>& v = _vDimChordVariations;

	v.push_back ("dim7");
	v.push_back ("m7b5");
	// NB. We never play a straight dim - always a dim7 or m7b5.

	return result;
}

//-----------------------------------------------------------------------------
// Static class members

std::vector<std::string>CChordBank::_vChromaticScale;

CChordBank::ClassMemberInit CChordBank::cmi;

CChordBank::ClassMemberInit::ClassMemberInit ()
{
	_vChromaticScale.push_back ("C");
	_vChromaticScale.push_back ("Db");
	_vChromaticScale.push_back ("D");
	_vChromaticScale.push_back ("Eb");
	_vChromaticScale.push_back ("E");
	_vChromaticScale.push_back ("F");
	_vChromaticScale.push_back ("Gb");
	_vChromaticScale.push_back ("G");
	_vChromaticScale.push_back ("Ab");
	_vChromaticScale.push_back ("A");
	_vChromaticScale.push_back ("Bb");
	_vChromaticScale.push_back ("B");
	_vChromaticScale.push_back ("C");
	_vChromaticScale.push_back ("Db");
	_vChromaticScale.push_back ("D");
	_vChromaticScale.push_back ("Eb");
	_vChromaticScale.push_back ("E");
	_vChromaticScale.push_back ("F");
	_vChromaticScale.push_back ("Gb");
	_vChromaticScale.push_back ("G");
	_vChromaticScale.push_back ("Ab");
	_vChromaticScale.push_back ("A");
	_vChromaticScale.push_back ("Bb");
	_vChromaticScale.push_back ("B");
}

// Randomizer static variable declaration
std::default_random_engine CChordBank::_eng;
