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

	CChordBank (std::string sNote);

	// Construct list (vector) of major, minor and diminished chords for the key.
	// These lists constitute the pool of chords from which progressions are randomly
	// created. Biased according to the percentages specified in the arguments.
	//
	int32_t BuildMinor (uint8_t nRootPercent, uint8_t nOtherMinorPercent, uint8_t nMajorPercent);
	int32_t BuildMajor (uint8_t nRootPercent, uint8_t nOtherMajorPercent, uint8_t nMinorPercent);

	// Random chord selector.
	void SetRandomChord();

	std::string GetChordName() { return _chord; }
	ChordType GetChordType() { return _chordType; }

	std::string GetChordVariation() { return _chordVariation; }

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

	uint32_t _iChordVarAmount = 20;	// Percentage, so range 0 - 100.
	std::vector<bool> _vChordVarChance;

	int32_t BuildMajorChordVariations();
	int32_t BuildMinorChordVariations();
	int32_t BuildDimChordVariations();

	//------------------------------------------------------------------------------------------
	// Static class members

	// Inner class hack for initializing class members.
	static struct ClassMemberInit { ClassMemberInit(); } cmi;

	static std::vector<std::string> _vChromaticScale;

	static std::default_random_engine _eng;
};

