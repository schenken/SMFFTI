#include "pch.h"
#include "CMIDIHandler.h"
#include "Common.h"

CMIDIHandler::CMIDIHandler (std::string sInputFile) : _sInputFile (sInputFile)
{
	_ticksPer16th = _ticksPerQtrNote / 4;
	_ticksPer32nd = _ticksPerQtrNote / 8;
	_ticksPerBar = _ticksPerQtrNote * 4;

	_nArpNoteTicks = _ticksPerBar / _nArpTime;

	_eng.seed (_rdev());
}

CMIDIHandler::StatusCode CMIDIHandler::CreateRandomFunkGrooveMIDICommandFile (std::string sOutFile, bool bOverwriteOutFile)
{
	StatusCode result = StatusCode::Success;

	bool bRandomGroove = true;	// dummy value - not used

	_vBarCount.push_back (1);

	if (!bOverwriteOutFile && akl::MyFileExists (sOutFile))
	{
		std::ostringstream ss;
		ss << "Output file already exists. Use the -o switch to overwrite, eg:\n"
			<< "SMFFTI.exe midicmds.txt MyMIDIFile.mid -o";
		_sStatusMessage = ss.str();
		return StatusCode::OutputFileAlreadyExists;
	}

	std::uniform_int_distribution<size_t> randChord (0, _vRFGChords.size() - 1);

	std::vector<std::string> vChords;
	std::vector<std::string> vNotePositions;
	std::string sChord, sPrevChord;
	for (size_t i = 0; i < 8; i++)
	{
		std::string sGroove = GetRandomGroove (bRandomGroove);
		vNotePositions.push_back (sGroove);

		// Randomize chord selection and validate.
		// (Except 1st chord must be Em7.)
		if (i == 0)
			sChord = "Em7";
		else
			sChord = _vRFGChords[randChord (_eng)];

		bool bErr = false;
		while (1)
		{
			// Last chord NOT Em7
			if (!bErr && i == 7 && sChord == "Em7")
				bErr = true;

			// No same chord consecutively.
			if (!bErr && sChord == sPrevChord)
				bErr = true;

			// No sus chords unless the last one.
			if (!bErr)
			{
				size_t pos = sChord.find ("sus");
				if (pos != std::string::npos)
					if (i != 7)
						bErr = true;
			}

			if (!bErr)
				break;
			bErr = false;

			sChord = _vRFGChords[randChord (_eng)];
		}

		sPrevChord = sChord;

		size_t n = std::count (sGroove.begin(), sGroove.end(), '+');
		std::ostringstream ss;
		ss << sChord << "(" << n << ")";
		vChords.push_back (ss.str());
	}

	std::ofstream ofs (sOutFile, std::ios::binary);

	std::ostringstream ss;

	ss <<

		"+TrackName=Random Funk Groove (" << sOutFile << ")\n"
		"+BassNote=0\n"
		"+Velocity=80\n"
		"+RandVelVariation=0\n"
		"+RandNoteStartOffset=0\n"
		"+RandNoteEndOffset=0\n"
		"+RandNoteOffsetTrim=1\n"
		"+NoteStagger=0\n"
		"+OctaveRegister=3\n"
		"+TransposeThreshold=11\n"
		"+Arpeggiator=0\n"
		"+ArpTime=16\n"
		"+ArpGatePercent=100\n"
		"+ArpOctaveSteps=1\n\n"

		"+FunkStrum=2\n"
		"+FunkStrumUpStrokeAttenuation=0.5\n"
		"+FunkStrumVelDeclineIncrement=8\n\n"

		;

		for (size_t i = 0; i < vChords.size(); i++)
		{
			ss <<
				sRuler << "\n"
				<< vNotePositions[i] << "\n"
				<< vChords[i] << "\n\n"
				;
		}

	ofs << ss.str();

	ofs.close();

	return result;
}

CMIDIHandler::StatusCode CMIDIHandler::Verify()
{
	StatusCode result = StatusCode::Success;

	akl::LoadTextFileIntoVector (_sInputFile, _vInput);

	uint8_t nDataLines = 0;
	uint16_t nNumberOfNotes = 0;
	int32_t nVal = 0;
	double ndVal = 0.0;
	uint32_t nLineNum = 0;
	uint32_t nRulerLen = 0;
	bool bCommentBlock = false;
	bool bRandomGroove = false;

	for each (auto sLine in _vInput)
	{
		nLineNum++;

		std::string sTemp = akl::RemoveWhitespace (sLine, 4); // strip all ws

		if (sTemp == "(#")
		{
			bCommentBlock = true;
			continue;
		}

		if (sTemp == "#)")
		{
			bCommentBlock = false;
			continue;
		}

		if (bCommentBlock)
			continue;

		if (sTemp.size() == 0)	// ignore blank lines
			continue;

		if (sTemp[0] == '#')	// ignore comments
			continue;

		if (nDataLines == 1)
		{
			// Expected Note Positions line.
			std::string sNotePositions = akl::RemoveWhitespace (sLine, 2);	// strip trailing spaces

			if (sNotePositions.size() == 0)
			{
				std::ostringstream ss;
				ss << "Blank Note Positions at line " << nLineNum;
				_sStatusMessage = ss.str();
				return StatusCode::BlankNotePositions;
			}

			// RandomGroove works for all parameters, but is designed
			// primarily for +FunkStrum.
			if (sNotePositions == "RandomGroove")
				sNotePositions = GetRandomGroove (bRandomGroove);

			// Check we've only got spaces, pluses or hashes.
			bool res = std::all_of (sNotePositions.begin(), sNotePositions.end(),
				[](char c) { return c == ' ' || c == '+' || c == '#'; });

			if (res == false) {
				std::ostringstream ss;
				ss << "Illegal character in Note Positions line " << nLineNum << ".\n"
					<< "Valid characters: +, # and space.";
				_sStatusMessage = ss.str();
				return StatusCode::IllegalCharInNotePositionsLine;
			}

			// First non-space char must be a +.
			if (akl::RemoveWhitespace (sNotePositions, 1)[0] != '+')
			{
				std::ostringstream ss;
				ss << "Line " << nLineNum << ": First non-space character must be a plus (+).";
				_sStatusMessage = ss.str();
				return StatusCode::FirstNonSpaceCharInNotePosLineMustBePlus;
			}

			// Must not exceed length of ruler line
			if (sNotePositions.size() > nRulerLen)
			{
				std::ostringstream ss;
				ss << "Line " << nLineNum << ": Too long - must not exceed length of ruler line.";
				_sStatusMessage = ss.str();
				return StatusCode::NotePositionLineTooLong;
			}

			_vNotePositions.push_back (sNotePositions);
			_vNotePosLineInFile.push_back (nLineNum);

			_vMelodyNotes.push_back ("");

			nNumberOfNotes += std::count (sNotePositions.begin(), sNotePositions.end(), '+');
			nDataLines++;
			continue;
		}

		if (nDataLines == 2)
		{
			std::vector<std::string> v = akl::Explode (sTemp, ",");
			if (v.size() == 0)
			{
				std::ostringstream ss;
				ss << "Line " << nLineNum << ": Missing chord list.";
				_sStatusMessage = ss.str();
				return StatusCode::MissingChordList;
			}

			if (bRandomGroove)
			{
				// We expect only one chord, and set a repeat value
				// according to the randomly-generated note position
				// string just created.

				std::string sNotePositions = _vNotePositions.back();
				size_t n = std::count (sNotePositions.begin(), sNotePositions.end(), '+');

				std::ostringstream ss;
				ss << v[0] << "(" << n << ")";
				v[0] = ss.str();

				int ak = 1;
			}

			for each (auto c in v)
			{
				std::vector<std::string> vChordIntervals;
				uint8_t nRoot = 0;

				// Lambda func to check for chord repeater, ie. chord names suffixed with a number
				// in parentheses, eg. Cm(3). Where found, the number value is given back.
				// (Max 16 repeats allowed.)
				auto ChordRepeat = [](std::string s, uint8_t& nNum)
				{
					size_t n = s.find ('(');
					size_t n1 = s.find (')');
					if (n != std::string::npos)
						if (n1 != std::string::npos)
							if (n > 0 && n1 == s.size() - 1 && (n1 - n > 1))
								if (std::all_of (s.begin() + n + 1, s.begin() + n1 - 1, ::isdigit))
								{
									int32_t nTemp;
									if (akl::VerifyTextInteger (s.substr (n + 1, (n1 - 1) - n), nTemp, 1, 128))
									{
										nNum = nTemp;
										return true;
									}
								}

					return false;
				};

				uint8_t nNumInstances = 1;
				std::string sChordName = c;

				if (ChordRepeat (c, nNumInstances))
					sChordName = c.substr (0, c.find ('('));

				if (!GetChordIntervals (sChordName, nRoot, vChordIntervals))
				{
					std::ostringstream ss;
					ss << "Line " << nLineNum << ": Invalid/blank chord name: " << sChordName;
					_sStatusMessage = ss.str();
					return StatusCode::InvalidOrBlankChordName;
				}

				for (uint8_t i = 0; i < nNumInstances; i++)
					_vChordNames.push_back (sChordName);
			}

			nDataLines++;
			continue;
		}

		if (nDataLines == 3)
		{
			// Optional melody instead of full chords
			nDataLines = 0;

			if (sTemp.substr (0, 2) == "M:")
			{
				_vMelodyNotes.back() = sTemp;
				continue;
			}
		}

		// Orphan Note Position line.
		if (std::all_of (sTemp.begin(), sTemp.end(), [](char c) { return c == ' ' || c == '+' || c == '#'; }))
		{
			std::ostringstream ss;
			ss << "Line " << nLineNum << ": Stray Note Position line? Should be preceded by a ruler line.";
			_sStatusMessage = ss.str();
			return StatusCode::StrayNotePositionLine;
		}

		if (sTemp[0] == '+')	// parameter
		{
			std::string parameter = sTemp.substr (1, sTemp.size() - 1);

			std::vector<std::string> vKeyValue = akl::Explode (parameter, "=");

			if (vKeyValue.size() < 2)
			{
				std::ostringstream ss;
				ss << "Line " << nLineNum << ": Value not supplied for +" << vKeyValue[0];
				_sStatusMessage = ss.str();
				return StatusCode::ParameterValueMissing;
			}

			std::vector<std::string> vKV2 = akl::Explode (sLine.substr (1, sLine.size() - 1), "=");	// ws not stripped

			if (vKeyValue[0] == "BassNote")
			{
				if (!akl::VerifyTextInteger (vKeyValue[1], nVal, 0, 1))
				{
					_sStatusMessage = "Invalid +BassNote value (valid: 0 or 1).";
					return StatusCode::InvalidBassNoteValue;
				}
				_bAddBassNote = nVal == 1;
				continue;
			}
			else if (vKeyValue[0] == "Velocity")
			{
				if (!akl::VerifyTextInteger (vKeyValue[1], nVal, 1, 127))
				{
					_sStatusMessage = "Invalid +Velocity value (range 1-127).";
					return StatusCode::InvalidVelocityValue;
				}
				_nVelocity = nVal;
			}
			else if (vKeyValue[0] == "RandVelVariation")
			{
				if (!akl::VerifyTextInteger (vKeyValue[1], nVal, 0, 127))
				{
					_sStatusMessage = "Invalid +RandVelVariation value (range 0-127).";
					return StatusCode::InvalidRandomVelocityVariationValue;
				}
				_nRandVelVariation = nVal;
			}
			else if (vKeyValue[0] == "RandNoteStartOffset")
			{
				if (!akl::VerifyTextInteger (vKeyValue[1], nVal, 0, 32))
				{
					_sStatusMessage = "Invalid +RandNoteStartOffset value (range 0-32).";
					return StatusCode::InvalidRandomNoteStartOffsetValue;
				}
				_nRandNoteStartOffset = nVal;
				if (_nRandNoteStartOffset > 0)
					_bRandNoteStart = true;
			}
			else if (vKeyValue[0] == "RandNoteEndOffset")
			{
				if (!akl::VerifyTextInteger (vKeyValue[1], nVal, 0, 32))
				{
					_sStatusMessage = "Invalid +RandNoteEndOffset value (range 0-32).";
					return StatusCode::InvalidRandomNoteEndOffsetValue;
				}
				_nRandNoteEndOffset = nVal;
				if (_nRandNoteEndOffset > 0)
					_bRandNoteEnd = true;
			}
			else if (vKeyValue[0] == "RandNoteOffsetTrim")
			{
				if (!akl::VerifyTextInteger (vKeyValue[1], nVal, 0, 1))
				{
					_sStatusMessage = "Invalid +RandNoteOffsetTrim value (valid: 0 or 1).";
					return StatusCode::InvalidRandomNoteOffsetTrimValue;
				}
				_bRandNoteOffsetTrim = nVal == 1;
			}
			else if (vKeyValue[0] == "NoteStagger")
			{
				if (!akl::VerifyTextInteger (vKeyValue[1], nVal, -32, 32))
				{
					_sStatusMessage = "Invalid +NoteStagger value (range -32 to 32).";
					return StatusCode::InvalidNoteStaggerValue;
				}
				_nNoteStagger = nVal;
			}
			else if (vKeyValue[0] == "OctaveRegister")
			{
				if (!akl::VerifyTextInteger (vKeyValue[1], nVal, 0, 7))
				{
					_sStatusMessage = "Invalid +OctaveRegister value (range 0-7).";
					return StatusCode::InvalidOctaveRegisterValue;
				}
				_sOctaveRegister = vKeyValue[1];
			}
			else if (vKeyValue[0] == "TransposeThreshold")
			{
				if (!akl::VerifyTextInteger (vKeyValue[1], nVal, 0, 48))
				{
					_sStatusMessage = "Invalid +TransposeThreshold value (range 0-48).";
					return StatusCode::InvalidTransposeThresholdValue;
				}
				_nTransposeThreshold = nVal;
			}
			else if (vKeyValue[0] == "Arpeggiator")
			{
				if (!akl::VerifyTextInteger (vKeyValue[1], nVal, 0, 13))
				{
					_sStatusMessage = "Invalid +Arpeggiator value (range 0-13).";
					return StatusCode::InvalidArpeggiatorValue;
				}
				_nArpeggiator = nVal;
			}
			else if (vKeyValue[0] == "ArpTime")
			{
				bool bOK = akl::VerifyTextInteger (vKeyValue[1], nVal, 1, 32);
				if (bOK && !(nVal > 0 && !(nVal & (nVal - 1))))	// must be power of 2 (1, 2, 4, 8, 16 or 32)
					bOK = false;
				if (!bOK)
				{
					_sStatusMessage = "Invalid +ArpTime value (Valid: 1, 2, 4, 8, 16, 32).";
					return StatusCode::InvalidArpeggiatorTimeValue;
				}
				_nArpTime = nVal;
				_nArpNoteTicks = _ticksPerBar / _nArpTime;
			}
			else if (vKeyValue[0] == "ArpGatePercent")
			{
				if (!akl::VerifyTextInteger (vKeyValue[1], nVal, 1, 200))
				{
					_sStatusMessage = "Invalid +ArpGatePercent value (range 1-200).";
					return StatusCode::InvalidArpeggiatorGatePercentValue;
				}
				_nArpGatePercent = nVal / 100.0f;
			}
			else if (vKeyValue[0] == "ArpOctaveSteps")
			{
				if (!akl::VerifyTextInteger (vKeyValue[1], nVal, -6, 6))
				{
					_sStatusMessage = "Invalid +ArpOctaveSteps value (range -6 to 6).";
					return StatusCode::InvalidArpeggiatorOctaveStepsValue;
				}
				_nArpOctaveSteps = nVal;
			}
			else if (vKeyValue[0] == "TrackName")
			{
				_sTrackName = akl::RemoveWhitespace (vKV2[1], 11);
			}
			else if (vKeyValue[0] == "FunkStrum")
			{
				if (!akl::VerifyTextInteger (vKeyValue[1], nVal, 0, 6))
				{
					_sStatusMessage = "Invalid +FunkStrum value (range 0-6).";
					return StatusCode::InvalidFunkStrumValue;
				}
				_bFunkStrum = nVal > 0;
				if (_bFunkStrum)
					_nNoteStagger = nVal;
				continue;
			}
			else if (vKeyValue[0] == "FunkStrumUpStrokeAttenuation")
			{
				if (!akl::VerifyDoubleInteger (vKeyValue[1], ndVal, 0.1, 1))
				{
					_sStatusMessage = "Invalid +FunkStrumUpStrokeAttenuation value (range 0.1 - 1.0).";
					return StatusCode::InvalidFunkStrumUpStrokeAttenuationValue;
				}
				_nFunkStrumUpStrokeAttenuation = ndVal;
				continue;
			}
			else if (vKeyValue[0] == "FunkStrumVelDeclineIncrement")
			{
				if (!akl::VerifyTextInteger (vKeyValue[1], nVal, 0, 20))
				{
					_sStatusMessage = "Invalid +FunkStrumVelDeclineIncrement value (range 0-20).";
					return StatusCode::InvalidFunkStrumVelDeclineIncrementValue;
				}
				_nFunkStrumVelDeclineIncrement = nVal;
				continue;
			}
			else if (vKeyValue[0] == "MelodyMode")
			{
				if (!akl::VerifyTextInteger (vKeyValue[1], nVal, 0, 1))
				{
					_sStatusMessage = "Invalid +MelodyMode value (valid: 0 or 1).";
					return StatusCode::InvalidMelodyModeValue;
				}
				_bMelodyMode = nVal == 1;
				_melodyModeLineNum = nLineNum;
				continue;
			}
			else
			{
				std::string p = sLine.substr (1, sTemp.size() - 1);
				_sStatusMessage = "Unrecognized command file parameter: +" + vKV2[0];
				return StatusCode::UnrecognizedCommandFileParameter;
			}

			continue;
		}

		if (sTemp.substr (0, 32) == sRuler)
		{
			// Ruler line. The next two lines should contain
			// (1) Note Positions: series of hash groups
			// (2) Comma-separated Chord Names, one for each of the Note Position hash groups.

			nRulerLen = sTemp.length();

			if (nRulerLen % 32 != 0)
			{
				std::ostringstream ss;
				ss << "Line " << nLineNum << ": Invalid ruler line.";
				_sStatusMessage = ss.str();
				return StatusCode::InvalidRulerLine;
			}

			uint32_t nNumBars = sTemp.length() / 32;
			for (uint32_t i = 1; i < nNumBars; i++)
			{
				if (sTemp.substr (i * 32, 32) != sRuler)
				{
					std::ostringstream ss;
					ss << "Line " << nLineNum << ": Invalid ruler line.";
					_sStatusMessage = ss.str();
					return StatusCode::InvalidRulerLine;
				}
			}

			if (nNumBars > 4)
			{
				std::ostringstream ss;
				ss << "Line " << nLineNum << ": Maximum of 4 bars per line.";
				_sStatusMessage = ss.str();
				return StatusCode::MaxFourBarsPerLine;
			}

			_vBarCount.push_back (nNumBars);

			nDataLines++;
			continue;
		}

		std::ostringstream ss;
		ss << "Line " << nLineNum << ": Invalid line.";
		_sStatusMessage = ss.str();
		return StatusCode::InvalidLine;
	}


	uint8_t nSharpFlat = 0;
	int8_t res = NoteToMidi ("C" + _sOctaveRegister, _nProvisionalLowestNote, nSharpFlat);

	// NoteStagger cancels randomized note positions.
	if (_nNoteStagger)
	{
		_bRandNoteStart = false;
		_bRandNoteEnd = false;
	}

	// FunkStrum cancels: Arp, randomized velocity and randomized note positions.
	if (_bFunkStrum)
	{
		_nArpeggiator = 0;
		_nRandVelVariation = 0;
		_bRandNoteStart = false;
		_bRandNoteEnd = false;
	}

	// Arpeggiation enabled cancels: randomized note positions., and note stagger.
	if (_nArpeggiator)
	{
		_bRandNoteStart = false;
		_bRandNoteEnd = false;
		_nNoteStagger = 0;
	}

	// Modify base velocity if random variation enabled.
	if (_nVelocity - (_nRandVelVariation / 2) < 0)
		_nVelocity = 5;
	else
		_nVelocity -= (_nRandVelVariation / 2);	// offset base velocity to allow for upward random variation.

	if (_vChordNames.size() == 0)
	{
		_sStatusMessage = "No chords specified - output file will not be produced.";
		return StatusCode::NoChordsSpecified;
	}

	// Check we have the same number of chords as note positions
	if (nNumberOfNotes != _vChordNames.size())
	{
		_sStatusMessage = "Number of chords does not match number of notes. (Check your + signs.)";
		return StatusCode::NumberOfChordsDoesNotMatchNoteCount;
	}

	return result;
}

CMIDIHandler::StatusCode CMIDIHandler::CopyFileWithGroove (std::string filename, bool bOverwriteOutFile)
{
	StatusCode nRes = StatusCode::Success;

	if (!bOverwriteOutFile && akl::MyFileExists (filename))
	{
		std::ostringstream ss;
		ss << "Output file already exists. Use the -o switch to overwrite, eg:\n"
			<< "SMFFTI.exe -sg mymidi.txt mymidi_groove.txt -o";
		_sStatusMessage = ss.str();
		return StatusCode::OutputFileAlreadyExists;
	}

	std::vector<uint32_t> vRand = { 0, 0, 0, 0, 1, 1, 1 };
	std::uniform_int_distribution<size_t> randNoteOff (0, vRand.size() - 1);

	for (auto& it : _vNotePositions)
	{
		// Walk the string, looking for plus-sign or hash-after-a-space,
		// indicating new note
		uint32_t n = 0;
		int32_t noteStart = -1, noteEnd = 0;
		while (n < it.length())
		{
			int ak = 1;
			if (it[n] == '+' && noteStart == -1)
			{
				noteStart = n;
			}

			else if (it[n] == '#' && noteStart == -1)
			{
				noteStart = n;
			}

			else if (noteStart >= 0 && (it[n] == ' ' || it[n] == '+' || n == it.length() - 1))
			{
				noteEnd = n - 1;
				if (n == it.length() - 1)
					noteEnd = n;

				for (int32_t i = noteStart + 1; i <= noteEnd; i++)
				{
					// Parse the note and make some gaps to introduce groove.

					if (i % 2 == 0)		// 1/16ths: Don't allow blanks.
						continue;

					uint32_t w = vRand[randNoteOff (_eng)];
					if (w == 0)
						it[i] = ' ';
					int ak = 1;

				}
				noteStart = -1;
			}

			n++;
		}
	}

	// Output copy of the input file with the generated groove.
	std::ofstream ofs (filename, std::ios::binary);
	uint32_t nLine = 1;
	uint32_t nLine2 = 0;
	for (auto s : _vInput)
	{
		if (nLine2 < _vNotePosLineInFile.size() && nLine == _vNotePosLineInFile[nLine2])
		{
			// Output the groove version of note positions.
			int ak = 1;
			ofs << _vNotePositions[nLine2++] << std::endl;
		}
		else
			ofs << s << std::endl;

		nLine++;
	}
	ofs.close();

	return nRes;
}

CMIDIHandler::StatusCode CMIDIHandler::CreateMIDIFile (const std::string& filename, bool bOverwriteOutFile)
{
	std::ofstream ofs;

	StatusCode nRes = InitMidiFile (ofs, filename, bOverwriteOutFile);
	if (nRes != StatusCode::Success)
		return nRes;

	GenerateNoteEvents();

	FinishMidiFile (ofs);

	return nRes;
}

CMIDIHandler::StatusCode CMIDIHandler::InitMidiFile (std::ofstream& ofs, const std::string& filename, bool bOverwriteOutFile)
{
	StatusCode nRes = StatusCode::Success;

	if (!bOverwriteOutFile && akl::MyFileExists (filename))
	{
		std::ostringstream ss;
		ss << "Output file already exists. Use the -o switch to overwrite, eg:\n"
			<< "SMFFTI.exe midicmds.txt MyMIDIFile.mid -o";
		_sStatusMessage = ss.str();
		return StatusCode::OutputFileAlreadyExists;
	}

	ofs.open (filename, std::ios::binary);

	//-------------------------------------------------------------------------
	// HEADER
	ofs << "MThd";

	_nVal32 = Swap32 (6);	// Header length; always 6
	ofs.write (reinterpret_cast<char*>(&_nVal32), sizeof (uint32_t));

	_nVal16 = Swap16 (0);	// Format 0: Single, multi-channel track
	ofs.write (reinterpret_cast<char*>(&_nVal16), sizeof (uint16_t));

	_nVal16 = Swap16 (1);	// Number of tracks; alwys 1 if format 0.
	ofs.write (reinterpret_cast<char*>(&_nVal16), sizeof (uint16_t));

	_nVal16 = Swap16 (_ticksPerQtrNote);	// Division: 96 ticks per 1/4 noteBit 15=0, bits 14-0 = 96
	ofs.write (reinterpret_cast<char*>(&_nVal16), sizeof (uint16_t));

	//-------------------------------------------------------------------------
	// ONE AND ONLY TRACK
	//
	// It's a series of <delta-time><event> pairs.
	// We push all track chunk data into a vector byte buffer first, before writing to file.

	//---------------------------------------
	// Meta-event: Track Name
	PushVariableValue (0);	// Delta-time is variable length.
	PushInt8 (0xFF);
	PushInt8 ((uint8_t)MetaEventName::MetaTrackName);
	PushVariableValue (_sTrackName.length());
	PushText (_sTrackName);

	//---------------------------------------
	// Event 2: Meta-event: Time signature
	PushVariableValue (0);
	PushInt8 (0xFF);
	PushInt8 ((uint8_t)MetaEventName::MetaTimeSignature);
	PushVariableValue (4);	// Data length.
	PushInt8 (4);	// 4 beats to the bar.
	PushInt8 (2);	// Negative power of 2, eg. 2 is a 1/4 note, 4 is 1/8th note.
	PushInt8 (36);	// No. MIDI clocks per metronome click.
	PushInt8 (8);	// No. 1/32nd notes per MIDI 1/4 note.

	return nRes;
}

void CMIDIHandler::FinishMidiFile (std::ofstream& ofs)
{
	if (_bRandNoteStart || _bRandNoteEnd)
		SortNoteEventsAndFixOverlaps();

	if (_nNoteStagger)
		ApplyNoteStagger();

	if (_nArpeggiator)
		ApplyArpeggiation();

	PushNoteEvents();

	// Meta-event: End Of Track
	PushVariableValue (0);
	PushInt8 (0xFF);
	PushInt8 ((uint8_t)MetaEventName::MetaEndOfTrack);
	PushVariableValue (0);

	// Write track chunk to file.
	ofs << "MTrk";
	_nVal32 = Swap32 (_vTrackBuf.size());
	ofs.write (reinterpret_cast<char*>(&_nVal32), sizeof (uint32_t));	// Chunk length.
	ofs.write ((char*)&_vTrackBuf[0], _vTrackBuf.size());				// Chunk data.

	ofs.close();
}

std::string CMIDIHandler::GetRandomGroove (bool& bRandomGroove)
{
	// Randomly construct the note positions by building a
	// string something like "+### +#   +### +#".

	// Init for dynamic construction.
	std::string sNotePositions;

	// Get the current number of bars to populate.
	uint32_t nNumBars = _vBarCount.back();

	// Lambda func to return random note length
	auto RandNoteLen = [](std::vector<int> v, size_t& nNum16ths)
	{
		// Note length:
		// 0 = off, 1 = 1/16th, 2 = 1/8th, 3 = 3/8ths, 4 = 1/4
		std::uniform_int_distribution<size_t> randNoteLen (0, v.size() - 1);
		nNum16ths = v[randNoteLen (_eng)];
		std::string x = "  ";	// blank 1/16th
		if (nNum16ths > 0)
			x = std::string ("+#######").substr (0, nNum16ths * 2);
		else
			nNum16ths = 1;	// for blank note still must register a 1/16th note
		return x;
	};

	// Lambda
	auto BuildNotePositionString = [&]()
	{
		sNotePositions = "";

		// Iterate in 1/16th increments
		uint32_t nNum16ths = nNumBars * 16;
		std::string sNoteLen;
		size_t nNoteLen16ths = 0;
		size_t i = 0;
		while (i < nNum16ths)
		{
			if (i % 4 == 0)
			{
				// 1/4 note position
				// We pass in a weighted list - favours shorter length
				sNoteLen = RandNoteLen ({ 0, 0, 0, 1, 1, 1, 2, 2, 2, 3, 4 }, nNoteLen16ths);	// any of the note lengths
				int ak = 1;
			}
			else if (i % 2 == 0)
			{
				// 1/8th note position
				sNoteLen = RandNoteLen ({ 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2 }, nNoteLen16ths);	// Off, 1/16th, 1/8th or 3/8ths
				int ak = 1;
			}
			else
			{
				// Odd-numbered 1/16th note position,
				// ie. an upstroke.
				sNoteLen = RandNoteLen ({ 0, 0, 0, 0, 0, 1, 1, 1, 1 }, nNoteLen16ths);		// Off or 1/16th
				int ak = 1;
			}

			sNotePositions += sNoteLen;
			i += nNoteLen16ths;	// advance counter according to determined note len.
			int ak = 1;
		}
	};

	bRandomGroove = true;

	// Validate the string
	while (1)
	{
		BuildNotePositionString();

		// Too many consecutive 1/16ths
		size_t pos = sNotePositions.find ("+#+#+#+#");
		if (pos != std::string::npos)
			continue;

		// No big gaps, ie. 3/8th or larger.
		pos = sNotePositions.find ("      ");
		if (pos != std::string::npos)
			continue;

		// No consecutive 1/4 notes.
		pos = sNotePositions.find ("+#######+#######");
		if (pos != std::string::npos)
			continue;

		// No more than two consecutive 1/8th notes
		pos = sNotePositions.find ("+###+###+###");
		if (pos != std::string::npos)
			continue;

		break;
	}


	return sNotePositions;
}

void CMIDIHandler::GenerateNoteEvents()
{
	// Double parse of the note positions. First time through counts the notes
	// and extends the chord list where chord repititions are played (ie. spaces
	// between hashes). Second parse produces the list (vector) of MIDI note events.
	// Its' done like this because we need to ascertain the full number of notes
	// played *before* calling AddMIDIChordNoteEvents.

	// If randomized note start enabled, prefix with an additional bar
	// to allow for note commencing *before* the notional start position.
	uint16_t nBar = _bRandNoteStart && (!_bRandNoteOffsetTrim) ? 1 : 0;

	std::ofstream ofs;
	if (_bMelodyMode)
	{
		ofs.open ("melody.txt", std::ios::binary);
		ofs << "+MelodyFile=1\n\n";
	}

	for (uint8_t i = 0; i < 2; i++)
	{
		uint32_t pos32nds = 0;

		bool bNoteOn = false;
		int32_t nChordPair = -1;
		int32_t nNote = -1;
		int32_t nPrevNote = 0;
		uint32_t nItem = 0;

		for each (auto s in _vNotePositions)
		{
			pos32nds = nBar * 32;

			std::string s2 (s);
			std::transform (s2.begin(), s2.end(), s2.begin(), ::toupper);

			// If a melody line is present for the current chord set, use it instead of
			// outputting full chords
			bool bMelody = false;
			std::vector<std::string> vMN;
			int32_t nMelodyNote = -1;
			if (i == 1 && _vMelodyNotes[nItem].size())
			{
				vMN = akl::Explode (_vMelodyNotes[nItem], ":,");
				bMelody = true;
			}

			uint32_t nNoteCount = 0;

			auto ResolveMelodyNote = [&]()
			{
				nNoteCount++;
				nMelodyNote = bMelody ? std::stoi (vMN[nNoteCount]) : -1;
				return nMelodyNote;
			};

			for each (auto c in s)
			{
				if (c == '+')
				{
					nNote++;

					// start of note
					if (!bNoteOn)
					{
						if (i == 0)
							bNoteOn = !bNoteOn;
						else
						{
							AddMIDIChordNoteEvents (ResolveMelodyNote(), ++nChordPair, _vChordNames[nNote], bNoteOn, pos32nds * _ticksPer32nd);
						}
					}
					else
					{
						// Another note detected without a gap from previous note,
						// so insert a Note Off first.
						if (i == 1)
						{
							AddMIDIChordNoteEvents (nMelodyNote, nChordPair, _vChordNames[nPrevNote], bNoteOn, pos32nds * _ticksPer32nd);
							AddMIDIChordNoteEvents (ResolveMelodyNote(), ++nChordPair, _vChordNames[nNote], bNoteOn, pos32nds * _ticksPer32nd);
						}
					}
				}
				else if (c == '#')
				{
					if (!bNoteOn)
					{
						// Consider this as repeat of the last chord
						if (i == 0)
							bNoteOn = !bNoteOn;
						else
						{
							AddMIDIChordNoteEvents (ResolveMelodyNote(), ++nChordPair, _vChordNames[nNote], bNoteOn, pos32nds * _ticksPer32nd);
						}

						if (i == 0)
						{
							std::string sNote = _vChordNames[nNote];
							_vChordNames.insert (_vChordNames.begin() + nNote, sNote);
						}

						nNote++;
					}
				}
				else
				{
					// end of note
					if (bNoteOn)
						if (i == 0)
							bNoteOn = !bNoteOn;
						else
							AddMIDIChordNoteEvents (nMelodyNote, nChordPair, _vChordNames[nNote], bNoteOn, pos32nds * _ticksPer32nd);
				}

				pos32nds++;
				nPrevNote = nNote;
			}

			// final end of note
			if (bNoteOn)
				if (i == 0)
					bNoteOn = !bNoteOn;
				else
					AddMIDIChordNoteEvents (nMelodyNote, nChordPair, _vChordNames[nNote], bNoteOn, pos32nds * _ticksPer32nd);


			// Dump the melody notes to file so user can copy the melody.
			if (i == 1 && _bMelodyMode)
			{
				for (size_t j = 0; j < _vBarCount[nItem]; j++)
					ofs << sRuler;
				ofs << "\n";

				_vBarCount;
				ofs << s << std::endl;


				std::string cn;
				std::string fred = akl::RemoveWhitespace (s, 8);


				//--------------------------------------------------------------------
				// Tokenize the note position string
				std::vector<std::string> eric;
				bool bGroup = false;
				std::string sTemp;
				for (auto c : fred)
				{
					if (c == '+')
					{
						if (!bGroup)
						{
							// New group of chars
							bGroup = true;
							sTemp += c;
						}
						else
						{
							// End of group, new group
							eric.push_back (sTemp);
							sTemp = c;
						}
					}
					else if (c == '#')
					{
						sTemp += c;
						bGroup = true;
					}
					else
					{
						// Assumed to be a space - end of group
						bGroup = false;
						eric.push_back (sTemp);
						sTemp = "";
					}
				}

				if (sTemp.size())
					eric.push_back (sTemp);
				//--------------------------------------------------------------------



				// Construct list of chords
				uint32_t nC = 0;
				std::string comma;
				for (auto e : eric)
				{
					if (e[0] == '+')
					{
						cn += comma + _vMelodyChordNames[nC];
						comma = ", ";
					}
					nC++;
				}
				ofs << cn << std::endl;

				uint32_t nCount = 0;
				std::string prevChordName;
				std::string dlim;
				ofs << "M: ";
				for (auto n : _vRandomMelodyNotes)
				{
					ofs << dlim << std::to_string(n);
					dlim = ", ";
					nCount++;
				}
				ofs << "\n\n";
				_vRandomMelodyNotes.clear();
				_vMelodyChordNames.clear();
			}

			// move pointer 4 bars forward
			if (i == 1)
				nBar += _vBarCount[nItem];

			nItem++;
		}

		_nNoteCount = nNote;
	}

	if (_bMelodyMode)
		ofs.close();
}

void CMIDIHandler::GenerateRandomMelody()
{
	_vBarCount.push_back (2);
	bool bRandomGroove = true;
	std::string sGroove = GetRandomGroove (bRandomGroove);

	// Temp saving of groove
	std::ofstream ofs ("D:\\SMFFTI\\tempGroove.txt", std::ios::binary);
	ofs << sGroove << std::endl;
	ofs.close();

	uint32_t pos32nds = 0;
	uint32_t nNoteSeq = -1;
	bool bNoteOn = false;
	uint8_t nNote = 60;

	// Notes (semitone intervals) that can be used in the melody.
	// Essentially, Major Pentatonic.
	std::vector<uint8_t> vNotes = { 0, 2, 4, 7, 9 };
	std::uniform_int_distribution<uint32_t> randNote (0, vNotes.size() - 1);

	// Lambda
	auto AddNote = [&](EventName evt)
	{
		uint32_t nTime = pos32nds * _ticksPer32nd;
		uint8_t nEvent = (uint8_t)evt;
		uint8_t nVel = 80;

		// Note On: New random note and increment note sequence number.
		if (nEvent == (uint8_t)EventName::NoteOn)
		{
			nNote = 60 + vNotes[randNote (_eng)];
			nNoteSeq++;
		}

		MIDINote note (nNoteSeq, nTime, nEvent, nNote, nVel);
		_vMIDINoteEvents.push_back (note);

		bNoteOn = !bNoteOn;
	};

	// Parse the groove
	for each (auto c in sGroove)
	{
		if (c == '+')
		{
			// start of note
			if (!bNoteOn)
			{
				AddNote (EventName::NoteOn);
			}
			else
			{
				// Another note detected without a gap from previous note,
				// so insert a Note Off first.
				AddNote (EventName::NoteOff);
				AddNote (EventName::NoteOn);
			}
		}
		else if (c == '#')
		{
			// note continues
		}
		else
		{
			// ie. a space
			// end of note
			if (bNoteOn)
				AddNote (EventName::NoteOff);
		}

		pos32nds++;
	}

	if (bNoteOn)
		AddNote (EventName::NoteOff);	// Ensure a final note-off.

	int ak = 1;
}

void CMIDIHandler::SortNoteEventsAndFixOverlaps()
{
	// If randomized note start/end applies, need to sort into event time order
	// and fix any overlap errors introduced.
	std::sort (_vMIDINoteEvents.begin(), _vMIDINoteEvents.end(),
		[](MIDINote m1, MIDINote m2) { return (m1.nTime < m2.nTime);  }
	);

	// Parse all notes to correct instances of overlap as a result of
	// the randomized note start/end. We may have introduced two
	// consecutive ONs. For each note, it should be a strictly
	// ON-OFF-ON-OFF sequence.
	std::map<uint8_t, uint8_t> mNotesEncountered;
	for each (auto note in _vMIDINoteEvents)
	{
		std::map<uint8_t, uint8_t>::iterator it;
		it = mNotesEncountered.find (note.nKey);
		if (it == mNotesEncountered.end())
		{
			// First time for this note, so add it to our list.
			mNotesEncountered.insert (std::pair<uint8_t, uint8_t>(note.nKey, note.nEvent));

			// Now, for this note, look for all instances of it the list of MIDI notes events,
			// and ensure that they go on, off, on, off...
			bool bOn = true;
			int i = -1;
			for each (auto note1 in _vMIDINoteEvents)
			{
				i++;

				if (note1.nKey != note.nKey)
					continue;

				_vMIDINoteEvents[i].nEvent = ((uint8_t)(bOn ? EventName::NoteOn : EventName::NoteOff) | _nChannel);

				bOn = !bOn;
			}
		}
	}
}

void CMIDIHandler::ApplyNoteStagger()
{
	SortChordNotes();

	uint32_t nItem = 0;

	while (nItem < _vMIDINoteEvents.size())
	{
		// Deal with each 'pair' of Chord Note Sets - one for Note On and one for Note Off.
		//
		// First: How many notes in the chord?
		uint8_t nNumNotes = 0;
		MIDINote note = _vMIDINoteEvents[nItem];
		do
		{
			nNumNotes++;
			nItem++;
		} while (nItem < _vMIDINoteEvents.size() && _vMIDINoteEvents[nItem].nKey != note.nKey);
		nItem -= nNumNotes;

		uint32_t nStartTime = _vMIDINoteEvents[nItem].nTime;
		uint8_t nOrigVel = _vMIDINoteEvents[nItem].nVel;

		int8_t ns = _nNoteStagger;

		// FunkStrum: Apply a stagger according to 1/16th notes.
		// Ascending-note stagger for down strokes, descending-note stagger for up strokes.
		int16_t nVelAdjAmt = 0;
		int16_t nVel = nOrigVel;
		if (_bFunkStrum)
		{
			nVelAdjAmt = _nFunkStrumVelDeclineIncrement;

			bool bDownStroke = ((nStartTime / _ticksPer16th) % 2) == 0;
			if (!bDownStroke)
			{
				// ie. Up Stroke
				ns = -_nNoteStagger;

				// Because the notes will be re-sorted as a result of the
				// stagger, for Up Strokes we have to *increase* the
				// velocity, so set an initial value. Up strokes also might
				// have less velocity as they're not always struck so hard?
				nVel = (uint8_t)(nVel * _nFunkStrumUpStrokeAttenuation);
				nVel -= (std::min)((nNumNotes - 1) * nVelAdjAmt, nVel - 1);
				nVelAdjAmt = -nVelAdjAmt;
			}
		}

		// Negative stagger: Start from the highest note.
		uint8_t nNoteStartOffset = 0;
		if (ns < 0)
			nNoteStartOffset = std::abs (ns) * (nNumNotes - 1);

		std::vector<MIDINote>::iterator it;
		for (uint8_t i = 0; i < nNumNotes; i++)
		{
			it = _vMIDINoteEvents.begin() + (nItem + i);
			it->nTime += nNoteStartOffset;
			nNoteStartOffset += ns;

			// FunkStrum: Declining velocity.
			it->nVel = (uint8_t)nVel;
			nVel -= nVelAdjAmt;
		}

		// Advance to next chord (pair)
		nItem += (nNumNotes * 2);
	}

	// Another sort is required, to get everything in time order..
	std::sort (_vMIDINoteEvents.begin(), _vMIDINoteEvents.end(),
		[](MIDINote m1, MIDINote m2) { return (m1.nTime < m2.nTime);  }
	);
}

void CMIDIHandler::ApplyArpeggiation()
{
	SortChordNotes();

	uint32_t nSeq = 0;
	uint32_t nItem = 0;

	uint32_t nArpGate = (int32_t)(_nArpNoteTicks * _nArpGatePercent);

	while (nItem < _vMIDINoteEvents.size())
	{
		// Deal with each 'pair' of Chord Note Sets - one for Note On and one for Note Off.
		//
		// First: How many notes in the chord?
		uint8_t nNumNotes = 0;
		MIDINote note = _vMIDINoteEvents[nItem];
		do
		{
			nNumNotes++;
			nItem++;
		} while (nItem < _vMIDINoteEvents.size() && _vMIDINoteEvents[nItem].nKey != note.nKey);
		nItem -= nNumNotes;
		//
		// For each note in chord, generate a bunch of arp notes by
		// cycling around the note set.
		//
		// Get the Note On and Off times for the chord.
		uint32_t nStartTime = _vMIDINoteEvents[nItem].nTime;
		uint32_t nEndTime = _vMIDINoteEvents[nItem + nNumNotes].nTime;
		//

		// For the specified arp type construct a sequence list
		// according to the number of notes in the chord.
		std::vector<uint8_t> vArpSequence;

		if (_nArpeggiator == 1)
		{
			// "Up"
			for (int8_t i = 0; i < nNumNotes; i++)
			{
				vArpSequence.push_back (i);
			}
		}
		else if (_nArpeggiator == 2)
		{
			// "Down"
			for (int8_t i = nNumNotes - 1; i >= 0; i--)
			{
				vArpSequence.push_back (i);
			}
		}
		else if (_nArpeggiator == 3)
		{
			// "Up Down"
			for (int8_t i = 0; i < nNumNotes; i++)
			{
				vArpSequence.push_back (i);
			}
			for (int8_t i = nNumNotes - 2; i >= 1; i--)
			{
				vArpSequence.push_back (i);
			}
		}
		else if (_nArpeggiator == 4)
		{
			// "Down Up"
			for (int8_t i = nNumNotes - 1; i >= 0; i--)
			{
				vArpSequence.push_back (i);
			}
			for (int8_t i = 1; i < nNumNotes - 1; i++)
			{
				vArpSequence.push_back (i);
			}
		}
		else if (_nArpeggiator == 5)
		{
			// "Up & Down"
			for (int8_t i = 0; i < nNumNotes; i++)
			{
				vArpSequence.push_back (i);
			}
			for (int8_t i = nNumNotes - 1; i >= 0; i--)
			{
				vArpSequence.push_back (i);
			}
		}
		else if (_nArpeggiator == 6)
		{
			// "Down & Up"
			for (int8_t i = nNumNotes - 1; i >= 0; i--)
			{
				vArpSequence.push_back (i);
			}
			for (int8_t i = 0; i < nNumNotes; i++)
			{
				vArpSequence.push_back (i);
			}
		}
		else if (_nArpeggiator == 7)
		{
			// "Converge"
			uint8_t low = 0;
			uint8_t high = nNumNotes - 1;
			for (int8_t i = 0; i < nNumNotes; i++)
			{
				if (i % 2 == 0)
					vArpSequence.push_back (low++);
				else
					vArpSequence.push_back (high--);
			}
		}
		else if (_nArpeggiator == 8)
		{
			// "Diverge"
			uint8_t low = 1;
			uint8_t high = 0;
			uint8_t mid = nNumNotes / 2;
			for (int8_t i = 0; i < nNumNotes; i++)
			{
				if (i % 2 == 0)
					vArpSequence.push_back (mid + high++);
				else
					vArpSequence.push_back (mid - low++);
			}
		}
		else if (_nArpeggiator == 9)
		{
			// "Converge & Diverge"
			int8_t direction = -1;
			uint8_t lo = 0;
			uint8_t hi = nNumNotes - 1;

			for (int8_t i = 0; i < nNumNotes; i++)
			{
				if (direction > 0)
					vArpSequence.push_back (hi--);
				else
					vArpSequence.push_back (lo++);

				direction = -direction;
			}

			// Last value for the converge becomes the first value of diverge.
			lo = vArpSequence.back() - 1;
			hi = lo + 2;
			if (direction < 0)
			{
				hi = vArpSequence.back() + 1;
				lo = hi - 2;
			}
			for (int8_t i = 1; i < nNumNotes - 1; i++)
			{
				if (direction > 0)
					vArpSequence.push_back (hi++);
				else
					vArpSequence.push_back (lo--);

				direction = -direction;
			}
		}
		else if (_nArpeggiator == 10)
		{
			// "Pinky Up"
			uint8_t low = 0;
			uint8_t high = nNumNotes - 1;
			for (int8_t i = 0; i < nNumNotes - 1; i++)
			{
				vArpSequence.push_back (low++);
				if (low >= high)
					low = 0;
				vArpSequence.push_back (high);
			}
		}
		else if (_nArpeggiator == 11)
		{
			// "Pinky UpDown"
			uint8_t low = 0;
			uint8_t high = nNumNotes - 1;
			int8_t direction = 1;
			for (int8_t i = 0; i < (nNumNotes - 2) * 2; i++)
			{
				vArpSequence.push_back (low);
				low += direction;
				if (low <= 0 || low >= high)
				{
					direction = -direction;
					low += direction;
					low += direction;
				}
				vArpSequence.push_back (high);
			}
		}
		else if (_nArpeggiator == 12)
		{
			// "Thumb Up"
			uint8_t low = 0;
			uint8_t high = 1;
			int8_t max = nNumNotes - 1;
			for (int8_t i = 0; i < max; i++)
			{
				vArpSequence.push_back (low);
				vArpSequence.push_back (high++);
				if (high > max)
					high = 1;
			}
		}
		else if (_nArpeggiator == 13)
		{
			// "Thumb UpDown"
			uint8_t low = 0;
			uint8_t high = 1;
			int8_t direction = 1;
			int8_t max = (nNumNotes - 2) * 2;
			for (int8_t i = 0; i < max; i++)
			{
				vArpSequence.push_back (low);
				vArpSequence.push_back (high);
				if ((high <= 1 && direction == -1) || (high >= (nNumNotes - 1) && direction == 1))
					direction = -direction;
				high += direction;
			}
		}

		// First note in arp sequence.
		uint8_t nArpItem = 0;
		note = _vMIDINoteEvents[nItem + vArpSequence[nArpItem]];

		// For the first note, start time is unchanged
		_vMIDINoteEvents2.push_back (note);
		// New event off for the arp note
		uint8_t nEventType = ((uint8_t)EventName::NoteOff) | _nChannel;
		uint32_t nTimeNote = note.nTime;
		note.nTime += nArpGate;
		note.nEvent = nEventType;
		_vMIDINoteEvents2.push_back (note);

		//
		// Loop until end time reached.
		uint32_t nCurTime = nTimeNote + _nArpNoteTicks;
		uint32_t nItemSave = nItem;
		int8_t nOctave = 0;
		uint8_t nTotalOctaveSteps = std::abs (_nArpOctaveSteps);
		uint8_t nOctaveCount = 0;
		while (nCurTime < nEndTime)
		{
			// Next note in the chord.
			nArpItem++;

			if (nArpItem >= vArpSequence.size())
			{
				// Recycle the arp
				nArpItem = 0;

				// Apply Octave Step Transposition.
				if (_nArpOctaveSteps != 0)
				{
					if (_nArpOctaveSteps > 0)
						nOctave++;
					else
						nOctave--;

					nOctaveCount++;
					if (nOctaveCount > nTotalOctaveSteps)
					{
						nOctaveCount = 0;
						nOctave = 0;
					}
				}
			}

			MIDINote note = _vMIDINoteEvents[nItem + vArpSequence[nArpItem]];

			// Apply any Octave Step Transposition.
			uint16_t noteTemp = nOctave * 12;
			if (noteTemp > 127)
				noteTemp -= 12;
			note.nKey += noteTemp;

			// Note start.
			note.nTime = nCurTime;
			_vMIDINoteEvents2.push_back (note);

			// Note end.
			nEventType = ((uint8_t)EventName::NoteOff) | _nChannel;
			nTimeNote = note.nTime;
			note.nTime += nArpGate; //nArpNoteTicks;
			note.nEvent = nEventType;
			_vMIDINoteEvents2.push_back (note);

			nCurTime = nTimeNote + _nArpNoteTicks;	//note.nTime;
		}

		// Advance to next chord (pair)
		nItem = nItemSave + (nNumNotes * 2);
	}

	_vMIDINoteEvents.assign (_vMIDINoteEvents2.begin(), _vMIDINoteEvents2.end());


	// Another sort is required, to get everything in time order..
	std::sort (_vMIDINoteEvents.begin(), _vMIDINoteEvents.end(),
		[](MIDINote m1, MIDINote m2) { return (m1.nTime < m2.nTime);  }
	);

	// Check for and fix overlaps, ie. instances of 2 consecutive Note On events
	// for the same note. Example:
	//
	//      336                436
	//       |------------------|
	//
	//                     432                532
	//                      |------------------|
	//
	// The end of the first note overlaps the start of the second note. To fix this,
	// we must truncate the end of the first note (for arpeggiation, it is the start
	// of the notes which must remain fixed). 
	//
	// In the list of events, we have this:
	//
	//        1. 336 Note On
	//        2. 432 Note On
	//        3. 436 Note Off
	//        4. 532 Note Off
	//
	// The problem is the two consecutive Note On events. So what we do is, insert
	// a Note Off event before item 2, with the same time as item 2; and delete the
	// orginal Note Off that corresponds to the item 1 Note On:
	//
	//        1. 336 Note On
	//           432 Note Off    <--- new event inserted
	//        2. 432 Note On
	//        (3. 436 Note Off)  <--- delete this
	//        4. 532 Note Off
	//
	// So then we have a correct sequence of Note On, Note Off, Note On, Notew Off, etc.

	uint32_t item1 = 0;
	uint32_t item2 = 0;
	uint32_t item3 = 0;
	std::map<uint8_t, uint8_t> mNotesEncountered;

	while (item1 < _vMIDINoteEvents.size())
	{
		MIDINote n1 = _vMIDINoteEvents[item1];

		std::map<uint8_t, uint8_t>::iterator it;
		it = mNotesEncountered.find (n1.nKey);
		if (it == mNotesEncountered.end())
		{
			// First time for this note, so add it to our list.
			mNotesEncountered.insert (std::pair<uint8_t, uint8_t>(n1.nKey, n1.nEvent));

			item2 = item1 + 1;
			while (item2 < _vMIDINoteEvents.size())
			{
				MIDINote n2 = _vMIDINoteEvents[item2];
				if (n2.nKey == n1.nKey)
				{
					if (n2.nEvent == ((uint8_t)EventName::NoteOn & 0xF0) && n1.nEvent == ((uint8_t)EventName::NoteOn & 0xF0))
					{
						// Overlap.
						//
						// Insert a Note Off event so that it sits before this n2 Note On event.
						MIDINote nNew (n1.nSeq, n2.nTime, (uint8_t)EventName::NoteOff | _nChannel, n1.nKey, 0);
						_vMIDINoteEvents.insert (_vMIDINoteEvents.begin() + item2, nNew);
						//
						// Locate the next event for the this note, which should be a Note Off, and delete it.
						item2++;
						item3 = item2 + 1;
						while (item3 < _vMIDINoteEvents.size())
						{
							MIDINote n3 = _vMIDINoteEvents[item3];
							if (n3.nKey == n2.nKey)
							{
								if ((n3.nEvent & 0xF0) == (uint8_t)EventName::NoteOff)
								{
									// Delete from vector.
									_vMIDINoteEvents.erase (_vMIDINoteEvents.begin() + item3);
									break;
								}
							}
							item3++;
						}
					}
					n1 = n2;
				}
				item2++;
			}
		}
		item1++;
	}
}

void CMIDIHandler::SortChordNotes()
{
	// For the sake of Arpeggiation or Note Stagger...
	// If downward transposition has occurred we must re-order the note such that,
	// for each group with the same time, sort into ascending note order

	std::map<uint8_t, MIDINote> mTemp;

	std::vector<MIDINote> vTemp;
	uint32_t itemFirst = 0;
	uint32_t itemLast = 1;
	while (1)
	{
		while (itemLast < _vMIDINoteEvents.size()
			&& _vMIDINoteEvents[itemLast].nTime == _vMIDINoteEvents[itemFirst].nTime
			&& _vMIDINoteEvents[itemLast].nEvent == _vMIDINoteEvents[itemFirst].nEvent)
			itemLast++;
		for (uint32_t i = itemFirst; i < itemLast; i++)
		{
			MIDINote note = _vMIDINoteEvents[i];
			mTemp.insert (std::pair<uint8_t, MIDINote> (note.nKey, note));
		}
		for each (auto v in mTemp)
		{
			MIDINote n = v.second;
			vTemp.push_back (n);
		}
		mTemp.clear();

		if (itemLast >= _vMIDINoteEvents.size())
			break;

		itemFirst = itemLast;
		itemLast++;
	}

	_vMIDINoteEvents.assign (vTemp.begin(), vTemp.end());
}

void CMIDIHandler::PushNoteEvents()
{
	uint32_t nPrevNoteTime = 0;
	for each (auto note in _vMIDINoteEvents)
	{
		uint32_t nNoteTime = note.nTime;
		uint32_t nDeltaTime = nNoteTime - nPrevNoteTime;

		PushVariableValue (nDeltaTime);
		PushInt8 (note.nEvent);
		PushInt8 (note.nKey);
		PushInt8 (note.nVel);

		nPrevNoteTime = nNoteTime;
	}
}

void CMIDIHandler::AddMIDIChordNoteEvents (int32_t nMelodyNote, uint32_t nNoteSeq, std::string chordName, bool& bNoteOn, uint32_t nEventTime)
{
	bNoteOn = !bNoteOn;

	if (!bNoteOn && _bFunkStrum)
	{
		// FunkStrum: Shorten the note slightly, to prevent
		// funk notes running into each other.
		nEventTime -= 3;
	}

	// Set randomizer ranges for note positions.
	std::uniform_int_distribution<int> randNoteStartOffset (-_nRandNoteStartOffset, _nRandNoteStartOffset);
	std::uniform_int_distribution<int> randNoteEndOffset (-_nRandNoteEndOffset, _nRandNoteEndOffset);

	std::uniform_int_distribution<int> randNoteStartOffsetPositive (0, _nRandNoteStartOffset);
	std::uniform_int_distribution<int> randNoteEndOffsetNegative (-_nRandNoteEndOffset, 0);

	// Range for random velocity.
	std::uniform_int_distribution<int> randVelVariation (0, _nRandVelVariation);
	
	// Lambda funcs for randomizing note start/end.
	auto fnRandStart = [&](bool bPositiveOnly)
	{
		int8_t nRand = 0;
		if (_bRandNoteStart)
			nRand = bPositiveOnly && _bRandNoteOffsetTrim ? randNoteStartOffsetPositive (_eng) : randNoteStartOffset (_eng);
		return nRand;
	};
	auto fnRandEnd = [&](bool bNegativeOnly)
	{
		int8_t nRand = 0;
		if (_bRandNoteEnd)
			nRand = bNegativeOnly && _bRandNoteOffsetTrim ? randNoteEndOffsetNegative (_eng) : randNoteEndOffset (_eng);
		return nRand;
	};

	// Lambda func to get random velocity variation.
	auto fnRandVel = [&]() {
		uint8_t nRand = 0;
		if (_nRandVelVariation > 0)
			nRand = randVelVariation (_eng);
		uint16_t nTemp = _nVelocity + nRand;
		if (nTemp > 127)
			nTemp = 127;
		return (uint8_t)nTemp;
	};

	std::vector<std::string> vChordIntervals;
	uint8_t nRoot = 0;
	GetChordIntervals (chordName, nRoot, vChordIntervals);


	// Init note stagger offset. If value positive, start with lowest note; if negative,
	// start from highest. First note itself is not staggered.
	int8_t notePosOffset = 0;
		
	uint8_t nEventType = (bNoteOn ? (uint8_t)EventName::NoteOn : (uint8_t)EventName::NoteOff) | _nChannel;
	uint32_t nET;


	// Has a melody note been specified?
	if (nMelodyNote >= 0)
	{
		static uint8_t nNote;
		if (bNoteOn)
		{
			uint8_t mn = (uint8_t)nMelodyNote;


			// Auto-correct notes to match scale of the chord.
			// This can happen if you re-use a melody from a major chord
			// for a minor chord, or vice-versa
			if (vChordIntervals[0] == "3")
			{
				if (mn == 2 || mn == 4 || mn == 9)
					mn++;
			}
			else
			{
				if (mn == 3 || mn == 5 || mn == 10)
					mn--;
			}


			nNote = nRoot + mn;

			// Remember note interval for output to 'melody text file'
			_vRandomMelodyNotes.push_back (mn);
			_vMelodyChordNames.push_back (chordName);
		}

		MIDINote note (nNoteSeq, nEventTime, nEventType, nNote, fnRandVel());
		_vMIDINoteEvents.push_back (note);
		return;
	}



	// MelodyMode: Instead of chords, output a random note
	// from the Major/Minor Pentatonic scale of the chord.
	// (No position offset is applicable.)
	if (_bMelodyMode)
	{
		// Notes (semitone intervals) that can be used in the melody.
		// Essentially, Major or Minor Pentatonic.
		std::vector<uint8_t> vNotes = { 0, 2, 4, 7, 9 };
		if (vChordIntervals[0] == "3")
			vNotes = { 0, 3, 5, 7, 10 };

		std::uniform_int_distribution<uint32_t> randNote (0, vNotes.size() - 1);

		static uint8_t nNote;
		if (bNoteOn)
		{
			uint8_t rn = vNotes[randNote (_eng)];
			nNote = nRoot + rn;

			// Remember random note interval (rn) for output to 'melody text file'
			_vRandomMelodyNotes.push_back (rn);
			_vMelodyChordNames.push_back (chordName);
		}

		MIDINote note (nNoteSeq, nEventTime, nEventType, nNote, fnRandVel());
		_vMIDINoteEvents.push_back (note);
		return;
	}


	// Optional extra bass note
	if (_bAddBassNote)
	{
		notePosOffset = (bNoteOn ? fnRandStart (nNoteSeq == 0) : fnRandEnd (nNoteSeq == _nNoteCount));

		nET = nEventTime + notePosOffset;
		int16_t noteTemp = nRoot - 12;
		if (noteTemp >= 0)
		{
			MIDINote note (nNoteSeq, nET, nEventType, nRoot - 12, fnRandVel());
			_vMIDINoteEvents.push_back (note);
		}
	}

	// Root note
	notePosOffset = (bNoteOn ? fnRandStart (nNoteSeq == 0) : fnRandEnd (nNoteSeq == _nNoteCount));

	nET = nEventTime + notePosOffset;
	MIDINote note (nNoteSeq, nET, nEventType, nRoot, fnRandVel());
	_vMIDINoteEvents.push_back (note);

	// Chord notes
	for each (auto item in vChordIntervals)
	{
		uint8_t nSemitones = std::stoi (item);

		// Downward transposition occurs if note is higher than highest-note threshold, or 127.
		uint16_t nNote = nRoot + nSemitones;
		while (nNote > (_nProvisionalLowestNote + _nTransposeThreshold) || nNote > 127)
			nNote -= 12;

		notePosOffset = (bNoteOn ? fnRandStart (nNoteSeq == 0) : fnRandEnd (nNoteSeq == _nNoteCount));

		nET = nEventTime + notePosOffset;

		MIDINote note (nNoteSeq, nET, nEventType, (uint8_t)nNote, fnRandVel());
		_vMIDINoteEvents.push_back (note);
	};

}

int8_t CMIDIHandler::NoteToMidi (std::string sNote, uint8_t& nNote, uint8_t& nSharpFlat)
{
	size_t nNoteLen = sNote.size();
	if (nNoteLen < 2 || nNoteLen > 3)
		return -1;

	char cOctaveNumber = sNote[nNoteLen - 1];
	int8_t nOctave = (uint8_t)std::string ("01234567").find (cOctaveNumber);
	if (nOctave == std::string::npos)
		return -1;

	std::string sNote2 = sNote.substr (0, nNoteLen - 1);
	sNote2[0] = std::toupper (sNote2[0]);

	std::map<std::string, uint8_t>::iterator it;
	it = _mChromaticScale.find (sNote2);
	if (it == _mChromaticScale.end())
		return -1;

	uint8_t nNote2 = it->second;

	// Middle C is always MIDI note number 60, but manufacturers can decide their
	// own ranges. Ableton Live sets Middle C as C3.
	nNote = nNote2 + ((nOctave + 2) * 12);

	if (nNoteLen == 3)
	{
		if (sNote[1] == '#')
			nSharpFlat = 1;
		else if (sNote[1] == 'b')
			nSharpFlat = 2;
	}

	return 0;
}

bool CMIDIHandler::GetChordIntervals (std::string sChordName, uint8_t& nRoot, std::vector<std::string>& vChordIntervals)
{
	bool bOK = true;

	std::string sChord;
	sChord.assign (1, sChordName[0]);
	if (sChordName.size() > 1 && (sChordName[1] == 'b' || sChordName[1] == '#'))
		sChord += sChordName[1];
	sChord += _sOctaveRegister;
	uint8_t nSharpFlat = 0;
	int8_t res = NoteToMidi (sChord, nRoot, nSharpFlat);

	if (res == -1)
		return false;

	// Adjust for flat/sharp.
	uint8_t nCount = 1;
	if (nSharpFlat > 0)
		nCount++;

	// Chord type
	std::string chordType = sChordName.substr (nCount, sChordName.size() - nCount);
	if (chordType == "")
		chordType = "maj";

	std::map<std::string, std::string>::iterator it;
	it = _mChordTypes.find (chordType);
	if (it == _mChordTypes.end())
		return false;

	vChordIntervals = akl::Explode (_mChordTypes[chordType], ",");

	return bOK;
}




uint32_t CMIDIHandler::Swap32 (uint32_t n) const
{
	return (((n >> 24) & 0xff) | ((n << 8) & 0xff0000) | ((n >> 8) & 0xff00) | ((n << 24) & 0xff000000));
};

uint16_t CMIDIHandler::Swap16 (uint16_t n) const
{
	return ((n >> 8) | (n << 8));
};

std::string CMIDIHandler::BitString (uint32_t n, uint32_t numBits) const
{
	std::string x;
	for (int i = numBits - 1; i >= 0; i--)
	{
		char a = n & (1 << i) ? '1' : '0';
		x += a;
		if (i % 8 == 0)
			x += "|";
	}
	return x;
};

void CMIDIHandler::PushVariableValue (uint32_t nVal)
{
	auto x = BitString (nVal, 32);

	uint32_t buf = nVal & 0x7F;

	while ((nVal >>= 7) > 0)
	{
		buf <<= 8;
		buf |= 0x80;
		buf += (nVal & 0x7F);
	}

	x = BitString (buf, 32);

	_vTrackBuf.push_back (buf & 0xFF);
	_nOffset++;
	while (buf & 0x80)
	{
		buf >>= 8;
		_vTrackBuf.push_back (buf & 0xFF);
		_nOffset++;
	}

	int ak = 1;
};

void CMIDIHandler::PushInt8 (uint8_t n)
{
	_vTrackBuf.push_back (n);
	_nOffset++;
}

void CMIDIHandler::PushText (const std::string& s)
{
	auto bytes = reinterpret_cast<const byte*>(s.data());
	_vTrackBuf.insert (_vTrackBuf.end(), bytes, bytes + s.length());
	_nOffset += s.length();
}

std::string CMIDIHandler::GetStatusMessage()
{
	return _sStatusMessage;
}

//-----------------------------------------------------------------------------
// Static class members

std::map<std::string, std::string>CMIDIHandler::_mChordTypes;
std::map<std::string, uint8_t>CMIDIHandler::_mChromaticScale;
std::vector<std::string>CMIDIHandler::_vRFGChords;

CMIDIHandler::ClassMemberInit CMIDIHandler::cmi;

CMIDIHandler::ClassMemberInit::ClassMemberInit ()
{
	_mChordTypes.insert (std::pair<std::string, std::string>("maj",		"4,7"));			// Major
	_mChordTypes.insert (std::pair<std::string, std::string>("7",		"4,7,10"));			// Dominant 7th
	_mChordTypes.insert (std::pair<std::string, std::string>("maj7",	"4,7,11"));			// Major 7th
	_mChordTypes.insert (std::pair<std::string, std::string>("9",		"4,7,10,14"));		// Dominant 9th
	_mChordTypes.insert (std::pair<std::string, std::string>("maj9",	"4,7,11,14"));		// Major 9th
	_mChordTypes.insert (std::pair<std::string, std::string>("add9",	"4,7,14"));			// Add 9
	_mChordTypes.insert (std::pair<std::string, std::string>("m",		"3,7"));			// Minor
	_mChordTypes.insert (std::pair<std::string, std::string>("m7",		"3,7,10"));			// Minor 7th
	_mChordTypes.insert (std::pair<std::string, std::string>("m9",		"3,7,10,14"));		// Minor 9th
	_mChordTypes.insert (std::pair<std::string, std::string>("madd9",	"3,7,14"));			// Minor Add 9
	_mChordTypes.insert (std::pair<std::string, std::string>("sus2",	"2,7"));
	_mChordTypes.insert (std::pair<std::string, std::string>("7sus2",	"2,7,10"));
	_mChordTypes.insert (std::pair<std::string, std::string>("sus4",	"5,7"));
	_mChordTypes.insert (std::pair<std::string, std::string>("7sus4",   "5,7,10"));
	_mChordTypes.insert (std::pair<std::string, std::string>("5",       "7"));				// Power chord

	_mChromaticScale.insert (std::pair<std::string, uint8_t>("C", 0));
	_mChromaticScale.insert (std::pair<std::string, uint8_t>("C#", 1));
	_mChromaticScale.insert (std::pair<std::string, uint8_t>("Db", 1));
	_mChromaticScale.insert (std::pair<std::string, uint8_t>("D", 2));
	_mChromaticScale.insert (std::pair<std::string, uint8_t>("D#", 3));
	_mChromaticScale.insert (std::pair<std::string, uint8_t>("Eb", 3));
	_mChromaticScale.insert (std::pair<std::string, uint8_t>("E", 4));
	_mChromaticScale.insert (std::pair<std::string, uint8_t>("F", 5));
	_mChromaticScale.insert (std::pair<std::string, uint8_t>("F#", 6));
	_mChromaticScale.insert (std::pair<std::string, uint8_t>("Gb", 6));
	_mChromaticScale.insert (std::pair<std::string, uint8_t>("G", 7));
	_mChromaticScale.insert (std::pair<std::string, uint8_t>("G#", 8));
	_mChromaticScale.insert (std::pair<std::string, uint8_t>("Ab", 8));
	_mChromaticScale.insert (std::pair<std::string, uint8_t>("A", 9));
	_mChromaticScale.insert (std::pair<std::string, uint8_t>("A#", 10));
	_mChromaticScale.insert (std::pair<std::string, uint8_t>("Bb", 10));
	_mChromaticScale.insert (std::pair<std::string, uint8_t>("B", 11));

	// Weighted to favour certain chords.
	//
	// m7
	_vRFGChords.push_back ("Em7");
	_vRFGChords.push_back ("Em7");
	_vRFGChords.push_back ("Em7");
	_vRFGChords.push_back ("Em7");
	_vRFGChords.push_back ("Em7");
	_vRFGChords.push_back ("Em7");
	_vRFGChords.push_back ("Em7");
	_vRFGChords.push_back ("Em7");
	_vRFGChords.push_back ("Em7");
	_vRFGChords.push_back ("Em7");
	_vRFGChords.push_back ("Em7");
	_vRFGChords.push_back ("Em7");
	_vRFGChords.push_back ("Am7");
	_vRFGChords.push_back ("Am7");
	_vRFGChords.push_back ("Am7");
	_vRFGChords.push_back ("Am7");
	_vRFGChords.push_back ("Am7");
	_vRFGChords.push_back ("Am7");
	_vRFGChords.push_back ("Am7");
	_vRFGChords.push_back ("Am7");
	_vRFGChords.push_back ("Bm7");
	_vRFGChords.push_back ("Bm7");
	_vRFGChords.push_back ("Bm7");
	_vRFGChords.push_back ("Bm7");
	_vRFGChords.push_back ("Bm7");
	_vRFGChords.push_back ("Bm7");
	_vRFGChords.push_back ("Bm7");
	_vRFGChords.push_back ("Bm7");
	_vRFGChords.push_back ("Dm7");
	_vRFGChords.push_back ("Dm7");
	_vRFGChords.push_back ("Dm7");
	_vRFGChords.push_back ("Dm7");
	//
	// Minor
	_vRFGChords.push_back ("Em");
	_vRFGChords.push_back ("Em");
	_vRFGChords.push_back ("Am");
	_vRFGChords.push_back ("Bm");
	//
	// 7sus4
	_vRFGChords.push_back ("E7sus4");
	_vRFGChords.push_back ("E7sus4");
	_vRFGChords.push_back ("A7sus4");
	_vRFGChords.push_back ("B7sus4");
}

// Randomizer static variable declaration
std::default_random_engine CMIDIHandler::_eng;

