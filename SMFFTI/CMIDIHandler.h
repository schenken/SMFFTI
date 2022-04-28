#pragma once

enum class EventName : uint8_t
{
	NoteOff = 0x80,
	NoteOn = 0x90,
	AfterTouch = 0xA0,
	ControlChange = 0xB0,
	ProgramChange = 0xC0,
	ChannelPressure = 0xD0,
	PitchBend = 0xE0,
	SysEx = 0xF0
};

enum class MetaEventName : uint8_t
{
	MetaSequence = 0x00,
	MetaText = 0x01,
	MetaCopyright = 0x02,
	MetaTrackName = 0x03,
	MetaInstrumentName = 0x04,
	MetaLyric = 0x05,
	MetaMarker = 0x06,
	MetaCuePoint = 0x07,
	MetaChannelPrefix = 0x20,
	MetaEndOfTrack = 0x2F,
	MetaSetTempo = 0x51,
	MetaSMPTEOffset = 0x54,
	MetaTimeSignature = 0x58,
	MetaKeySignature = 0x59,
	MetaSequencerSpecific = 0x7F
};

class CMIDIHandler
{
public:
	CMIDIHandler (std::string sInputFile);

	uint8_t Verify();	// Load command file and check content is valid.

	// Whack out a dead simple MIDI file. Single track with just a few notes.
	uint8_t CreateMIDIFile (std::string filename, bool bOverwriteOutFile);

	std::string GetStatusMessage();

private:
	void GenerateNoteEvents();
	void SortNoteEventsAndFixOverlaps();
	void ApplyArpeggiation();
	void SortArpeggiatedChordNotes();
	void ApplyRandomizedVelocity();

	void AddMIDIChordNoteEvents (uint32_t nNoteSeq, std::string chordName, bool& bNoteOn, uint32_t nEventTime);
	int8_t NoteToMidi (std::string sNote, uint8_t& nNote, uint8_t& nSharpFlat);

	bool GetChordIntervals (std::string sChordName, uint8_t& nRoot, std::vector<std::string>& vChordIntervals);

	uint32_t Swap32 (uint32_t n);
	uint16_t Swap16 (uint16_t n);

	std::string BitString (uint32_t n, uint32_t numBits);
		
	// Convert 32-bit integer to the MIDI "variable length" value and append to buffer.
	void PushVariableValue (uint32_t n);

	void PushInt8 (uint8_t n);
	void PushText (const std::string& s);

	// File with MIDI content directives.
	std::string _sInputFile;

	// Store MIDI input file content
	std::vector<std::string> _vInput;

	std::vector<std::string> _vNotePositions;
	std::vector<std::string> _vChordNames;
	std::vector<uint32_t> _vBarCount;

	// Track chunk storage
	std::vector<byte> _vTrackBuf;
	uint32_t _nOffset = 0;

	uint16_t _ticksPerQtrNote = 96;
	uint16_t _ticksPer32nd;
	uint16_t _ticksPerBar;
	uint8_t _nChannel = 0;
	uint8_t _nVelocity = 80;
	uint8_t _nRandVelVariation = 0;
	uint8_t _nVelocityOff = 0;
	uint32_t _currentTime = 0;

	// Transitory values
	uint32_t _nVal32 = 0;
	uint16_t _nVal16 = 0;
	std::string _sText = "";

	/*
	The MIDI note event list (_vMIDINoteEvents), once populated and sorted is, in fact, a
	sequence of pairs of sets of notes for each chord, in ON/OFF sequence, eg.

		chord 1 - Event On
		chord 1 - Event Off
		chord 2 - Event On
		chord 2 - Event Off
		chord 3 - Event On
		chord 3 - Event Off
	*/
	struct MIDINote
	{
		uint32_t nTime;
		uint8_t nKey;
		uint8_t nEvent;
		uint32_t nSeq;

		MIDINote() : nTime (0), nEvent ((uint8_t)EventName::NoteOn), nKey (60), nSeq (0) {}
		MIDINote (uint32_t nSeq_, uint32_t nTime_, uint8_t nEvent_, uint8_t nKey_)
			: nSeq (nSeq_), nTime (nTime_), nEvent (nEvent_), nKey (nKey_) {}
	};
	std::vector<MIDINote> _vMIDINoteEvents;

	std::vector<MIDINote> _vMIDINoteEvents2;

	bool _bAddBassNote = false;
	uint8_t _nRandNoteStartOffset = 0;
	uint8_t _nRandNoteEndOffset = 0;
	bool _bRandNoteStart = false;
	bool _bRandNoteEnd = false;
	bool _bRandNoteOffsetTrim = false;

	int32_t _nNoteCount = -1;
	int8_t _nNoteStagger = 0;

	std::string _sOctaveRegister = "3";	// Default: Root notes placed in the C3 - B3 range.

	// The lowest note (actually a C), determined by _sOctaveRegister, where chord notes
	// will be placed (excepting optional bass notes). It is notional (or provisional) in
	// the sense that, when active, downward transposition of upper chord notes might
	// place notes *bloew* this value.
	uint8_t _nProvisionalLowestNote = 0;

	uint8_t _nTransposeThreshold = 48;	// By default, downward transposition unlikely.

	// Arp stuff
	uint32_t _nArpeggiator = 0;
	uint32_t _nArpTime;
	uint32_t _nArpNoteTicks;
	float _nArpGatePercent;
	int8_t _nArpOctaveSteps = 0;	// Positive/negative values to transpose higher/lower

	std::string _sTrackName = "Made by SMFFTI";

	std::string _sStatusMessage = "";

	// Randomizer
	std::random_device _rdev;

	//---------------------------------------------------------------------
	// Static class members

	// Inner class hack for initializing class members.
	static struct ClassMemberInit { ClassMemberInit(); } cmi;

	static std::map<std::string, std::string>_mChordTypes;
	static std::map<std::string, uint8_t>_mChromaticScale;

	static std::default_random_engine _eng;
};

