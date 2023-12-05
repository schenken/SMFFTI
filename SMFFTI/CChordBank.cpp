#include "pch.h"
#include "CChordBank.h"
#include "CMIDIHandler.h"

// Constructor that builds everything.
CChordBank::CChordBank (const std::string& sNote, const std::vector<uint32_t>& ctv)
: _sKey (sNote)
{
	_eng.seed (_rdev());

	auto it = std::find (_vChromaticScale.begin(), _vChromaticScale.end(), _sKey);
	_iChord = std::distance (_vChromaticScale.begin(), it);

	BuildMajorChordVariations (ctv);
	BuildMinorChordVariations (ctv);
	BuildDimChordVariations (ctv);
}

void CChordBank::BuildMinor (uint8_t nRootPercent, uint8_t nOtherMinorPercent, uint8_t nMajorPercent)
{
	// Minor key intervals: T, S, T, T, S, T, T
	//
	// So what we do is build the list according to the percentages specified.
	// If the total percentage is less than 100, the remaining percentage is alloted to the dim chord.
	// If the total percentage is greater than 100, an error value is returned.

	uint8_t iChordCount = 0;

	// Root chord
	for (uint32_t i = 0; i < nRootPercent; i++)
	{
		_vChords.push_back (Chord (_vChromaticScale[_iChord], ChordType::Minor));
		iChordCount++;
		_nMinorKey_RootChord_Multiplier = 1;
	}

	// Other minor chords
	for (uint32_t i = 0; i < nOtherMinorPercent; i++)
	{
		if (i % 2 == 0)
			_vChords.push_back (Chord (_vChromaticScale[_iChord + 5], ChordType::Minor));
		else
			_vChords.push_back (Chord (_vChromaticScale[_iChord + 7], ChordType::Minor));

		iChordCount++;
		_nMinorKey_TwoOtherMinorChords_Multiplier = 2;
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
		_nMinorKey_ThreeMajorChords_Multiplier = 3;
	}

	// Diminished chords
	while (iChordCount < 100)
	{
		_vChords.push_back (Chord (_vChromaticScale[_iChord + 2], ChordType::Diminished));
		iChordCount++;
		_nMinorKey_DimChord_Multiplier = 1;
	}

	_nMinorKey_TotalChordsAvailable = 
		(_nMinorKey_RootChord_Multiplier * _nNumMinChordVars) +
		(_nMinorKey_TwoOtherMinorChords_Multiplier * _nNumMinChordVars) +
		(_nMinorKey_ThreeMajorChords_Multiplier * _nNumMajChordVars) +
		(_nMinorKey_DimChord_Multiplier * _nNumDimChordVars);

	ASSERT (_vChords.size() == 100);
}

void CChordBank::BuildMajor (uint8_t nRootPercent, uint8_t nOtherMajorPercent, uint8_t nMinorPercent)
{
	// Major key intervals: T, T, S, T, T, T, S

	uint8_t iChordCount = 0;

	// Root chord
	for (uint32_t i = 0; i < nRootPercent; i++)
	{
		_vChords.push_back (Chord (_vChromaticScale[_iChord], ChordType::Major));
		iChordCount++;
		_nMajorKey_RootChord_Multiplier = 1;
	}

	// Other major chords
	for (uint32_t i = 0; i < nOtherMajorPercent; i++)
	{
		if (i % 2 == 0)
			_vChords.push_back (Chord (_vChromaticScale[_iChord + 5], ChordType::Major));
		else
			_vChords.push_back (Chord (_vChromaticScale[_iChord + 7], ChordType::Major));

		iChordCount++;
		_nMajorKey_TwoOtherMajorChords_Multiplier = 2;
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
		_nMajorKey_ThreeMinorChords_Multiplier = 3;
	}

	// Diminished chords
	while (iChordCount < 100)
	{
		_vChords.push_back (Chord (_vChromaticScale[_iChord + 11], ChordType::Diminished));
		iChordCount++;
		_nMajorKey_DimChord_Multiplier = 1;
	}

	_nMajorKey_TotalChordsAvailable = 
		(_nMajorKey_RootChord_Multiplier * _nNumMajChordVars) +
		(_nMajorKey_TwoOtherMajorChords_Multiplier * _nNumMajChordVars) +
		(_nMajorKey_ThreeMinorChords_Multiplier * _nNumMinChordVars) +
		(_nMajorKey_DimChord_Multiplier * _nNumDimChordVars);

	ASSERT (_vChords.size() == 100);
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

	std::vector<std::string>* pv = &_vMajorChordVariations;

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
		// Won't reach this
		ASSERT (1==0);
		break;
	}

 	std::uniform_int_distribution<uint32_t> randChordVariation (0, pv->size() - 1);
	uint32_t iChordVar = randChordVariation (_eng);
	_chordVariation = (*pv)[iChordVar];
}

int32_t CChordBank::BuildMajorChordVariations (const std::vector<uint32_t>& ctv)
{
	int result = 0;
	uint32_t n;

	std::vector<std::string>& v = _vMajorChordVariations;

	n = ctv[static_cast<uint32_t> (ChordTypeVariation::Major)];
	for (uint32_t i = 0; i < n; i++)
		v.push_back ("");
	_nNumMajChordVars += n ? 1 : 0;

	n = ctv[static_cast<uint32_t> (ChordTypeVariation::Dominant_7th)];
	for (uint32_t i = 0; i < n; i++)
		v.push_back ("7");
	_nNumMajChordVars += n ? 1 : 0;

	n = ctv[static_cast<uint32_t> (ChordTypeVariation::Major_7th)];
	for (uint32_t i = 0; i < n; i++)
		v.push_back ("maj7");
	_nNumMajChordVars += n ? 1 : 0;

	n = ctv[static_cast<uint32_t> (ChordTypeVariation::Dominant_9th)];
	for (uint32_t i = 0; i < n; i++)
		v.push_back ("9");
	_nNumMajChordVars += n ? 1 : 0;

	n = ctv[static_cast<uint32_t> (ChordTypeVariation::Major_9th)];
	for (uint32_t i = 0; i < n; i++)
		v.push_back ("maj9");
	_nNumMajChordVars += n ? 1 : 0;

	n = ctv[static_cast<uint32_t> (ChordTypeVariation::Add_9)];
	for (uint32_t i = 0; i < n; i++)
		v.push_back ("add9");
	_nNumMajChordVars += n ? 1 : 0;

	n = ctv[static_cast<uint32_t> (ChordTypeVariation::Sus_2)];
	for (uint32_t i = 0; i < n; i++)
		v.push_back ("sus2");
	_nNumMajChordVars += n ? 1 : 0;

	n = ctv[static_cast<uint32_t> (ChordTypeVariation::_7_Sus_2)];
	for (uint32_t i = 0; i < n; i++)
		v.push_back ("7sus2");
	_nNumMajChordVars += n ? 1 : 0;

	n = ctv[static_cast<uint32_t> (ChordTypeVariation::Sus_4)];
	for (uint32_t i = 0; i < n; i++)
		v.push_back ("sus4");
	_nNumMajChordVars += n ? 1 : 0;

	n = ctv[static_cast<uint32_t> (ChordTypeVariation::_7_Sus_4)];
	for (uint32_t i = 0; i < n; i++)
		v.push_back ("7sus4");
	_nNumMajChordVars += n ? 1 : 0;

	// If nothing specified, ensure at least one entry (basic major type)
	if (v.size() == 0)
	{
		v.push_back ("");
		_nNumMajChordVars = 1;
	}

	return result;
}

int32_t CChordBank::BuildMinorChordVariations (const std::vector<uint32_t>& ctv)
{
	int result = 0;
	uint32_t n;

	std::vector<std::string>& v = _vMinorChordVariations;

	n = ctv[static_cast<uint32_t> (ChordTypeVariation::Minor)];
	for (uint32_t i = 0; i < n; i++)
		v.push_back ("m");
	_nNumMinChordVars += n ? 1 : 0;

	n = ctv[static_cast<uint32_t> (ChordTypeVariation::Minor_7th)];
	for (uint32_t i = 0; i < n; i++)
		v.push_back ("m7");
	_nNumMinChordVars += n ? 1 : 0;

	n = ctv[static_cast<uint32_t> (ChordTypeVariation::Minor_9th)];
	for (uint32_t i = 0; i < n; i++)
		v.push_back ("m9");
	_nNumMinChordVars += n ? 1 : 0;

	n = ctv[static_cast<uint32_t> (ChordTypeVariation::Minor_Add_9)];
	for (uint32_t i = 0; i < n; i++)
		v.push_back ("madd9");
	_nNumMinChordVars += n ? 1 : 0;

	// If nothing specified, ensure at least one entry (basic minor type)
	if (v.size() == 0)
	{
		v.push_back ("m");
		_nNumMinChordVars = 1;
	}

	return result;
}

int32_t CChordBank::BuildDimChordVariations (const std::vector<uint32_t>& ctv)
{
	int result = 0;
	uint32_t n;

	std::vector<std::string>& v = _vDimChordVariations;

	n = ctv[static_cast<uint32_t> (ChordTypeVariation::Dim)];
	for (uint32_t i = 0; i < n; i++)
		v.push_back ("dim");
	_nNumDimChordVars += n ? 1 : 0;

	n = ctv[static_cast<uint32_t> (ChordTypeVariation::Dim_7th)];
	for (uint32_t i = 0; i < n; i++)
		v.push_back ("dim7");
	_nNumDimChordVars += n ? 1 : 0;

	n = ctv[static_cast<uint32_t> (ChordTypeVariation::HalfDim)];
	for (uint32_t i = 0; i < n; i++)
		v.push_back ("m7b5");
	_nNumDimChordVars += n ? 1 : 0;

	// If nothing specified, ensure at least one entry (basic dim)
	if (v.size() == 0)
	{
		v.push_back ("dim");
		_nNumDimChordVars = 1;
	}

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
