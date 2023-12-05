#pragma once

/*
17/3/23 Encapsulates functionality relating to chord selection. You tell it which key
you are using and it build a list of major, minor and diminished chords that can be used
in a chord progression.
*/

enum class ChordType : uint8_t
{
	Major, Minor, Diminished
};

class CChordBank
{
public:

	CChordBank (const std::string& sNote, const std::vector<uint32_t>& ctv);

	// Construct list (vector) of major, minor and diminished chords for the key.
	// These lists constitute the pool of chords from which progressions are randomly
	// created. Biased according to the percentages specified in the arguments.
	//
	void BuildMinor (uint8_t nRootPercent, uint8_t nOtherMinorPercent, uint8_t nMajorPercent);
	void BuildMajor (uint8_t nRootPercent, uint8_t nOtherMajorPercent, uint8_t nMinorPercent);

	// Random chord selector.
	void SetRandomChord();

	std::string GetChordName() { return _chord; }
	ChordType GetChordType() { return _chordType; }

	std::string GetChordVariation() { return _chordVariation; }

	const uint32_t GetTotalChordsAvailableForMinorKey()
	{
		return _nMinorKey_TotalChordsAvailable;
	}
	const uint32_t GetTotalChordsAvailableForMajorKey()
	{
		return _nMajorKey_TotalChordsAvailable;
	}

protected:

	std::string _sKey;
	bool _bMinorKey;
	uint8_t _iChord;

	struct Chord
	{
		std::string sNote;
		ChordType iChordType; // 0=Major, 1=Minor, 2=Diminished

		Chord (std::string n, ChordType ct) : sNote (n), iChordType (ct) {}
	};

	std::vector<Chord> _vChords;

	std::vector<std::string> _vMajorChordVariations;
	std::vector<std::string> _vMinorChordVariations;
	std::vector<std::string> _vDimChordVariations;

	// Randomizer
	std::random_device _rdev;

	uint8_t _iRandChord = 127;
	std::string _chord;
	ChordType _chordType;
	std::string _chordVariation;

	int32_t BuildMajorChordVariations (const std::vector<uint32_t>& ctv);
	int32_t BuildMinorChordVariations (const std::vector<uint32_t>& ctv);
	int32_t BuildDimChordVariations (const std::vector<uint32_t>& ctv);
	
	// These variables all pertain specificially to RCR.
	// Don't know how to describe these. They are used to
	// determine the total number of chords that can be
	// randomly selected using RCR.
	uint32_t _nMinorKey_RootChord_Multiplier = 0;				// 0 or 1 (there is only 1 root chord)
	uint32_t _nMinorKey_TwoOtherMinorChords_Multiplier = 0;		// 0 or 2
	uint32_t _nMinorKey_ThreeMajorChords_Multiplier = 0;		// 0 or 3
	uint32_t _nMinorKey_DimChord_Multiplier = 0;				// 0 or 1
	//
	uint32_t _nMajorKey_RootChord_Multiplier = 0;				// 0 or 1
	uint32_t _nMajorKey_TwoOtherMajorChords_Multiplier = 0;		// 0 or 2
	uint32_t _nMajorKey_ThreeMinorChords_Multiplier = 0;		// 0 or 3
	uint32_t _nMajorKey_DimChord_Multiplier = 0;				// 0 or 1
	//
	//
	// The above values are used to multiply their
	// respective values below. These values are then summed
	// to give a total. For example, if we are using Gm as
	// our key, and we are permitting all 7 of its chords
	// to be used, and we are allowing *all* 17 of the chord
	// variations, this means:
	//
	//		 4 variations of Gm
	//		 3     "      "  Adim
	//		10     "      "  Bb
	//		 4     "      "  Cm
	//		 4     "      "  Dm
	//		10     "      "  Eb
	//		10     "      "  F
	//     ---
	//      45
	//
	// This give us a total of 45 possible chords that RCR can choose from.
	// Of course, Auto-chords can also use any of these, but regarding RCR
	// we need to know this number for our processing that prevents a chord
	// from being chosen if it is already in the RCR 'history' list. Specifically,
	// we need to know when the history is 'full', ie. there are no more chords
	// that can be chosen (since the user has rejected them all by invoking RCR
	// multiple times).
	//
	// Number of Major Chord Variations (can be 1 - 10)
	uint32_t _nNumMajChordVars = 0;
	//
	// Number of Minor Chord Variations (can be 1 - 4)
	uint32_t _nNumMinChordVars = 0;
	//
	// Number of Diminished Chord Variations (can be 1 - 3)
	uint32_t _nNumDimChordVars = 0;
	//
	// The totals of the number of chords that are available for
	// selection by Auto-chords and RCR.
	uint32_t _nMinorKey_TotalChordsAvailable = 0;
	uint32_t _nMajorKey_TotalChordsAvailable = 0;
	//

	//------------------------------------------------------------------------------------------
	// Static class members

	// Inner class hack for initializing class members.
	static struct ClassMemberInit { ClassMemberInit(); } cmi;

	static std::vector<std::string> _vChromaticScale;

	static std::default_random_engine _eng;
};

