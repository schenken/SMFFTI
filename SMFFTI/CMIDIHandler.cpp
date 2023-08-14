#include "pch.h"
#include "CMIDIHandler.h"
#include "CAutoRhythm.h"
#include "Common.h"

CMIDIHandler::CMIDIHandler (std::string sInputFile) : _sInputFile (sInputFile)
{
	_ticksPer16th = _ticksPerQtrNote / 4;
	_ticksPer32nd = _ticksPerQtrNote / 8;
	_ticksPerBar = _ticksPerQtrNote * 4;

	_nArpNoteTicks = _ticksPerBar / _nArpTime;

	for (int i = 0; i < static_cast<int>(ChordTypeVariation::_COUNT_); i++)
		_vChordTypeVariationFactors.push_back (0);

	// Default values, when chord type variations not specified in the command file.
	_vChordTypeVariationFactors[static_cast<int>(ChordTypeVariation::Major)]		= 1000;
	_vChordTypeVariationFactors[static_cast<int>(ChordTypeVariation::Dominant_7th)] = 100;
	_vChordTypeVariationFactors[static_cast<int>(ChordTypeVariation::Major_7th)]	= 150;
	_vChordTypeVariationFactors[static_cast<int>(ChordTypeVariation::Dominant_9th)] = 50;
	_vChordTypeVariationFactors[static_cast<int>(ChordTypeVariation::Major_9th)]	= 50;
	_vChordTypeVariationFactors[static_cast<int>(ChordTypeVariation::Add_9)]		= 200;
	_vChordTypeVariationFactors[static_cast<int>(ChordTypeVariation::Sus_2)]		= 15;
	_vChordTypeVariationFactors[static_cast<int>(ChordTypeVariation::_7_Sus_2)]		= 15;
	_vChordTypeVariationFactors[static_cast<int>(ChordTypeVariation::Sus_4)]		= 10;
	_vChordTypeVariationFactors[static_cast<int>(ChordTypeVariation::_7_Sus_4)]		= 10;
	//
	_vChordTypeVariationFactors[static_cast<int>(ChordTypeVariation::Minor)]		= 1000;
	_vChordTypeVariationFactors[static_cast<int>(ChordTypeVariation::Minor_7th)]	= 250;
	_vChordTypeVariationFactors[static_cast<int>(ChordTypeVariation::Minor_9th)]	= 50;
	_vChordTypeVariationFactors[static_cast<int>(ChordTypeVariation::Minor_Add_9)]	= 150;
	//
	_vChordTypeVariationFactors[static_cast<int>(ChordTypeVariation::Dim)]			= 0;
	_vChordTypeVariationFactors[static_cast<int>(ChordTypeVariation::Dim_7th)]		= 1;
	_vChordTypeVariationFactors[static_cast<int>(ChordTypeVariation::HalfDim)]		= 1;

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

	std::ofstream ofs (sOutFile, std::ios::out);

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

	// Initialise necessary Auto-chords defaults.
	std::vector<std::string> v = akl::Explode (_sAutoChordsMinorChordBias, ",");
	int num = 0;
	for (int i = 0; i < 3; i++)
	{
		int n = stoi (v[i]);
		_vAutoChordsMinorChordBias.push_back (n);
		num += n;
	}
	ASSERT (num <= 100);	// Ensure your code defaults are okay.
	v = akl::Explode (_sAutoChordsMajorChordBias, ",");
	num = 0;
	for (int i = 0; i < 3; i++) 
	{
		int n = stoi (v[i]);
		_vAutoChordsMajorChordBias.push_back (n);
		num += n;
	}
	ASSERT (num <= 100);	// Ensure your code defaults are okay.

	// RCR For remembering line positions when saving the previous
	// chord progression back to the original file.
	uint32_t nRCRIndex = 0;
	uint32_t nRCRLineOffset = 0;

	for each (auto sLine in _vInput)
	{
		_vInputCopy.push_back (sLine);

		nLineNum++;
		nRCRIndex = (nLineNum - 1) + nRCRLineOffset;


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
			}

			std::string sChordList;
			std::string comma = "";
			uint32_t nReplaceCount = 0;

			// T2015A
			std::uniform_int_distribution<uint16_t> randModInt (0, 99);
			if (_bRCR)
				InitChordBank (_sRCRKey);

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

				// Random Chord Replacement.
				// Look for ? prefix
				std::string qm = "";
				if (sChordName[0] == '?')
				{
					if (_bRCR)
					{
						// T2015A Which chord bank to use - the main one or the Modal Interchange one.
						uint16_t iRandModInt = randModInt (_eng);
						uint8_t iCB = _vChordBankChoice[iRandModInt];

						_vChordBank[iCB]->SetRandomChord();
						sChordName = _vChordBank[iCB]->GetChordName() + _vChordBank[iCB]->GetChordVariation();
						nReplaceCount++;
						if (nReplaceCount == 1)
							_vInputCopy[nRCRIndex] = "#" + _vInputCopy[nRCRIndex];
						qm = "?";
					}
					else
					{
						sChordName = sChordName.substr (1, sChordName.size());
					}
				}

				std::string sChordType;
				if (!GetChordIntervals (sChordName, nRoot, vChordIntervals, sChordType))
				{
					std::ostringstream ss;
					ss << "Line " << nLineNum << ": Invalid/blank chord name: " << sChordName;
					_sStatusMessage = ss.str();
					return StatusCode::InvalidOrBlankChordName;
				}

				for (uint8_t i = 0; i < nNumInstances; i++)
					_vChordNames.push_back (sChordName);

				// RCR: Building new chord progression string
				sChordList += comma + qm + sChordName;
				if (nNumInstances > 1)
					sChordList = sChordList + "(" + std::to_string (nNumInstances) + ")";
				comma = ", ";
			}

			// RCR
			if (nReplaceCount)
			{
				// 230814 Bug fix: Chord progressions amended by RCR are now saved (to
				// the source file) in reverse order, ie. the latest at the top.
				// By having the the 'active' chord progression where it, ideally, should
				// be, ie. immediately below the note positions line, we prevent a cock-up
				// when the output file is used as input to generate the MIDI file.
				//_vInputCopy.insert (_vInputCopy.begin() + nRCRIndex + 1, sChordList);
				_vInputCopy.insert (_vInputCopy.begin() + nRCRIndex, sChordList);
				nRCRLineOffset++;
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
			if (vKeyValue[0] == "RootNoteOnly")
			{
				if (!akl::VerifyTextInteger (vKeyValue[1], nVal, 0, 1))
				{
					_sStatusMessage = "Invalid +RootNoteOnly value (valid: 0 or 1).";
					return StatusCode::InvalidRootNoteOnlyValue;
				}
				_bRootNoteOnly = nVal == 1;
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
			else if (vKeyValue[0] == "AutoMelody")
			{
				if (!akl::VerifyTextInteger (vKeyValue[1], nVal, 0, 1))
				{
					_sStatusMessage = "Invalid +AutoMelody value (valid: 0 or 1).";
					return StatusCode::InvalidAutoMelodyValue;
				}
				_bAutoMelody = nVal == 1;
				_autoMelodyLineNum = nLineNum;
				continue;
			}
			else if (vKeyValue[0] == "AutoRhythmNoteLenBias")
			{
				_sAutoRhythmNoteLenBias = vKeyValue[1];
				if (!ValidBiasParam (_sAutoRhythmNoteLenBias, 6))
				{
					_sStatusMessage = "Invalid +AutoRhythmNoteLenBias parameter.";
					return StatusCode::InvalidAutoRhythmNoteLenBias;
				}
			}
			else if (vKeyValue[0] == "AutoRhythmGapLenBias")
			{
				_sAutoRhythmGapLenBias = vKeyValue[1];
				if (!ValidBiasParam (_sAutoRhythmGapLenBias, 6))
				{
					_sStatusMessage = "Invalid +AutoRhythmGapLenBias parameter.";
					return StatusCode::InvalidAutoRhythmGapLenBias;
				}
			}
			else if (vKeyValue[0] == "AutoRhythmConsecutiveNoteChancePercentage")
			{
				if (!akl::VerifyTextInteger (vKeyValue[1], nVal, 0, 100))
				{
					_sStatusMessage = "Invalid +AutoRhythmConsecutiveNoteChancePercentage value (range 0-100).";
					return StatusCode::InvalidAutoRhythmConsecutiveNoteChancePercentage;
				}
				_nAutoRhythmConsecutiveNoteChancePercentage = std::stoi (vKeyValue[1]);
			}
			else if (vKeyValue[0] == "AllMelodyNotes")
			{
				if (!akl::VerifyTextInteger (vKeyValue[1], nVal, 0, 1))
				{
					_sStatusMessage = "Invalid +AllMelodyNotes value (valid: 0 or 1).";
					return StatusCode::InvalidAllMelodyNotesValue;
				}
				_bAllMelodyNotes = nVal == 1;
			}
			else if (vKeyValue[0] == "AutoChordsNumBars")
			{
				bool bInvalid = false;
				int num = 0;
				try
				{
					num = stoi (vKeyValue[1]);
				}
				catch (const std::invalid_argument& e)
				{
					e;
					bInvalid = true;
				}
				if (bInvalid || (num != 2 && num != 4 && num != 8 && num != 16))
				{
					_sStatusMessage = "Invalid +AutoChordsNumBars value (valid: 2, 4, 8 or 16).";
					return StatusCode::InvalidAutoChordsNumBarsValue;
				}
				_nAutoChordsNumBars = num;
			}
			else if (vKeyValue[0] == "AutoChordsMinorChordBias")
			{
				_sAutoChordsMinorChordBias = vKeyValue[1];
				if (!ValidBiasParam (_sAutoChordsMinorChordBias, 3))
				{
					_sStatusMessage = "Invalid +_sAutoChordsMinorChordBias parameter.";
					return StatusCode::InvalidAutoChordsMinorChordBias;
				}
				else
				{
					std::vector<std::string> v = akl::Explode (_sAutoChordsMinorChordBias, ",");
					int num = 0;
					for (int i = 0; i < 3; i++)
					{
						int n = stoi (v[i]);
						_vAutoChordsMinorChordBias.push_back (n);
						num += n;
					}
					if (num > 100)
					{
						_sStatusMessage = "Invalid +_sAutoChordsMinorChordBias parameter.\n"
							"The 3 values must not exceed 100%";
						return StatusCode::InvalidAutoChordsMinorChordBias;
					}
				}
			}
			else if (vKeyValue[0] == "AutoChordsMajorChordBias")
			{
				_sAutoChordsMajorChordBias = vKeyValue[1];
				if (!ValidBiasParam (_sAutoChordsMajorChordBias, 3))
				{
					_sStatusMessage = "Invalid +_sAutoChordsMajorChordBias parameter.";
					return StatusCode::InvalidAutoChordsMajorChordBias;
				}
				else
				{
					std::vector<std::string> v = akl::Explode (_sAutoChordsMajorChordBias, ",");
					int num = 0;
					for (int i = 0; i < 3; i++) 
					{
						int n = stoi (v[i]);
						_vAutoChordsMajorChordBias.push_back (n);
						num += n;
					}
					if (num > 100)
					{
						_sStatusMessage = "Invalid +_sAutoChordsMajorChordBias parameter.\n"
							"The 3 values must not exceed 100%";
						return StatusCode::InvalidAutoChordsMajorChordBias;
					}
				}
			}
			else if (vKeyValue[0] == "AutoChordsShortNoteBiasPercent")
			{
				if (!akl::VerifyTextInteger (vKeyValue[1], nVal, 0, 100))
				{
					_sStatusMessage = "Invalid +AutoChordsShortNoteBiasPercent value (range 0-100).";
					return StatusCode::InvalidAutoChordsShortNoteBiasPercent;
				}
				_nAutoChordsShortNoteBiasPercent = nVal;
			}
			else if (vKeyValue[0] == "AutoChords_CTV_maj")
			{
				if (!akl::VerifyTextInteger (vKeyValue[1], nVal, 0, 100000))
				{
					_sStatusMessage = "Invalid +AutoChords_CTV_maj value (range 0-100000).";
					return StatusCode::InvalidAutoChordsCTV_maj_Value;
				}
				uint32_t i = static_cast<uint32_t>(ChordTypeVariation::Major);
				_vChordTypeVariationFactors[i] = nVal;
			}
			else if (vKeyValue[0] == "AutoChords_CTV_7")
			{
				if (!akl::VerifyTextInteger (vKeyValue[1], nVal, 0, 100000))
				{
					_sStatusMessage = "Invalid +AutoChords_CTV_7 value (range 0-100000).";
					return StatusCode::InvalidAutoChordsCTV_7_Value;
				}
				uint32_t i = static_cast<uint32_t>(ChordTypeVariation::Dominant_7th);
				_vChordTypeVariationFactors[i] = nVal;
			}
			else if (vKeyValue[0] == "AutoChords_CTV_maj7")
			{
				if (!akl::VerifyTextInteger (vKeyValue[1], nVal, 0, 100000))
				{
					_sStatusMessage = "Invalid +AutoChords_CTV_maj7 value (range 0-100000).";
					return StatusCode::InvalidAutoChordsCTV_maj7_Value;
				}
				uint32_t i = static_cast<uint32_t>(ChordTypeVariation::Major_7th);
				_vChordTypeVariationFactors[i] = nVal;
			}
			else if (vKeyValue[0] == "AutoChords_CTV_9")
			{
				if (!akl::VerifyTextInteger (vKeyValue[1], nVal, 0, 100000))
				{
					_sStatusMessage = "Invalid +AutoChords_CTV_9 value (range 0-100000).";
					return StatusCode::InvalidAutoChordsCTV_9_Value;
				}
				uint32_t i = static_cast<uint32_t>(ChordTypeVariation::Dominant_9th);
				_vChordTypeVariationFactors[i] = nVal;
			}
			else if (vKeyValue[0] == "AutoChords_CTV_maj9")
			{
				if (!akl::VerifyTextInteger (vKeyValue[1], nVal, 0, 100000))
				{
					_sStatusMessage = "Invalid +AutoChords_CTV_maj9 value (range 0-100000).";
					return StatusCode::InvalidAutoChordsCTV_maj9_Value;
				}
				uint32_t i = static_cast<uint32_t>(ChordTypeVariation::Major_9th);
				_vChordTypeVariationFactors[i] = nVal;
			}
			else if (vKeyValue[0] == "AutoChords_CTV_add9")
			{
				if (!akl::VerifyTextInteger (vKeyValue[1], nVal, 0, 100000))
				{
					_sStatusMessage = "Invalid +AutoChords_CTV_add9 value (range 0-100000).";
					return StatusCode::InvalidAutoChordsCTV_add9_Value;
				}
				uint32_t i = static_cast<uint32_t>(ChordTypeVariation::Add_9);
				_vChordTypeVariationFactors[i] = nVal;
			}
			else if (vKeyValue[0] == "AutoChords_CTV_sus2")
			{
				if (!akl::VerifyTextInteger (vKeyValue[1], nVal, 0, 100000))
				{
					_sStatusMessage = "Invalid +AutoChords_CTV_sus2 value (range 0-100000).";
					return StatusCode::InvalidAutoChordsCTV_sus2_Value;
				}
				uint32_t i = static_cast<uint32_t>(ChordTypeVariation::Sus_2);
				_vChordTypeVariationFactors[i] = nVal;
			}
			else if (vKeyValue[0] == "AutoChords_CTV_7sus2")
			{
				if (!akl::VerifyTextInteger (vKeyValue[1], nVal, 0, 100000))
				{
					_sStatusMessage = "Invalid +AutoChords_CTV_7sus2 value (range 0-100000).";
					return StatusCode::InvalidAutoChordsCTV_7sus2_Value;
				}
				uint32_t i = static_cast<uint32_t>(ChordTypeVariation::_7_Sus_2);
				_vChordTypeVariationFactors[i] = nVal;
			}
			else if (vKeyValue[0] == "AutoChords_CTV_sus4")
			{
				if (!akl::VerifyTextInteger (vKeyValue[1], nVal, 0, 100000))
				{
					_sStatusMessage = "Invalid +AutoChords_CTV_sus4 value (range 0-100000).";
					return StatusCode::InvalidAutoChordsCTV_sus4_Value;
				}
				uint32_t i = static_cast<uint32_t>(ChordTypeVariation::Sus_4);
				_vChordTypeVariationFactors[i] = nVal;
			}
			else if (vKeyValue[0] == "AutoChords_CTV_7sus4")
			{
				if (!akl::VerifyTextInteger (vKeyValue[1], nVal, 0, 100000))
				{
					_sStatusMessage = "Invalid +AutoChords_CTV_7sus4 value (range 0-100000).";
					return StatusCode::InvalidAutoChordsCTV_7sus4_Value;
				}
				uint32_t i = static_cast<uint32_t>(ChordTypeVariation::_7_Sus_4);
				_vChordTypeVariationFactors[i] = nVal;
			}
			else if (vKeyValue[0] == "AutoChords_CTV_min")
			{
				if (!akl::VerifyTextInteger (vKeyValue[1], nVal, 0, 100000))
				{
					_sStatusMessage = "Invalid +AutoChords_CTV_min value (range 0-100000).";
					return StatusCode::InvalidAutoChordsCTV_min_Value;
				}
				uint32_t i = static_cast<uint32_t>(ChordTypeVariation::Minor);
				_vChordTypeVariationFactors[i] = nVal;
			}
			else if (vKeyValue[0] == "AutoChords_CTV_m7")
			{
				if (!akl::VerifyTextInteger (vKeyValue[1], nVal, 0, 100000))
				{
					_sStatusMessage = "Invalid +AutoChords_CTV_m7 value (range 0-100000).";
					return StatusCode::InvalidAutoChordsCTV_m7_Value;
				}
				uint32_t i = static_cast<uint32_t>(ChordTypeVariation::Minor_7th);
				_vChordTypeVariationFactors[i] = nVal;
			}
			else if (vKeyValue[0] == "AutoChords_CTV_m9")
			{
				if (!akl::VerifyTextInteger (vKeyValue[1], nVal, 0, 100000))
				{
					_sStatusMessage = "Invalid +AutoChords_CTV_m9 value (range 0-100000).";
					return StatusCode::InvalidAutoChordsCTV_m9_Value;
				}
				uint32_t i = static_cast<uint32_t>(ChordTypeVariation::Minor_9th);
				_vChordTypeVariationFactors[i] = nVal;
			}
			else if (vKeyValue[0] == "AutoChords_CTV_madd9")
			{
				if (!akl::VerifyTextInteger (vKeyValue[1], nVal, 0, 100000))
				{
					_sStatusMessage = "Invalid +AutoChords_CTV_madd9 value (range 0-100000).";
					return StatusCode::InvalidAutoChordsCTV_madd9_Value;
				}
				uint32_t i = static_cast<uint32_t>(ChordTypeVariation::Minor_Add_9);
				_vChordTypeVariationFactors[i] = nVal;
			}
			else if (vKeyValue[0] == "AutoChords_CTV_dim7")
			{
				if (!akl::VerifyTextInteger (vKeyValue[1], nVal, 0, 100000))
				{
					_sStatusMessage = "Invalid +AutoChords_CTV_dim7 value (range 0-100000).";
					return StatusCode::InvalidAutoChordsCTV_dim7_Value;
				}
				uint32_t i = static_cast<uint32_t>(ChordTypeVariation::Dim_7th);
				_vChordTypeVariationFactors[i] = nVal;
			}
			else if (vKeyValue[0] == "AutoChords_CTV_dim")
			{
				if (!akl::VerifyTextInteger (vKeyValue[1], nVal, 0, 100000))
				{
					_sStatusMessage = "Invalid +AutoChords_CTV_dim value (range 0-100000).";
					return StatusCode::InvalidAutoChordsCTV_dim_Value;
				}
				uint32_t i = static_cast<uint32_t>(ChordTypeVariation::Dim);
				_vChordTypeVariationFactors[i] = nVal;
			}
			else if (vKeyValue[0] == "AutoChords_CTV_m7b5")
			{
				if (!akl::VerifyTextInteger (vKeyValue[1], nVal, 0, 100000))
				{
					_sStatusMessage = "Invalid +AutoChords_CTV_m7b5 value (range 0-100000).";
					return StatusCode::InvalidAutoChordsCTV_m7b5_Value;
				}
				uint32_t i = static_cast<uint32_t>(ChordTypeVariation::HalfDim);
				_vChordTypeVariationFactors[i] = nVal;
			}
			else if (vKeyValue[0] == "WriteOldRuler")
			{
				if (!akl::VerifyTextInteger (vKeyValue[1], nVal, 0, 1))
				{
					_sStatusMessage = "Invalid +WriteOldRuler value (valid: 0 or 1).";
					return StatusCode::InvalidWriteOldRuler;
				}
				_bWriteOldRuler = nVal == 1;
				if (_bWriteOldRuler)
					sRuler = sRulerOld;
			}
			else if (vKeyValue[0] == "RandomChordReplacementKey")
			{
				if (!_bAutoChords)
				{
					_sRCRKey = vKeyValue[1];
					_bRCR = true;
				}
			}
			else if (vKeyValue[0] == "AutoMelodyDontUsePentatonic")
			{
				if (!akl::VerifyTextInteger (vKeyValue[1], nVal, 0, 1))
				{
					_sStatusMessage = "Invalid +AutoMelodyDontUsePentatonic value (valid: 0 or 1).";
					return StatusCode::InvalidAutoMelodyDontUsePentatonic;
				}
				_bAutoMelodyDontUsePentatonic = nVal == 1;
			}
			else if (vKeyValue[0] == "ModalInterchangeChancePercentage")
			{
				// T2015A
				if (!akl::VerifyTextInteger (vKeyValue[1], nVal, 0, 100))
				{
					_sStatusMessage = "Invalid +ModalInterchangeChancePercentage value (range 0-100).";
					return StatusCode::InvalidModalInterchangeChancePercentage;
				}
				_nModalInterchangeChancePercentage = std::stoi (vKeyValue[1]);
			}
			else
			{
				std::string p = sLine.substr (1, sTemp.size() - 1);
				_sStatusMessage = "Unrecognized command file parameter: +" + vKV2[0];
				return StatusCode::UnrecognizedCommandFileParameter;
			}

			continue;
		}

		if (sLine.substr (0, 32) == sRulerOld || sLine.substr (0, 31) == sRulerNew.substr (0, 31))
		{
			// Ruler line. You are able to use either of the two ruler types.
			// The next two lines should contain
			// (1) Note Positions: series of hash groups
			// (2) Comma-separated Chord Names, one for each of the Note Position hash groups.

			nRulerLen = sLine.length();

			// When the alt ruler - the one showing 1/16ths - is used, the user might not
			// have put a space at the end, so we add one here.
			if (nRulerLen % 32 == 31)
			{
				sLine += " ";
				nRulerLen = sLine.length();
			}

			if (nRulerLen % 32 != 0)
			{
				std::ostringstream ss;
				ss << "Line " << nLineNum << ": Invalid ruler line.";
				_sStatusMessage = ss.str();
				return StatusCode::InvalidRulerLine;
			}

			uint32_t nNumBars = sLine.length() / 32;
			for (uint32_t i = 1; i < nNumBars; i++)
			{
				if (sLine.substr (i * 32, 32) != sRulerOld && sLine.substr (i * 32, 32) != sRulerNew)
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
			_vRulerLineInFile.push_back (nLineNum);

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

	// Auto-Melody: If specified, the melody line can include a few instances
	// of the additional notes from the pentatonic scale of the chord.
	if (!_bAutoMelodyDontUsePentatonic)
	{
		for (int i = 0; i < 2; i++)
		{
			// Major chords
			_mMelodyNotes["maj"].push_back (2);
			_mMelodyNotes["maj"].push_back (9);
			_mMelodyNotes["7"].push_back (2);
			_mMelodyNotes["7"].push_back (9);
			_mMelodyNotes["maj7"].push_back (2);
			_mMelodyNotes["maj7"].push_back (9);
			_mMelodyNotes["9"].push_back (2);
			_mMelodyNotes["9"].push_back (9);
			_mMelodyNotes["maj9"].push_back (2);
			_mMelodyNotes["maj9"].push_back (9);
			_mMelodyNotes["add9"].push_back (2);
			_mMelodyNotes["add9"].push_back (9);

			// Minor chords
			_mMelodyNotes["m"].push_back (5);
			_mMelodyNotes["m"].push_back (10);
			_mMelodyNotes["m7"].push_back (5);
			_mMelodyNotes["m7"].push_back (10);
			_mMelodyNotes["m9"].push_back (5);
			_mMelodyNotes["m9"].push_back (10);
			_mMelodyNotes["madd9"].push_back (5);
			_mMelodyNotes["madd9"].push_back (10);
		}
	}

	return result;
}

CMIDIHandler::StatusCode CMIDIHandler::CopyFileWithAutoRhythm (std::string filename, bool bOverwriteOutFile)
{
	StatusCode nRes = StatusCode::Success;

	if (!bOverwriteOutFile && akl::MyFileExists (filename))
	{
		std::ostringstream ss;
		ss << "Output file already exists. Use the -o switch to overwrite, eg:\n"
			<< "SMFFTI.exe -ar mymidi.txt mymidi_rhythm.txt -o";
		_sStatusMessage = ss.str();
		return StatusCode::OutputFileAlreadyExists;
	}

	//--------------------------------------------------------------------------
	// Init randomizer vectors
	auto InitRandVectors = [](std::string s)
	{
		std::vector<uint32_t> vOut;
		std::vector<std::string> v = akl::Explode (s, ",");
		uint32_t nVal = 32;
		for (uint8_t i = 0; i < 6; i++)
		{
			uint32_t nCount = std::stoi (v[i]);
			for (uint32_t j = 0; j < nCount; j++)
				vOut.push_back (nVal);

			nVal = nVal >> 1;
		}
		return vOut;
	};
	std::vector<uint32_t> vNoteLenChoice = InitRandVectors (_sAutoRhythmNoteLenBias);
	std::vector<uint32_t> vGapLenChoice = InitRandVectors (_sAutoRhythmGapLenBias);

	std::vector<uint32_t> vNoteOrGap (100);
	for (uint32_t i = 0; i < _nAutoRhythmConsecutiveNoteChancePercentage; i++)
		vNoteOrGap[i] = 1;
	//--------------------------------------------------------------------------

	std::uniform_int_distribution<uint32_t> randNoteOrGap (0, vNoteOrGap.size() - 1);

	// Lambda: Serves to return a length value for either a note position or a gap.
	//
	// maxNoteLen: The theoretical largest note length permitted - this is dependent upon where
	// in the bar the note position is, eg. a whole note is possible only if the note
	// position is right at the start of a bar.
	//
	// maxLen: The actual maximum length allowed. For instance, although a whole note might
	// be possible, if another chord follows in the same bar, there won't be enough room
	// for a whole note. maxLen reflects this.
	//
	// vChoice: The vector of numbers representing the choice of random values possible.
	auto RandLen = [&](uint8_t maxNoteLen, uint8_t maxLen, std::vector<uint32_t> vChoice)
	{
		uint32_t nIndex = 0;
		for (uint32_t i = 0; i < vChoice.size(); i++)
		{
			if (vChoice[i] == maxNoteLen)
			{
				nIndex = i;
				break;
			}
		}
		std::uniform_int_distribution<uint32_t> randChoice (nIndex, vChoice.size() - 1);
		uint8_t nLen;
		do
			nLen = vChoice[randChoice (_eng)];
		while (nLen > maxLen);

		return nLen;
	};

	uint32_t nItem = 0;
	uint32_t nChordNameIndex = 0;
	std::vector<std::string> vNewChordList;
	for (auto& it : _vNotePositions)
	{
		// Pad out string if ness.
		while (it.size() < _vBarCount[nItem] * 32)
			it.push_back (' ');

		// Locate + signs.
		std::vector<uint8_t> vPlusSignPos;
		std::vector<uint8_t> vChordRepCount;
		uint32_t n = 0;
		for (auto c : it)
		{
			if (c == '+')
			{
				vPlusSignPos.push_back (n);
				vChordRepCount.push_back (0);
			}
			n++;
		}

		for (uint32_t i = 0; i < vPlusSignPos.size(); i++)
		{
			uint8_t nStart = vPlusSignPos[i];
			uint8_t nEnd = (i < vPlusSignPos.size() - 1) ?
				vPlusSignPos[i + 1] : (uint8_t)it.size();
			//vPlusSignPos[i + 1] : (uint8_t)it.size() - 1;

			uint32_t j = nStart;
			bool bNoteOn = true, bPrevNoteOn = false;
			while (j < nEnd)
			{
				uint8_t maxLen = nEnd - j;
				uint8_t nLargestNoteLen;

				if (j % 32 == 0)			// whole note position
					nLargestNoteLen = 32;
				else if (j % 16 == 0)		// 1/2 note position
					nLargestNoteLen = 16;
				else if (j % 8 == 0)		// 1/4 note position
					nLargestNoteLen = 8;
				else if (j % 4 == 0)		// 1/8 note position
					nLargestNoteLen = 4;
				else if (j % 2 == 0)		// 1/16th note position
					nLargestNoteLen = 2;
				else						// 1/32nd note position
					nLargestNoteLen = 1;

				// Randomize whether to create a note, or a gap.
				// *Always* place note at start of bar.
				bNoteOn = j == nStart ? 1 : vNoteOrGap[randNoteOrGap (_eng)];

				// NB. No consecutive gaps. Consecutive notes, yes, (if _sAutoRhythmNoteChancePercentage non-zero)
				// but not gaps. Sussing this was a bit improvement. So remember, Andrew, a value of zero fof
				// AutoRhythmNoteConsecutiveChancePercentage does NOT mean NO GAPS (which would be a bit 
				// pointless anyway) - it means what it says: No consecutive notes; there will always be
				// a gap in between, thanks to this critical condition here.
				if (!bPrevNoteOn)
					bNoteOn = true;

				// No notes starting on off-beat 1/32nds; 
				// that is, all notes starting on even-numbered 1/32nds.
				// (This is IMPORTANT for proper syncopation.)
				if (nLargestNoteLen == 1)
					bNoteOn = false;

				uint8_t nl = RandLen (nLargestNoteLen, maxLen, bNoteOn ? vNoteLenChoice : vGapLenChoice);

				// Output the note/gap chars.
				uint32_t m = j + nl;
				char ch = bNoteOn ? '+' : ' ';
				for (uint32_t k = j; k < m; k++)
				{
					it[k] = ch;
					ch = bNoteOn ? '#' : ' ';
				}

				if (bNoteOn)
					vChordRepCount[i]++;

				bPrevNoteOn = bNoteOn;

				j = m;
			}
		}

		std::string newChordList;
		std::string comma;
		for (auto c : vChordRepCount)
		{
			_vChordNames[nChordNameIndex] += "(" + std::to_string (c) + ")";
			newChordList += comma + _vChordNames[nChordNameIndex];
			comma = ", ";
			nChordNameIndex++;
		}
		vNewChordList.push_back (newChordList);

		nItem++;
	}

	// Output copy of the input file with the generated rhythm.
	std::ofstream ofs (filename, std::ios::out);
	uint32_t nLine = 1;
	uint32_t nLine2 = 0;
	uint32_t iNewChordList = 0;
	bool bIgnoreNextLine = false;
	for (auto s : _vInput)
	{
		if (bIgnoreNextLine)
		{
			bIgnoreNextLine = false;
			nLine++;
			continue;
		}

		if (nLine2 < _vNotePosLineInFile.size() && nLine == _vNotePosLineInFile[nLine2])
		{
			// Output the rhythm version of note positions.
			ofs << _vNotePositions[nLine2++] << std::endl;
			ofs << vNewChordList[iNewChordList++] << std::endl;
			bIgnoreNextLine = true;
		}
		else
			ofs << s << std::endl;

		nLine++;
	}
	ofs.close();

	return nRes;
}

// 2303090953 Auto-Chords: Generate copy of original command file, complete with 4-bar sequence
// of randomized chords and note lengths.
CMIDIHandler::StatusCode CMIDIHandler::CopyFileWithAutoChords (std::string filename, bool bOverwriteOutFile)
{
	StatusCode nRes = StatusCode::Success;

	if (!bOverwriteOutFile && akl::MyFileExists (filename))
	{
		std::ostringstream ss;
		ss << "Output file already exists. Use the -o switch to overwrite, eg:\n"
			<< "SMFFTI.exe -ac mymidi.txt mymidi_ac.txt -o";
		_sStatusMessage = ss.str();
		return StatusCode::OutputFileAlreadyExists;
	}

	// T2015A
	_bRCR = false;
	InitChordBank (_vChordNames[0]);
	std::uniform_int_distribution<uint16_t> randModInt (0, 99);

	//--------------------------------------------------------------------------
	// Output copy of the input file with the generated rhythm.
	std::ofstream ofs (filename, std::ios::out);
	uint32_t nLine = 1;
	uint32_t nLine2 = 0;
	uint32_t iNewChordList = 0;
	uint32_t nIgnoreLines = 0;

	uint8_t nNumBarsPerLine = _nAutoChordsNumBars == 2 ? 2 : 4;

	for (auto s : _vInput)
	{
		if (nIgnoreLines > 0)
		{
			nIgnoreLines--;
			nLine++;
			continue;
		}

		if (nLine2 < _vRulerLineInFile.size() && nLine == _vRulerLineInFile[nLine2])
		{
			for (int iFred = 0; iFred < (_nAutoChordsNumBars / nNumBarsPerLine); iFred++)
			{
				if (iFred > 0)
					ofs << "\n";

				for (int i = 0; i < nNumBarsPerLine; i++)
					ofs << sRuler;
				ofs << "\n";

				// Object that generates the random rhythm pattern, specifying a short note
				// bias percentage (0 means no short notes; 100 means _all_ short notes).
				// (Short notes being 16 32nds or shorter.)
				CAutoRhythm autoRhythm (_nAutoChordsShortNoteBiasPercent);
				uint32_t nNoteCount;
				std::string sPattern = autoRhythm.GetPattern (nNoteCount, nNumBarsPerLine * 32);

				// Output the rhythm version of note positions.
				ofs << sPattern << std::endl;

				std::string sComma = "";
				for (uint32_t k = 0; k < nNoteCount; k++)
				{
					// T2015A Which chord bank to use - the main one or the Modal Interchange one.
					uint16_t iRandModInt = randModInt (_eng);
					uint8_t iCB = _vChordBankChoice[iRandModInt];

					// This sets the chosen random chord in the CChordBank object;
					// you then have to interrogate it to retrieve the actual chord value.
					_vChordBank[iCB]->SetRandomChord();

					// Now apply a possible random chord type variation.
					std::string sChordVar = _vChordBank[iCB]->GetChordVariation();

					std::string sChordName = _vChordBank[iCB]->GetChordName();
					ofs << sComma << sChordName << sChordVar;
					sComma = ", ";
				}
				ofs << std::endl;

			}

			nIgnoreLines = 2;
		}
		else
			ofs << s << std::endl;

		nLine++;
	}
	ofs.close();

	return nRes;
}

CMIDIHandler::StatusCode CMIDIHandler::ConvertMIDIToSMFFTI (std::string inFile, std::string outFile, bool bOverwriteOutFile)
{
	StatusCode nRes = StatusCode::Success;

	struct NoteEvent
	{
		bool bOn;
		uint16_t nTime;
		uint16_t nNoteNum;

		NoteEvent (bool bOn_, uint16_t t, uint16_t n) : bOn (bOn_), nTime (t), nNoteNum (n) {}
	};
	std::vector<NoteEvent> vNoteEvents;

	// T2O4GU Holds chord details that have been extracted from MIDI file.
	struct ChordDetails
	{
		uint16_t nStart;    // where the chord begins, in 1/32nds.
		uint16_t nEnd;
		std::vector<uint16_t> vNoteNumber;   // eg. 60 = C3

		ChordDetails (uint16_t nS, uint16_t nE, uint16_t nNote)
		{
			nStart = nS;
			nEnd = nE;
			vNoteNumber.push_back (nNote);
		}
	};
	std::vector<ChordDetails> vChordDetails;  // List of chords.

	// Determines the length of the chord.
	//      0 = Length of first (earliest) note.
	//      1 = Maximize: From start of first Note On to last Note Off.
	//      2 = Minimize: From last Note On to first Note Off, ie. length of overlap of all notes.
	int chordLengthType = 0;

    std::string chunkType (4, '\0');
    uint32_t n32;
    uint16_t n16;
    uint16_t nNumberTracks = 0;

    auto Swap32 = [](uint32_t n)
    {
        return (((n >> 24) & 0xff) | ((n << 8) & 0xff0000) | ((n >> 8) & 0xff00) | ((n << 24) & 0xff000000));
    };

    auto Swap16 = [](uint16_t n)
    {
        return ((n >> 8) | (n << 8));
    };

    // Assembles a variable-length variable (from the track buffer)
    auto ReadValue = [](std::vector<char> vBuf, uint32_t& pos)
    {
        uint32_t nValue = 0;
        uint8_t nByte = 0;

        nValue = vBuf[pos++];

        if (nValue & 0x80)
        {
            // Extract bottom 7 bits of byte
            nValue &= 0x7F;

            do
            {
                // Next byte
                nByte = vBuf[pos++];

                // Construct value by setting bottom 7 bits, then shifting 7 bits.
                nValue = (nValue << 7) | (nByte & 0x7F);

            } while (nByte & 0x80);

        }

        return nValue;
    };

    auto ReadString = [](std::vector<char> vBuf, uint32_t nLen, uint32_t& pos)
    {
        std::string s (vBuf.begin() + pos, vBuf.begin() + pos + nLen);
        pos += nLen;
        s.erase (s.find_last_not_of ('\0') + 1);            // strip trailing nulls
        s.erase (s.find_last_not_of (" \t\n\r\f\v") + 1);   // strip trailing whitespace
        return s;
    };

    // Convert, say, 5 to "0000000000000101"
    auto BitString8 = [](uint8_t n)
    {
        std::string x;
        for (int i = 7; i >= 0; i--)
        {
            char a = n & (1 << i) ? '1' : '0';
            x += a;
        }
        return x;
    };

	std::ifstream ifs (inFile, std::fstream::in | std::ios::binary);

    // ------------------------------------------------------------------------------
    // HEADER CHUNK
    ifs.read (&chunkType[0], 4);
    if (chunkType == "MThd")
    {
        // get length of header - should be 6.
        ifs.read ((char*)&n32, 4);
        uint32_t hdrLen = Swap32 (n32);

        // Header data consists of 3 words:
        //
        //  1. Format:
        //      0 = single track
        //      1 = one or more simultaneous tracks
        //      2 = one or more sequential indepedent single-track patterns.
        //
        //  2. No. Tracks
        //
        //  3. Divsion, which specifies meaning of delta-times:
        //
        //       bit 15       bits 14 thru 8             bits 7 thru 0
        //         0          ------- ticks per quarter note ---------
        //         1          negative SMPTE format      ticks per frame

        ifs.read ((char*)&n16, 2);
        uint16_t nFileFormat = Swap16 (n16);

		// Only single track files allowed.
		if (nFileFormat != 0)
		{
			std::ostringstream ss;
			ss << "MIDI file invalid for this operation. Only single-track MIDI files can be used.";
			_sStatusMessage = ss.str();
			return StatusCode::InvalidMIDIFile;
		}

        ifs.read ((char*)&n16, 2);
        nNumberTracks = Swap16 (n16);

        ifs.read ((char*)&n16, 2);
        uint16_t nDivision = Swap16 (n16);
    }

    // ------------------------------------------------------------------------------
    // TRACK CHUNKS
    for (uint16_t nTrack = 0; nTrack < nNumberTracks; nTrack++)
    {
        ifs.read (&chunkType[0], 4);
        if (chunkType == "MTrk")
        {
            // get length of track data.
            ifs.read ((char*)&n32, 4);
            uint32_t trkLen = Swap32 (n32);

            // Read all track data into buffer.
            std::vector<char> trackBuf (trkLen);
            ifs.read (&trackBuf[0], trkLen);

            uint32_t offset = 0;

            // Now we're dealing with a series of <delta-time><event> pairs.
            bool bEndOfTrack = false;
            uint8_t nPreviousStatus = 0;

            uint32_t nTotalTime = 0;

            while (offset < trkLen && !bEndOfTrack)
            {
                uint32_t nDeltaTime = 0;
                uint8_t nStatus = 0;    // Event type, ie. MIDI, SysEx or Meta.

                // Delta-time is variable length.
                nDeltaTime = ReadValue (trackBuf, offset);

                nTotalTime += nDeltaTime;

                nStatus = trackBuf[offset++];
                
                // "Running Status": There might not always be a status value -
                // we apply the previous one. Revert the offset in order to read
                // the event data.
                if (nStatus < 0x80)
                {
                    nStatus = nPreviousStatus;
                    offset--;
                }

				// For the purposes of interpreting MIDI clips for conversion
				// to SMFFTI command lines, we're interested in only a few
				// relevant MIDI events...
                if ((nStatus & 0xF0) == (uint8_t)EventName::NoteOff)
                {
                    nPreviousStatus = nStatus;
                    uint16_t nChannel = nStatus & 0x0F;
                    uint16_t nNoteId = trackBuf[offset++];
                    uint16_t nNoteVelocity = trackBuf[offset++];

                    // Add to events vector.
                    vNoteEvents.push_back (NoteEvent (false, nTotalTime, nNoteId));
                }
                else if ((nStatus & 0xF0) == (uint8_t)EventName::NoteOn)
                {
                    nPreviousStatus = nStatus;
                    uint16_t nChannel = nStatus & 0x0F;
                    uint16_t nNoteId = trackBuf[offset++];
                    uint16_t nNoteVelocity = trackBuf[offset++];

                    // Add to events vector.
                    vNoteEvents.push_back (NoteEvent (true, nTotalTime, nNoteId));
                }
                else if ((nStatus & 0xF0) == (uint8_t)EventName::SysEx)
                {
                    nPreviousStatus = nStatus;

                    if (nStatus == 0xFF)
                    {
                        // Meta Message
                        uint8_t nType = trackBuf[offset++];
                        uint8_t nLength = ReadValue(trackBuf, offset);

                        if (nType == (uint8_t)MetaEventName::MetaSequence)
                        {
                            // What is the byte-order of this 16-bit integer?
                            // Assume LSB, so swap bytes.
                            uint8_t nLSB = trackBuf[offset++];
                            uint8_t nMSB = trackBuf[offset++];
                            uint16_t nValue = (nMSB << 8) | nLSB;
                        }
                        else if (nType == (uint8_t)MetaEventName::MetaTrackName)
                        {
                            std::string sText = ReadString (trackBuf, nLength, offset);
                        }
                        else if (nType == (uint8_t)MetaEventName::MetaEndOfTrack)
                        {
                            bEndOfTrack = true;
                            continue;
                        }
                        else if (nType == (uint8_t)MetaEventName::MetaTimeSignature)
                        {
                            uint16_t nn = trackBuf[offset++];
                            uint16_t x = trackBuf[offset];
                            uint16_t dd = (1 << trackBuf[offset++]);
                            uint16_t cc = trackBuf[offset++];
                            uint16_t bb = trackBuf[offset++];
                        }
                        else
                        {
                            // Unknown meta event.
                            offset += nLength;
                        }
                    }
                }
                else
                {
                    std::cout << "Unrecognised Event Type: " << BitString8 (nStatus) << std::endl;
                }
            }

            // check if EOF reached.
            if (ifs.peek() == EOF)
                int ak = 1;
        }
    }
    ifs.close();

    // Now parse the Note Event list to identify when each note starts and ends.
    uint32_t iEvent = 0;
    while (iEvent < vNoteEvents.size())
    {
        // Our primary objective is to locate when notes begin.
        if (vNoteEvents[iEvent].bOn == false)
        {
            iEvent++;
            continue;
        }

        // Now read forward to find where this note ends.
        for (uint32_t i = iEvent + 1; i < vNoteEvents.size(); i++)
        {
            // Here we ignore Note On events.
            if (vNoteEvents[i].bOn)
                continue;
            
            if (vNoteEvents[i].nNoteNum == vNoteEvents[iEvent].nNoteNum)
            {
                // The start and end of notes in the chord may be slightly offset,
                // so quantize start and end of notes to 1/32nds (12 ticks per 1/32nd)
                // in order to group notes into chords.
                float fStart = std::ceil ((vNoteEvents[iEvent].nTime / 12.0f) - 0.6f);
                float fEnd = std::ceil ((vNoteEvents[i].nTime / 12.0f) - 0.6f);
                //float fLen = fEnd - fStart;

                uint32_t nStart = (uint32_t)fStart;
                uint32_t nEnd = (uint32_t)fEnd;
                //uint32_t nLen = (uint32_t)fLen;

                // See if vChordDetails already has any notes for this "chord".
                // "Chord" is defined as any group of notes which overlap.
                bool bExistingChord = false;
                for (std::vector<ChordDetails>::iterator it = vChordDetails.begin(); it < vChordDetails.end(); it++)
            	{
                    if (nStart >= it->nStart && nStart < it->nEnd)
                    {
                        // Yes, it does, so add this note to the chord
                        bExistingChord = true;
                        it->vNoteNumber.push_back (vNoteEvents[iEvent].nNoteNum);

                        if (chordLengthType == 1)
                        {
                            // Maximize
                            if (nEnd > it->nEnd)
                                it->nEnd = nEnd;
                        }
                        else if (chordLengthType == 2)
                        {
                            // Minimize
                            it->nStart = nStart;
                            if (nEnd < it->nEnd)
                                it->nEnd = nEnd;
                        }

                        break;
                    }
                }

                if (!bExistingChord)
                    vChordDetails.push_back (ChordDetails (nStart, nEnd, vNoteEvents[i].nNoteNum));

                break;
            }
        }

        iEvent++;
    };

    // Output the chords in SMFFTI format.
	if (!bOverwriteOutFile && akl::MyFileExists (outFile))
	{
		std::ostringstream ss;
		ss << "Output file already exists. Use the -o switch to append to this file, eg:\n"
			<< "SMFFTI.exe -m mymidi.mid mymidi.txt -o";
		_sStatusMessage = ss.str();
		return StatusCode::OutputFileAlreadyExists;
	}

	std::vector<std::string> vOutFile;

	// Load existing content of output file if it exists.
	if (akl::MyFileExists (outFile))
		akl::LoadTextFileIntoVector (outFile, vOutFile);

	// Convert notes to relative note intervals.
	// eg. From "60, 63, 67" to "0, 3, 7".
	auto SetNoteZeroOffset = [](std::vector<uint16_t>& vNoteIntervals)
    {
		uint16_t nLowestNote = vNoteIntervals[0];
		for (uint16_t i = 0; i < vNoteIntervals.size(); i++)
			vNoteIntervals[i] -= nLowestNote;
    };

	std::vector<char> vOutStrChordPos;
	std::vector<std::string> vChordName;
	uint32_t n32ndPos = 0;

	// For the sake of identifying chord types, we need to ensure
	// all notes are contained within a single octave, so here we
	// do some necessary downward transposing.
	_mChordTypes["9"] = "2,4,7,10";
	_mChordTypes["maj9"] = "2,4,7,11";
	_mChordTypes["add9"] = "2,4,7";
	_mChordTypes["m9"] = "2,3,7,10";
	_mChordTypes["madd9"] = "2,3,7";

	// Loop through chords in the progression.
    for each (auto c in vChordDetails)
    {
		// Sort the notes from lowest to highest.
		// vNotes is used to identify the chord TYPE.
		std::vector<uint16_t> vNotes (c.vNoteNumber);
		std::sort (vNotes.begin(), vNotes.end());

		// Remove all duplicate notes, ie. same note one or more octaves higher.
		uint16_t iStartNote = 0;
		while (iStartNote < vNotes.size() - 1)
		{
			bool bDuplicate = false;
			uint16_t n1 = vNotes[iStartNote] % 12;
			for (uint16_t i = iStartNote + 1; i < vNotes.size(); i++)
			{
				uint16_t n2 = vNotes[i] % 12;
				if (n1 == n2)
				{
					vNotes.erase (vNotes.begin() + iStartNote);
					bDuplicate = true;
					break;
				}
			}

			if (!bDuplicate)
				iStartNote++;
		}

		// Second copy of notes vector which is used to identify the chord NAME.
		std::vector<uint16_t> vNotes2 (vNotes);

		// Check if the notes, in their current order, match a chord type...
		std::string sChordType;
		bool bMinor;
		for (int i = 0; i < 10; i++)	// max 10 attempt, easily plenty
		{
			SetNoteZeroOffset (vNotes);
			sChordType = IsValidChordType (vNotes, bMinor);
			if (sChordType.length())
				break;

			// No match yet, so transpose the lowest note up an octave and try again.
			vNotes[0] += 12;
			std::sort (vNotes.begin(), vNotes.end());
			vNotes2.push_back (vNotes2[0] + 12);
			vNotes2.erase (vNotes2.begin());
			std::sort (vNotes2.begin(), vNotes2.end());
		}

		if (sChordType.length() == 0)
		{
			// Abort
			std::ostringstream ss;
			ss << "Invalid chord in MIDI file: Chord no. " << vChordName.size() + 1;
			_sStatusMessage = ss.str();
			return StatusCode::InvalidChordInMidiFile;
		}

		// Hack: We don't need "maj" appearing for major chords.
		if (sChordType == "maj")
			sChordType = "";

		// Identify the name of the chord.
		uint16_t nRoot = (vNotes2[0] % 12);
		std::string sRoot;
		for (const auto& pair : _mChromaticScale2)
		{
			if (nRoot == pair.second)
			{
				sRoot = pair.first;
				break;
			}
		}
		std::string sChordName = sRoot + sChordType;


		// Now append chord position/name to output strings.
		// Each character in the output string represents a 1/32nd note.
		//
		// First, go to where the chord begins.
        while (n32ndPos < c.nStart)
        {
			vOutStrChordPos.push_back (' ');
            n32ndPos++;
        }
		//
		// Output chord length.
        uint32_t nLen = c.nEnd - c.nStart;
        for (uint32_t i = 0; i < nLen; i++)
        {
   			vOutStrChordPos.push_back (i == 0 ? '+' : '#');
            n32ndPos++;
        }
		//
		// Save chord name
		vChordName.push_back (sChordName);

    };

	// Any progression longer than 2 bars will be split into 4-bar sections.
	size_t nChordPosLen = vOutStrChordPos.size();
	std::string sRuler = "$ . . . | . . . | . . . | . . . ";
	if (nChordPosLen > 32)
		sRuler += sRuler;
	if (nChordPosLen > 64)
		sRuler += sRuler;

	uint32_t nRulerLen = sRuler.length();

	auto PadCharVector = [](std::vector<char>& vCh, uint32_t nBufLen)
    {
		// vCh is the vector to be padded out.
		// nBufLen is a length, eg. 32, such that the vector will be padded
		// to the next multiple of nBufLen.
		uint32_t nLen = vCh.size();
		uint32_t nLen2 = (((nLen - 1) / nBufLen) + 1) * nBufLen;
		uint32_t nPad = nLen2 - nLen;
		for (uint32_t i = 0; i < nPad; i++)
			vCh.push_back (' ');
    };

	// Pad the output strings with spaces to give them a 
	// length corresponding to a multiple of the ruler length.
	PadCharVector (vOutStrChordPos, nRulerLen);

	auto CountOccurrences = [](const std::string& str, const std::string& target)
	{
		uint32_t count = 0;
		uint32_t pos = 0;
		while ((pos = str.find(target, pos)) != std::string::npos)
		{
			count++;
			pos += target.length();
		}
		return count;
	};

	uint32_t n32ndCount = n32ndPos;
	n32ndPos = 0;
	std::string vLine;
	uint32_t iChord = 0;

	//std::string sTime = akl::TimeStamp();
	//vOutFile.push_back ("\n------------------------------------------------");
	//vOutFile.push_back ("# Timestamp: " + sTime);

	while (n32ndPos < n32ndCount)
	{
		vOutFile.push_back ("");
		vOutFile.push_back (sRuler);

		vLine = std::string (vOutStrChordPos.begin() + n32ndPos, vOutStrChordPos.begin() + n32ndPos + nRulerLen);
		vOutFile.push_back (vLine);

		// Output chord names appearing in this line.
		uint32_t nPlusCount = CountOccurrences (vLine, "+"); // No. chords appearing in this line.
		vLine.clear();
		std::string sComma = "";
		for (uint32_t i = 0; i < nPlusCount; i++)
		{
			vLine += sComma + vChordName[iChord++];
			sComma = ", ";
		}
		vOutFile.push_back (vLine);

		n32ndPos += nRulerLen;
	}

	akl::WriteVectorToTextFile (outFile, vOutFile);
	return nRes;
}

std::string CMIDIHandler::IsValidChordType (const std::vector<uint16_t>& vNotes, bool& bMinor)
{
	std::string sChordType = "";
	bMinor = false;

	std::string comma = "";
	std::ostringstream ss;
	for (uint16_t i = 1; i < vNotes.size(); i++)
	{
		ss << comma << vNotes[i];
		comma = ",";
	}
	std::string sIntervals = ss.str();

	for (const auto& pair : _mChordTypes)
	{
		if (sIntervals == pair.second)
		{
			sChordType = pair.first;

			if (sIntervals[0] == '3')
				bMinor = true;

			break;
		}
    }

	return sChordType;
}


CMIDIHandler::StatusCode CMIDIHandler::GenRandMelodies (std::string filename, bool bOverwriteOutFile)
{
	StatusCode nRes = StatusCode::Success;

	if (!bOverwriteOutFile && akl::MyFileExists (filename))
	{
		std::ostringstream ss;
		ss << "Output file already exists. Use the -o switch to overwrite, eg:\n"
			<< "SMFFTI.exe -grm generic_rand_melodies.txt -o";
		_sStatusMessage = ss.str();
		return StatusCode::OutputFileAlreadyExists;
	}

	// Major Pentatonic intervals. Since SMFFTI auto-corrects notes according to whether
	// they are in major or minor key, we don't have to worry about specifying the
	// Minor Pentatonic for this operation.
	std::vector<uint8_t> vNotes = { 0, 2, 4, 7, 9 };
	std::uniform_int_distribution<uint32_t> randNote (0, vNotes.size() - 1);

	std::ofstream ofs (filename, std::ios::out);

	ofs << "# Generic Randomized Melody lines. Generated by SMFFTI (-grm) at "
		<< akl::TimeStamp() << "\n\n";

	for (size_t i = 0; i < 1000; i++)
	{
		ofs << "#Melody " << i << "\nM: ";
		std::string comma;
		for (size_t j = 0; j < 64; j++)
		{
			uint16_t rn = vNotes[randNote (_eng)];
			ofs << comma << rn;
			comma = ", ";
		}
		ofs << "\n\n";
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

	if (_bRCR)
		akl::WriteVectorToTextFile (_sInputFile, _vInputCopy);

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
			}
			else if (i % 2 == 0)
			{
				// 1/8th note position
				sNoteLen = RandNoteLen ({ 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2 }, nNoteLen16ths);	// Off, 1/16th, 1/8th or 3/8ths
			}
			else
			{
				// Odd-numbered 1/16th note position,
				// ie. an upstroke.
				sNoteLen = RandNoteLen ({ 0, 0, 0, 0, 0, 1, 1, 1, 1 }, nNoteLen16ths);		// Off or 1/16th
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


	// Melody Mode: Save the melody to timestamped file
	// so it can be reused.
	std::ofstream ofs;
	std::string sMelodySaveFile = _sInputFile;
	if (_bAutoMelody)
	{
		std::string ts = akl::TimeStamp();
		std::string::size_type pos = _sInputFile.find_last_of ('.');
		if (pos == std::wstring::npos)
		{
			pos = _sInputFile.size();
			ts += ".txt";
		}

		sMelodySaveFile.insert (pos, "_" + ts);
		ofs.open (sMelodySaveFile, std::ios::out);

		std::string fname = PathFindFileNameA (&sMelodySaveFile[0]);
		ofs << "+TrackName = " << fname << "\n\n";
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

			// Lambda
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


			//---------------------------------------------------------------------
			// Dump the melody notes to file so user can copy the melody.
			if (i == 1 && _bAutoMelody)
			{
				for (size_t j = 0; j < _vBarCount[nItem]; j++)
					ofs << sRuler;
				ofs << "\n";
				ofs << s << std::endl;


				// Construct list of chords
				uint32_t nC = 0;
				std::string cn;	// chord name list, eg. "C, Am, F, G"
				std::string comma;
				std::vector<std::string> vNotePosItems = TokenizeNotePosStr (s);
				for (auto e : vNotePosItems)
				{
					if (e[0] == '+')
					{
						cn += comma + _vMelodyChordNames[nC];
						comma = ", ";
					}
					nC++;
				}
				ofs << cn << std::endl;

				// Construct sequence of semitone intervals representing the melody
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
			//---------------------------------------------------------------------


			// move pointer 4 bars forward
			if (i == 1)
				nBar += _vBarCount[nItem];

			nItem++;
		}

		_nNoteCount = nNote;
	}

	if (_bAutoMelody)
		ofs.close();
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
	std::string sChordType;
	GetChordIntervals (chordName, nRoot, vChordIntervals, sChordType);


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



	// AutoMelody: Instead of chords, output a random note
	// from the Major/Minor Pentatonic scale of the chord.
	// (No position offset is applicable.)
	if (_bAutoMelody)
	{
		// Notes (semitone intervals) that can be used in the melody.
		// Essentially, Major or Minor Pentatonic.
		auto it = _mMelodyNotes.find (sChordType);
		std::vector<uint8_t> vNotes = it->second;

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

		notePosOffset = (bNoteOn ? fnRandStart (nNoteSeq == 0) : fnRandEnd (nNoteSeq == _nNoteCount));
		nET = nEventTime + notePosOffset;
		MIDINote note (nNoteSeq, nET, nEventType, nNote, fnRandVel());
		_vMIDINoteEvents.push_back (note);
		return;
	}

	// Output ALL possible melody notes. For major/minor chords, this will be the pentatonic;
	// in the case of suspended/diminished chords, it will just be the chord notes.
	if (_bAllMelodyNotes)
	{
 		auto it = _mMelodyNotes.find (sChordType);
		std::vector<uint8_t> vNotes = it->second;
		uint8_t nLastNote = 127;
		for each (auto nSemitones in vNotes)
		{
			if (nSemitones == nLastNote)
				continue;

			// Downward transposition occurs if note is higher than highest-note threshold, or 127.
			uint16_t nNote = nRoot + nSemitones;
			while (nNote > (_nProvisionalLowestNote + _nTransposeThreshold) || nNote > 127)
				nNote -= 12;

			notePosOffset = (bNoteOn ? fnRandStart (nNoteSeq == 0) : fnRandEnd (nNoteSeq == _nNoteCount));

			nET = nEventTime + notePosOffset;

			MIDINote note (nNoteSeq, nET, nEventType, (uint8_t)nNote, fnRandVel());
			_vMIDINoteEvents.push_back (note);

			nLastNote = nSemitones;
		};
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
	if (_bRootNoteOnly == false)
	{
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

bool CMIDIHandler::GetChordIntervals (std::string sChordName, uint8_t& nRoot, 
	std::vector<std::string>& vChordIntervals, std::string& sChordType)
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

	vChordIntervals = akl::Explode (it->second, ",");
	sChordType = it->first;

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

std::vector<std::string> CMIDIHandler::TokenizeNotePosStr (std::string notePosStr)
{
	// Parse a string like "+## ##### ##+## # # ####+## # #" and
	// put all the items in a vector.

	std::vector<std::string> vT;

	notePosStr = akl::RemoveWhitespace (notePosStr, 8);

	bool bGroup = false;
	std::string sTemp;
	for (auto c : notePosStr)
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
				vT.push_back (sTemp);
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
			vT.push_back (sTemp);
			sTemp = "";
		}
	}

	if (sTemp.size())
		vT.push_back (sTemp);

	return vT;
}

bool CMIDIHandler::ValidBiasParam (std::string& str, uint8_t numValues)
{
	// The 'bias' parameter string (AutoRhythmNoteLenBias, AutoRhythmGapLenBias
	// AutoRhythmNoteOnOffBias) is expected to be a comma-separated list of
	// integer values.
	//
	// AutoRhythmNoteLenBias should be a set of 6 numbers. Each number represents 
	// the relative chance of the note being a given length where, from left to right,
	// the numbers pertain to whole-notes, half-notes, quarter-notes, eight-notes,
	// sixteenth-notes and thirty-second-notes. For example, if the parameter string 
	// is "1, 1, 1, 10, 1, 1" it means that the length of a note has a much greater 
	// chance of being an eighth-note.
	//
	// Exactly the same applies to AutoRhythmGapLenBias, except it pertains to the
	// length of the space between notes.
	//

	bool bOk = true;

	std::vector<std::string> v = akl::Explode (str, ",");
	if (v.size() != numValues)
		return false;

	std::string str2;
	std::string comma;
	for (uint32_t i = 0; i < v.size(); i++)
	{
		int32_t nVal;
		if (!akl::VerifyTextInteger (v[i], nVal, 0, 1000))
			return false;

		// Absolute hack: To prevent endless loop, in the case of AutoRhythmNoteLenBias
		// and AutoRhythmGapLenBias, ensure that there is at least one instance for the
		// 32nds values. That is, a parameter value of, for example, "1, 1, 1, 1, 1, 0"
		// will cause loop, so this would be changed to "1, 1, 1, 1, 1, 1". This is
		// all about guaranteeing that we can have a note or gap length of 1/32nd when
		// necessary. Can't easily think of a way around this for now.
		// If you really do not want to have any 32nd notes/gaps, specify big numbers
		// (eg. 1000) for the other values - this should virtually exclude 32nds.
		if (i == 5 && nVal == 0)
			v[i] = "1";

		// Reconstruct param string for sake of dreadful hack mentioned above.
		str2 += comma + v[i];
		comma = ", ";
	}

	// The input string is modified.
	str = str2;

	return bOk;
}

// For Auto-Chords and Random Chord Replacement (RCR) handling.
void CMIDIHandler::InitChordBank (const std::string& sKey)
{
	// Only one execution of this routine, for either Auto-Chords or RCR.
	if (_bChordBankInit)
		return;

	_bChordBankInit = true;

	std::string sNote = sKey.substr (0, 1);
	if (sKey.size() > 1 && (sKey[1] == 'b' || sKey[1] == '#'))
		sNote += sKey[1];

	// Is it a minor key?
	bool bMinorKey = false;
	size_t pos = sKey.find ("m");
	if (pos != std::string::npos)
		bMinorKey = true;

	// Create two CChordBank objects that will hold the chord lists. One is for the main key,
	// the other is a Modal Interchange version, which *may* be used for chord selection (T2015A).
	_vChordBank.push_back (std::make_unique<CChordBank> (sNote, _vChordTypeVariationFactors));
	_vChordBank.push_back (std::make_unique<CChordBank> (sNote, _vChordTypeVariationFactors));

	// Initialize index values for referencing the relevant chord bank.
	if (bMinorKey)
	{
		_iCB_Main = 0;
		_iCB_ModInt = 1;
	}

	// Minor version
	uint8_t nRootPercentage = _vAutoChordsMinorChordBias[0];
	uint8_t nOtherMinorPercentage = _vAutoChordsMinorChordBias[1];	// The other two minor chords.
	uint8_t nMajorPercentage = _vAutoChordsMinorChordBias[2];		// The three major chords.
	// (Remaining percentage alloted to the diminished chord.)
	_vChordBank[_iCB_Main]->BuildMinor (nRootPercentage, nOtherMinorPercentage, nMajorPercentage);

	// Major version.
	nRootPercentage = _vAutoChordsMajorChordBias[0];
	uint8_t nOtherMajorPercentage = _vAutoChordsMajorChordBias[1];	// The other two major chords.
	uint8_t nMinorPercentage = _vAutoChordsMajorChordBias[2];		// The three minor chords.
	// (Remaining percentage alloted to the diminished chord.)
	_vChordBank[_iCB_ModInt]->BuildMajor (nRootPercentage, nOtherMajorPercentage, nMinorPercentage);

	// T2015A Randomizer for chance of Modal Interchange.
	// 100-element vector, each element of which can be 0 or 1, indicating which
	// chord bank to use for selecting the random chord.
	for (int i = 0; i < 100; i++)
	{
		uint8_t whichChordBank = i < _nModalInterchangeChancePercentage ? _iCB_ModInt : _iCB_Main;
		_vChordBankChoice.push_back (whichChordBank);
	}
}

std::string CMIDIHandler::GetStatusMessage()
{
	return _sStatusMessage;
}

//-----------------------------------------------------------------------------
// Static class members

std::string CMIDIHandler::_version = "0.44";

std::map<std::string, std::string>CMIDIHandler::_mChordTypes;
std::map<std::string, std::vector<uint8_t>>CMIDIHandler::_mMelodyNotes;
std::map<std::string, uint8_t>CMIDIHandler::_mChromaticScale;
std::map<std::string, uint8_t>CMIDIHandler::_mChromaticScale2;
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
	_mChordTypes.insert (std::pair<std::string, std::string>("dim",		"3,6"));			// Diminished
	_mChordTypes.insert (std::pair<std::string, std::string>("dim7",	"3,6,9"));		// Diminished 7th
	_mChordTypes.insert (std::pair<std::string, std::string>("m7b5",	"3,6,10"));		// Half-Diminished 7th

	// Auto-Melody: Semitone positions of all the notes available.
	// (The +AutoMelodyUsePentatonic parameter allows you to expand the notes in the major and minor
	// chords to include the additional notes of the chord's respective pentatonic scale.)
	//
	// Major chords
	_mMelodyNotes.insert (std::pair<std::string, std::vector<uint8_t>>("maj",    { 0, 0, 0, 0, 0, 4, 4, 4, 7, 7, 7, 7, 7 } ));
	_mMelodyNotes.insert (std::pair<std::string, std::vector<uint8_t>>("7",      { 0, 0, 0, 0, 0, 4, 4, 4, 7, 7, 7, 7, 7, 10, 10 } ));
	_mMelodyNotes.insert (std::pair<std::string, std::vector<uint8_t>>("maj7",   { 0, 0, 0, 0, 0, 4, 4, 4, 7, 7, 7, 7, 7, 11, 11 } ));
	_mMelodyNotes.insert (std::pair<std::string, std::vector<uint8_t>>("9",      { 0, 0, 0, 0, 0, 4, 4, 4, 7, 7, 7, 7, 7 } ));
	_mMelodyNotes.insert (std::pair<std::string, std::vector<uint8_t>>("maj9",   { 0, 0, 0, 0, 0, 4, 4, 4, 7, 7, 7, 7, 7 } ));
	_mMelodyNotes.insert (std::pair<std::string, std::vector<uint8_t>>("add9",   { 0, 0, 0, 0, 0, 4, 4, 4, 7, 7, 7, 7, 7 } ));
	//
	// Minor chords
	_mMelodyNotes.insert (std::pair<std::string, std::vector<uint8_t>>("m",      { 0, 0, 0, 0, 0, 3, 3, 3, 7, 7, 7, 7, 7 } ));
	_mMelodyNotes.insert (std::pair<std::string, std::vector<uint8_t>>("m7",     { 0, 0, 0, 0, 0, 3, 3, 3, 7, 7, 7, 7, 7 } ));
	_mMelodyNotes.insert (std::pair<std::string, std::vector<uint8_t>>("m9",     { 0, 0, 0, 0, 0, 3, 3, 3, 7, 7, 7, 7, 7, 14, 14 } ));
	_mMelodyNotes.insert (std::pair<std::string, std::vector<uint8_t>>("madd9",  { 0, 0, 0, 0, 0, 3, 3, 3, 7, 7, 7, 7, 7, 14, 14 } ));
	//
	// Suspended and diminished chords
	_mMelodyNotes.insert (std::pair<std::string, std::vector<uint8_t>>("sus2",   { 0, 0, 0, 0, 2, 2, 7, 7, 7 } ));
	_mMelodyNotes.insert (std::pair<std::string, std::vector<uint8_t>>("7sus2",  { 0, 0, 0, 0, 2, 2, 7, 7, 7, 10 } ));
	_mMelodyNotes.insert (std::pair<std::string, std::vector<uint8_t>>("sus4",   { 0, 0, 0, 0, 5, 5, 7, 7, 7 } ));
	_mMelodyNotes.insert (std::pair<std::string, std::vector<uint8_t>>("7sus4",  { 0, 0, 0, 0, 5, 5, 7, 7, 7, 10 } ));
	_mMelodyNotes.insert (std::pair<std::string, std::vector<uint8_t>>("5",      { 0, 0, 0, 0, 0, 2, 4, 4, 7, 7, 7, 9 } ));
	_mMelodyNotes.insert (std::pair<std::string, std::vector<uint8_t>>("dim",    { 0, 0, 0, 0, 0, 3, 3, 6, 6 } ));
	_mMelodyNotes.insert (std::pair<std::string, std::vector<uint8_t>>("dim7",   { 0, 0, 0, 0, 0, 3, 3, 6, 6, 9 } ));
	_mMelodyNotes.insert (std::pair<std::string, std::vector<uint8_t>>("m7b5",   { 0, 0, 0, 0, 0, 3, 3, 6, 6, 10 } ));

	// Sanity check that _mMelodyNotes corresponds correctly to _mChordTypes.
	for each (auto ct in _mChordTypes)
	{
		std::map<std::string, std::vector<uint8_t>>::iterator it;
		it = _mMelodyNotes.find (ct.first);
		ASSERT (it != _mMelodyNotes.end());
	}

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

	// Just the flat annotation, which I prefer.
	_mChromaticScale2.insert (std::pair<std::string, uint8_t>("C", 0));
	_mChromaticScale2.insert (std::pair<std::string, uint8_t>("Db", 1));
	_mChromaticScale2.insert (std::pair<std::string, uint8_t>("D", 2));
	_mChromaticScale2.insert (std::pair<std::string, uint8_t>("Eb", 3));
	_mChromaticScale2.insert (std::pair<std::string, uint8_t>("E", 4));
	_mChromaticScale2.insert (std::pair<std::string, uint8_t>("F", 5));
	_mChromaticScale2.insert (std::pair<std::string, uint8_t>("Gb", 6));
	_mChromaticScale2.insert (std::pair<std::string, uint8_t>("G", 7));
	_mChromaticScale2.insert (std::pair<std::string, uint8_t>("Ab", 8));
	_mChromaticScale2.insert (std::pair<std::string, uint8_t>("A", 9));
	_mChromaticScale2.insert (std::pair<std::string, uint8_t>("Bb", 10));
	_mChromaticScale2.insert (std::pair<std::string, uint8_t>("B", 11));

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

