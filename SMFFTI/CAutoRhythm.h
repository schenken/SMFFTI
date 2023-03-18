#pragma once

/*
18/3/23 Handles rhythm processing for Auto-chord.
*/

class CAutoRhythm
{
public:
	CAutoRhythm (uint32_t nShortNotePrefPercent);

	std::string GetPattern (uint32_t& nNumNotes);

protected:
	std::vector<uint8_t> _vNoteLens;

	// Randomizer
	std::random_device _rdev;

	//------------------------------------------------------------------------------------------
	// Static class members

	// Inner class hack for initializing class members.
	static struct ClassMemberInit { ClassMemberInit(); } cmi;

	static std::vector<std::string> _vChromaticScale;

	static std::default_random_engine _eng;
};

