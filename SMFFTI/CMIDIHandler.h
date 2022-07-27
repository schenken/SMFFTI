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

	enum class StatusCode : uint16_t
	{
		Success,
		ParameterValueMissing,
		InvalidBassNoteValue,
		NumberOfChordsDoesNotMatchNoteCount,
		InvalidVelocityValue,
		InvalidRandomVelocityVariationValue,
		InvalidRandomNoteStartOffsetValue,
		InvalidRandomNoteEndOffsetValue,
		InvalidRandomNoteOffsetTrimValue,
		InvalidNoteStaggerValue,
		InvalidOctaveRegisterValue,
		InvalidTransposeThresholdValue,
		InvalidArpeggiatorValue,
		InvalidArpeggiatorTimeValue,
		InvalidArpeggiatorGatePercentValue,
		InvalidArpeggiatorOctaveStepsValue,
		UnrecognizedCommandFileParameter,
		IllegalCharInNotePositionsLine,
		BlankNotePositions,
		FirstNonSpaceCharInNotePosLineMustBePlus,
		NotePositionLineTooLong,
		InvalidRulerLine,
		MissingChordList,
		InvalidOrBlankChordName,
		StrayNotePositionLine,
		InvalidLine,
		NoChordsSpecified,
		InvalidFunkStrumValue,
		InvalidFunkStrumUpStrokeAttenuationValue,
		InvalidFunkStrumVelDeclineIncrementValue,
		MaxFourBarsPerLine,
		OutputFileAlreadyExists,
		InvalidMelodyModeValue,
		InvalidAutoRhythmNoteLenBias,
		InvalidAutoRhythmGapLenBias,
		InvalidAutoRhythmConsecutiveNoteChancePercentage
	};

	CMIDIHandler (std::string sInputFile);

	StatusCode CreateRandomFunkGrooveMIDICommandFile (std::string sOutFile, bool bOverwriteOutFile);

	StatusCode Verify();	// Load command file and check content is valid.

	// Whack out a dead simple MIDI file. Single track with just a few notes.
	StatusCode CreateMIDIFile (const std::string& filename, bool bOverwriteOutFile);

	// Generate a copy of the input file, but with it containing a
	// randomly-generated rhythm.
	StatusCode CopyFileWithAutoRhythm (std::string filename, bool bOverwriteOutFile);

	StatusCode GenRandMelodies (std::string filename, bool bOverwriteOutFile);

	std::string GetStatusMessage();

private:
	std::string GetRandomGroove (bool& bRandomGroove);
	void GenerateNoteEvents();
	void SortNoteEventsAndFixOverlaps();
	void ApplyNoteStagger();
	void ApplyArpeggiation();
	void SortChordNotes();
	void PushNoteEvents();

	void AddMIDIChordNoteEvents (int32_t nMelodyNote, uint32_t nNoteSeq, std::string chordName, bool& bNoteOn, uint32_t nEventTime);
	int8_t NoteToMidi (std::string sNote, uint8_t& nNote, uint8_t& nSharpFlat);

	bool GetChordIntervals (std::string sChordName, uint8_t& nRoot, std::vector<std::string>& vChordIntervals);

	StatusCode InitMidiFile (std::ofstream& ofs, const std::string& filename, bool bOverwriteOutFile);
	void FinishMidiFile (std::ofstream& ofs);

	uint32_t Swap32 (uint32_t n) const;
	uint16_t Swap16 (uint16_t n) const;

	std::string BitString (uint32_t n, uint32_t numBits) const;
		
	// Convert 32-bit integer to the MIDI "variable length" value and append to buffer.
	void PushVariableValue (uint32_t n);

	void PushInt8 (uint8_t n);
	void PushText (const std::string& s);

	std::vector<std::string> TokenizeNotePosStr (std::string notePosStr);

	bool ValidBiasParam (std::string& str, uint8_t numValues);

	// File with MIDI content directives.
	std::string _sInputFile;

	// Store MIDI input file content
	std::vector<std::string> _vInput;

	std::vector<std::string> _vNotePositions;
	std::vector<uint32_t> _vNotePosLineInFile;
	std::vector<std::string> _vChordNames;
	std::vector<std::string> _vMelodyNotes;
	std::vector<uint32_t> _vBarCount;

	// Track chunk storage
	std::vector<byte> _vTrackBuf;
	uint32_t _nOffset = 0;

	uint16_t _ticksPerQtrNote = 96;
	uint16_t _ticksPer16th;
	uint16_t _ticksPer32nd;
	uint16_t _ticksPerBar;
	uint8_t _nChannel = 0;
	uint8_t _nVelocityOff = 0;
	uint32_t _currentTime = 0;

	// Transitory values
	uint32_t _nVal32 = 0;
	uint16_t _nVal16 = 0;
	std::string _sText = "";

	uint8_t _nVelocity = 80;
	uint8_t _nRandVelVariation = 0;
	bool _bAddBassNote = false;
	uint8_t _nRandNoteStartOffset = 0;
	uint8_t _nRandNoteEndOffset = 0;
	bool _bRandNoteStart = false;
	bool _bRandNoteEnd = false;
	bool _bRandNoteOffsetTrim = false;

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
		uint8_t nVel;

		MIDINote() : nTime (0), nEvent ((uint8_t)EventName::NoteOn), nKey (60), nSeq (0), nVel (80) {}
		MIDINote (uint32_t nSeq_, uint32_t nTime_, uint8_t nEvent_, uint8_t nKey_, uint8_t nVel_)
			: nSeq (nSeq_), nTime (nTime_), nEvent (nEvent_), nKey (nKey_), nVel (nVel_) {}
	};
	std::vector<MIDINote> _vMIDINoteEvents;

	std::vector<MIDINote> _vMIDINoteEvents2;

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
	uint32_t _nArpTime = 8;	// 1/8th default
	uint32_t _nArpNoteTicks;
	float _nArpGatePercent = 0.5f;
	int8_t _nArpOctaveSteps = 0;	// Positive/negative values to transpose higher/lower

	bool _bFunkStrum = false;
	double _nFunkStrumUpStrokeAttenuation = 1.0;
	uint8_t _nFunkStrumVelDeclineIncrement = 5;

	bool _bMelodyMode = 0;
	uint32_t _melodyModeLineNum = 0;
	std::vector<uint8_t> _vRandomMelodyNotes;
	std::vector<std::string> _vMelodyChordNames;

	std::string _sTrackName = "Made by SMFFTI";

	std::string _sStatusMessage = "";

	// Randomizer
	std::random_device _rdev;

	// Auto-Rhythm (-ar): Three params for controlling the articulation
	// of the groove/syncopation. The defaults set here are for a
	// reasonably groovy rhythm, suitable for bass guitar, for example.
	//
	// Defines the choices for note and gap lengths. Specifies the
	// number of 32nd, 16th, 8th, 1/4, 1/2 and whole notes, from
	// left-to-right in the string.
	// NB. *ALWAYS* specify at least ONE 32nd (the last in the list).
	std::string _sAutoRhythmNoteLenBias = "0, 0, 8, 16, 16, 4";
	std::string _sAutoRhythmGapLenBias = "0, 0, 0, 4, 8, 1";
	//
	// Percentage chance of* consecutive* notes.
	// 0 means alternating notes and gaps.
	// 50 means 50 % chance of consecutive notes, ie.no gap in - between.
	// 100 means no gaps (except even-numbered 32nds, when no note is possible).
	uint32_t _sAutoRhythmConsecutiveNoteChancePercentage = 35;

	const std::string sRuler = "[......|.......|.......|.......]";

	//---------------------------------------------------------------------
	// Static class members

	// Inner class hack for initializing class members.
	static struct ClassMemberInit { ClassMemberInit(); } cmi;

	static std::map<std::string, std::string>_mChordTypes;
	static std::map<std::string, uint8_t>_mChromaticScale;

	static std::vector<std::string> _vRFGChords;

	static std::default_random_engine _eng;
};

