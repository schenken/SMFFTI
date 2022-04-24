#include "pch.h"
#include "CMIDIHandler.h"
#include "Common.h"

CMIDIHandler::CMIDIHandler (std::string sInputFile) : _sInputFile (sInputFile)
{
	_ticksPer32nd = _ticksPerQtrNote / 8;
	_ticksPerBar = _ticksPerQtrNote * 4;

	_nArpTime = 8;	// 1/8th default
	_nArpNoteTicks = _ticksPerBar / _nArpTime;
	_nArpGatePercent = 0.5f;	// 50% default

	_eng.seed (_rdev());
}

uint8_t CMIDIHandler::Verify()
{
	uint8_t result = 0;

	akl::LoadTextFileIntoVector (_sInputFile, _vInput);

	uint8_t nDataLines = 0;
	uint16_t nNumberOfNotes = 0;

	for each (auto sLine in _vInput)
	{
		std::string sTemp = akl::RemoveWhitespace (sLine, 4); // strip all ws

		if (nDataLines == 1)
		{
			// This should be a Note Position line, ie. series of hashes
			std::string sNotePositions = akl::RemoveWhitespace (sLine, 2);	// strip trailing spaces
			_vNotePositions.push_back (sNotePositions);
			nNumberOfNotes += std::count (sNotePositions.begin(), sNotePositions.end(), '+');
			nDataLines++;
			continue;
		}

		if (nDataLines == 2)
		{
			// This should be a list of chord names, comma-separated.
			std::vector<std::string> v = akl::Explode (sTemp, ",");
			_vChordNames.insert (_vChordNames.end(), v.begin(), v.end());
			nDataLines = 0;
			continue;
		}
		
		if (sTemp.size() == 0)	// ignore blank lines
			continue;

		if (sTemp[0] == '#')	// ignore comments
			continue;

		if (sTemp[0] == '+')	// parameter
		{
			std::string parameter = sTemp.substr (1, sTemp.size() - 1);
			int ak = 1;

			std::vector<std::string> vKeyValue = akl::Explode (parameter, "=");

			if (vKeyValue[0] == "BassNote")
			{
				_bAddBassNote = true;
				continue;
			}
			else if (vKeyValue[0] == "Velocity")
			{
				_nVelocity = std::stoi (vKeyValue[1]);	// overrides default
			}
			else if (vKeyValue[0] == "RandVelVariation")
			{
				_nRandVelVariation = std::stoi (vKeyValue[1]);
			}
			else if (vKeyValue[0] == "RandNoteStartOffset")
			{
				_nRandNoteStartOffset = std::stoi (vKeyValue[1]);
				if (_nRandNoteStartOffset > 0)
					_bRandNoteStart = true;
			}
			else if (vKeyValue[0] == "RandNoteEndOffset")
			{
				_nRandNoteEndOffset = std::stoi (vKeyValue[1]);
				if (_nRandNoteEndOffset > 0)
					_bRandNoteEnd = true;
			}
			else if (vKeyValue[0] == "RandNoteOffsetTrim")
			{
				_bRandNoteOffsetTrim = vKeyValue[1] == "1";
			}
			else if (vKeyValue[0] == "NoteStagger")
			{
				_nNoteStagger = std::stoi (vKeyValue[1]);
			}
			else if (vKeyValue[0] == "ProvisionalLowestNote")
			{
				_sProvisionalLowestNote = vKeyValue[1];
			}
			else if (vKeyValue[0] == "TransposeThreshold")
			{
				_nTransposeThreshold = std::stoi (vKeyValue[1]);
				if (_nTransposeThreshold < 0)
					_nTransposeThreshold = 0;
			}
			else if (vKeyValue[0] == "Arpeggiator")
			{
				_nArpeggiator = std::stoi (vKeyValue[1]);
			}
			else if (vKeyValue[0] == "ArpTime")
			{
				_nArpTime = std::stoi (vKeyValue[1]);
				_nArpNoteTicks = _ticksPerBar / _nArpTime;
			}
			else if (vKeyValue[0] == "ArpGatePercent")
			{
				_nArpGatePercent = std::stoi (vKeyValue[1]) / 100.0f;
			}
			else if (vKeyValue[0] == "ArpOctaveSteps")
			{
				_nArpOctaveSteps = std::stoi (vKeyValue[1]);
			}
		}

		if (sTemp.substr (0, 32) == "[......|.......|.......|.......]")
		{
			// Ruler line. The next two lines should contain
			// (1) Note Positions: series of hash groups
			// (2) Comma-separated Chord Names, one for each of the Note Position hash groups.

			uint32_t nNumBars = sTemp.length() / 32;
			_vBarCount.push_back (nNumBars);

			nDataLines++;
			continue;
		}

		uint8_t e = (uint8_t)EventName::NoteOn | 0x01;
		uint8_t w = e & 0XF0;
		int ak = 1;
	}


	// Arpeggiation enabled cancels: Randomized Note Positions., and Note Stagger.
	if (_nArpeggiator)
	{
		_bRandNoteStart = false;
		_bRandNoteEnd = false;
		_nNoteStagger = 0;
	}

	// Translate "Note Octave" to MIDI note number, eg. C4 -> 60
	uint8_t nSharpFlat = 0;
	int8_t res = NoteToMidi (_sProvisionalLowestNote, _nProvisionalLowestNote, nSharpFlat);
	if (res == -1)
		return 2;

	_sOctave.assign (1, _sProvisionalLowestNote[1]);

	// Check we have the same number of chords as note positions
	if (nNumberOfNotes != _vChordNames.size())
		return 1;

	return result;
}

void CMIDIHandler::CreateMIDIFile (std::string filename)
{
	std::ofstream ofs (filename, std::ios::binary);

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
	_sText = "AK Dummy MIDI Sequence";
	PushVariableValue (_sText.length());
	PushText (_sText);

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

	GenerateNoteEvents();

	SortNoteEventsAndFixOverlaps();

	if (_nArpeggiator)
		ApplyArpeggiation();

	if (_nRandVelVariation > 0)
		ApplyRandomizedVelocity();

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

			if (s2 == "BLANK")
			{ }
			else
			{
				for each (auto c in s)
				{
					// Each string here represents 2 bars
					if (c == '+')
					{
						nNote++;

						// start of note
						if (!bNoteOn)
						{
							if (i == 0)
								bNoteOn = !bNoteOn;
							else
								AddMIDIChordNoteEvents (++nChordPair, _vChordNames[nNote], bNoteOn, pos32nds * _ticksPer32nd);
						}
						else
						{
							// Another note detected without a gap from previous note,
							// so insert a Note Off first.
							if (i == 1)
							{
								AddMIDIChordNoteEvents (nChordPair, _vChordNames[nPrevNote], bNoteOn, pos32nds * _ticksPer32nd);
								AddMIDIChordNoteEvents (++nChordPair, _vChordNames[nNote], bNoteOn, pos32nds * _ticksPer32nd);
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
								AddMIDIChordNoteEvents (++nChordPair, _vChordNames[nNote], bNoteOn, pos32nds * _ticksPer32nd);

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
								AddMIDIChordNoteEvents (nChordPair, _vChordNames[nNote], bNoteOn, pos32nds * _ticksPer32nd);
					}

					pos32nds++;
					nPrevNote = nNote;
				}

				// final end of note
				if (bNoteOn)
					if (i == 0)
						bNoteOn = !bNoteOn;
					else
						AddMIDIChordNoteEvents (nChordPair, _vChordNames[nNote], bNoteOn, pos32nds * _ticksPer32nd);
			}

			// move pointer 4 bars forward
			if (i == 1)
				nBar += _vBarCount[nItem];

			nItem++;
		}

		_nNoteCount = nNote;
	}
}

void CMIDIHandler::SortNoteEventsAndFixOverlaps()
{
	// If randomized note start/end applies, need to sort into event time order
	// and fix any overlap errors introduced.
	if (_bRandNoteStart || _bRandNoteEnd)
	{
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
}

void CMIDIHandler::ApplyArpeggiation()
{
	SortArpeggiatedChordNotes();

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
						MIDINote nNew (n1.nSeq, n2.nTime, (uint8_t)EventName::NoteOff | _nChannel, n1.nKey);
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

void CMIDIHandler::SortArpeggiatedChordNotes()
{
	// For the sake of arpeggiation...
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
		for (uint8_t i = itemFirst; i < itemLast; i++)
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

void CMIDIHandler::ApplyRandomizedVelocity()
{
	// Range for random velocity.
	std::uniform_int_distribution<int> randVelVariation (0, _nRandVelVariation);
	if (_nVelocity - (_nRandVelVariation / 2) < 0)
		_nVelocity = 5;
	else
		_nVelocity -= (_nRandVelVariation / 2);	// offset base velocity to allow for upward random variation.

	uint32_t nPrevNoteTime = 0;
	for each (auto note in _vMIDINoteEvents)
	{
		// Lambda func to get random velocity variation
		auto fnRandVel = [&]() {
			uint8_t nRand = 0;
			if (_nRandVelVariation > 0)
				nRand = randVelVariation (_eng);
			uint16_t nTemp = _nVelocity + nRand;
			if (nTemp > 127)
				nTemp = 127;
			return (uint8_t)nTemp;
		};

		uint32_t nNoteTime = note.nTime;
		uint32_t nDeltaTime = nNoteTime - nPrevNoteTime;

		PushVariableValue (nDeltaTime);
		PushInt8 (note.nEvent);
		PushInt8 (note.nKey);
		PushInt8 ((note.nEvent & 0xF0) == (uint8_t)EventName::NoteOn ? fnRandVel() : _nVelocityOff);

		nPrevNoteTime = nNoteTime;
	}
}

void CMIDIHandler::AddMIDIChordNoteEvents (uint32_t nNoteSeq, std::string chordName, bool& bNoteOn, uint32_t nEventTime)
{
	bNoteOn = !bNoteOn;

	// set randomizer ranges
	std::uniform_int_distribution<int> randNoteStartOffset (-_nRandNoteStartOffset, _nRandNoteStartOffset);
	std::uniform_int_distribution<int> randNoteEndOffset (-_nRandNoteEndOffset, _nRandNoteEndOffset);

	std::uniform_int_distribution<int> randNoteStartOffsetPositive (0, _nRandNoteStartOffset);
	std::uniform_int_distribution<int> randNoteEndOffsetNegative (-_nRandNoteEndOffset, 0);

	// Lambda funcs for randomizing note start/end
	auto fnRandStart = [&](bool bPositiveOnly) {
		int8_t nRand = 0;
		if (_bRandNoteStart)
			nRand = bPositiveOnly && _bRandNoteOffsetTrim ? randNoteStartOffsetPositive (_eng) : randNoteStartOffset (_eng);
		return nRand;
	};
	auto fnRandEnd = [&](bool bNegativeOnly) {
		int8_t nRand = 0;
		if (_bRandNoteEnd)
			nRand = bNegativeOnly && _bRandNoteOffsetTrim ? randNoteEndOffsetNegative (_eng) : randNoteEndOffset (_eng);
		return nRand;
	};

	// Sort out the root note
	std::string sChord;
	sChord.assign (1, chordName[0]);
	if (chordName.size() > 1 && (chordName[1] == 'b' || chordName[1] == '#'))
		sChord += chordName[1];
	sChord += _sOctave;
	uint8_t nRoot;
	uint8_t nSharpFlat = 0;
	int8_t res = NoteToMidi (sChord, nRoot, nSharpFlat);

	// Adjust for flat/sharp.
	uint8_t nCount = 1;
	if (nSharpFlat > 0)
		nCount++;

	// Chord type
	std::string chordType = chordName.substr (nCount, chordName.size() - nCount);
	if (chordType == "")
		chordType = "maj";
	std::vector<std::string> vNoteIntervals = akl::Explode (_mChordTypes[chordType], ",");

	// Init note stagger offset. If value positive, start with lowest note; if negative,
	// start from highest. First note itself is not staggered.
	int8_t notePosOffset = -_nNoteStagger;
	if (_nNoteStagger < 0)
		notePosOffset = -_nNoteStagger * ((uint8_t)vNoteIntervals.size() + 1 + (_bAddBassNote * 1));
		
	uint8_t nEventType = (bNoteOn ? (uint8_t)EventName::NoteOn : (uint8_t)EventName::NoteOff) | _nChannel;
	uint32_t nET;

	// Optional extra bass note
	if (_bAddBassNote)
	{
		if (_nNoteStagger && bNoteOn)
			notePosOffset += _nNoteStagger;
		else
			notePosOffset = (bNoteOn ? fnRandStart (nNoteSeq == 0) : fnRandEnd (nNoteSeq == _nNoteCount));

		nET = nEventTime + notePosOffset;
		int16_t noteTemp = nRoot - 12;
		if (noteTemp >= 0)
		{
			MIDINote note (nNoteSeq, nET, nEventType, nRoot - 12);
			_vMIDINoteEvents.push_back (note);
		}
	}


	// Root note
	if (_nNoteStagger && bNoteOn)
		notePosOffset += _nNoteStagger;
	else
		notePosOffset = (bNoteOn ? fnRandStart (nNoteSeq == 0) : fnRandEnd (nNoteSeq == _nNoteCount));

	nET = nEventTime + notePosOffset;
	MIDINote note (nNoteSeq, nET, nEventType, nRoot);
	_vMIDINoteEvents.push_back (note);

	// Chord notes
	for each (auto item in vNoteIntervals)
	{
		uint8_t nSemitones = std::stoi (item);

		if (_nNoteStagger && bNoteOn)
			notePosOffset += _nNoteStagger;
		else
			notePosOffset = (bNoteOn ? fnRandStart (nNoteSeq == 0) : fnRandEnd (nNoteSeq == _nNoteCount));

		nET = nEventTime + notePosOffset;

		// Downward transposition occurs if note is higher than highest-note threshold, or 127.
		uint16_t nNote = nRoot + nSemitones;
		while (nNote > (_nProvisionalLowestNote + _nTransposeThreshold) || nNote > 127)
			nNote -= 12;

		MIDINote note (nNoteSeq, nET, nEventType, (uint8_t)nNote);
		_vMIDINoteEvents.push_back (note);
	};

}

int8_t CMIDIHandler::NoteToMidi (std::string sNote, uint8_t& nNote, uint8_t& nSharpFlat)
{
	size_t nNoteLen = sNote.size();
	if (nNoteLen < 1 || nNoteLen > 3)
		return -1;

	char cNoteName = toupper (sNote[0]);

	if (cNoteName == '_')	// can't allow meta char used in search string
		return -1;

	uint8_t nNote2 = (uint8_t)std::string ("C_D_EF_G_A_B").find (cNoteName);
	if (nNote2 == std::string::npos)
		return -1;

	char cOctaveNumber = sNote[nNoteLen - 1];
	uint8_t nOctave = (uint8_t)std::string ("01234567").find (cOctaveNumber);
	if (nOctave == std::string::npos)
		return -1;

	// Middle C is always MIDI note number 60, but manufacturers can decide their
	// own ranges. Ableton Live sets Middle C as C3.
	nNote = nNote2 + ((nOctave + 2) * 12);

	if (sNote[1] == '#')
	{
		nNote++;
		nSharpFlat = 1;
	}
	else if (sNote[1] == 'b')
	{
		nNote--;
		nSharpFlat = 2;
	}

	return 0;
}





uint32_t CMIDIHandler::Swap32 (uint32_t n)
{
	return (((n >> 24) & 0xff) | ((n << 8) & 0xff0000) | ((n >> 8) & 0xff00) | ((n << 24) & 0xff000000));
};

uint16_t CMIDIHandler::Swap16 (uint16_t n)
{
	return ((n >> 8) | (n << 8));
};

std::string CMIDIHandler::BitString (uint32_t n, uint32_t numBits)
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

void CMIDIHandler::PushInt8 (uint8_t n) {
	_vTrackBuf.push_back (n);
	_nOffset++;
}

void CMIDIHandler::PushText (const std::string& s)
{
	auto bytes = reinterpret_cast<const byte*>(s.data());
	_vTrackBuf.insert (_vTrackBuf.end(), bytes, bytes + s.length());
	_nOffset += s.length();
}


std::map<std::string, std::string>CMIDIHandler::_mChordTypes;

CMIDIHandler::ClassMemberInit CMIDIHandler::cmi;

CMIDIHandler::ClassMemberInit::ClassMemberInit ()
{
	_mChordTypes.insert (std::pair<std::string, std::string>("maj",		"4,7"));
	_mChordTypes.insert (std::pair<std::string, std::string>("maj7",	"4,7,11"));
	_mChordTypes.insert (std::pair<std::string, std::string>("maj9",	"4,7,11,14"));
	_mChordTypes.insert (std::pair<std::string, std::string>("add9",	"4,7,14"));
	_mChordTypes.insert (std::pair<std::string, std::string>("7",		"4,7,10"));
	_mChordTypes.insert (std::pair<std::string, std::string>("m",		"3,7"));
	_mChordTypes.insert (std::pair<std::string, std::string>("m7",		"3,7,10"));
	_mChordTypes.insert (std::pair<std::string, std::string>("m9",		"3,7,10,14"));
	_mChordTypes.insert (std::pair<std::string, std::string>("madd9",	"3,7,14"));
	_mChordTypes.insert (std::pair<std::string, std::string>("sus2",	"2,7"));
	_mChordTypes.insert (std::pair<std::string, std::string>("7sus2",	"2,7,10"));
	_mChordTypes.insert (std::pair<std::string, std::string>("sus4",	"5,7"));
	_mChordTypes.insert (std::pair<std::string, std::string>("7sus4",	"5,7,10"));
}

// Randomizer static variable declaration
std::default_random_engine CMIDIHandler::_eng;

