#pragma once

#include "CChordBank.h"

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

enum class ChordTypeVariation : uint32_t
{
	Major,
	Dominant_7th,
	Major_7th,
	Dominant_9th,
	Major_9th,
	Add_9,
	Sus_2,
	_7_Sus_2,
	Sus_4,
	_7_Sus_4,
	Minor,
	Minor_7th,
	Minor_9th,
	Minor_Add_9,
	Dim,
	Dim_7th,
	HalfDim,
	_COUNT_
};


class CMIDIHandler
{
public:

	enum class StatusCode : uint16_t
	{
		Success,
		InvalidInputFile,
		ParameterValueMissing,
		InvalidBassNoteValue,
		InvalidRootNoteOnlyValue,
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
		InvalidCommandFileParameter,
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
		InvalidAutoMelodyValue,
		InvalidAutoRhythmNoteLenBias,
		InvalidAutoRhythmGapLenBias,
		InvalidAutoRhythmConsecutiveNoteChancePercentage,
		InvalidAllMelodyNotesValue,
		InvalidChordsBiasPercentage,
		InvalidAutoChordsNumBarsValue,
		InvalidAutoChordsMinorChordBias,
		InvalidAutoChordsMajorChordBias,
		InvalidAutoChordsShortNoteBiasPercent,
		InvalidAutoChordsCTV_maj_Value,
		InvalidAutoChordsCTV_7_Value,
		InvalidAutoChordsCTV_maj7_Value,
		InvalidAutoChordsCTV_9_Value,
		InvalidAutoChordsCTV_maj9_Value,
		InvalidAutoChordsCTV_add9_Value,
		InvalidAutoChordsCTV_sus2_Value,
		InvalidAutoChordsCTV_7sus2_Value,
		InvalidAutoChordsCTV_sus4_Value,
		InvalidAutoChordsCTV_7sus4_Value,
		InvalidAutoChordsCTV_min_Value,
		InvalidAutoChordsCTV_m7_Value,
		InvalidAutoChordsCTV_m9_Value,
		InvalidAutoChordsCTV_madd9_Value,
		InvalidAutoChordsCTV_dim_Value,
		InvalidAutoChordsCTV_dim7_Value,
		InvalidAutoChordsCTV_m7b5_Value,
		InvalidWriteOldRuler,
		InvalidAutoMelodyDontUsePentatonic,
		InvalidModalInterchangeChancePercentage,
		InvalidChordInMidiFile,
		InvalidMIDIFile,
		InvalidStartCommentBlock,
		InvalidEndCommentBlock,
		ParamAlreadySpecified,
		IllegalParamAfterMusicData,
		InvalidSYS_RCRHistoryCount,
		NoMusicData
	};

	enum class ParamCode : uint16_t
	{
		AllMelodyNotes,
		Arpeggiator,
		ArpGatePercent,
		ArpOctaveSteps,
		ArpTime,
		AutoChords_CTV_7,
		AutoChords_CTV_7sus2,
		AutoChords_CTV_7sus4,
		AutoChords_CTV_9,
		AutoChords_CTV_add9,
		AutoChords_CTV_dim,
		AutoChords_CTV_dim7,
		AutoChords_CTV_m7,
		AutoChords_CTV_m7b5,
		AutoChords_CTV_m9,
		AutoChords_CTV_madd9,
		AutoChords_CTV_maj,
		AutoChords_CTV_maj7,
		AutoChords_CTV_maj9,
		AutoChords_CTV_min,
		AutoChords_CTV_sus2,
		AutoChords_CTV_sus4,
		AutoChordsMajorChordBias,
		AutoChordsMinorChordBias,
		AutoChordsNumBars,
		AutoChordsShortNoteBiasPercent,
		AutoMelody,
		AutoMelodyDontUsePentatonic,
		AutoRhythmConsecutiveNoteChancePercentage,
		AutoRhythmGapLenBias,
		AutoRhythmNoteLenBias,
		BassNote,
		FunkStrum,
		FunkStrumUpStrokeAttenuation,
		FunkStrumVelDeclineIncrement,
		ModalInterchangeChancePercentage,
		NoteStagger,
		OctaveRegister,
		RandNoteEndOffset,
		RandNoteOffsetTrim,
		RandNoteStartOffset,
		RandomChordReplacementKey,
		RandVelVariation,
		RootNoteOnly,
		TrackName,
		TransposeThreshold,
		Velocity,
		WriteOldRuler,
		SYS_RCRHistoryCount,
		SYS_ParameterCount
	};

	CMIDIHandler (std::string sInputFile);

	StatusCode CreateRandomFunkGrooveMIDICommandFile (std::string sOutFile, bool bOverwriteOutFile);

	// Validate the command file.
	StatusCode VerifyFile();

	// Validate memory (vector) instance of command file.
	StatusCode VerifyMemFile (const std::vector<std::string>& vFile);

	// Whack out a dead simple MIDI file. Single track with just a few notes.
	StatusCode CreateMIDIFile (const std::string& filename, bool bOverwriteOutFile);

	// Generate a copy of the input file, but with it containing a
	// randomly-generated rhythm.
	StatusCode CopyFileWithAutoRhythm (std::string filename, bool bOverwriteOutFile);

	// 2303090952: Generate a copy of the input file, but with it containing a
	// randomly-generated chords (over 4 bars).
	StatusCode CopyFileWithAutoChords (std::string filename, bool bOverwriteOutFile);

	// T2015A
	void UsingAutoChords() { _bAutoChords = true; }

	// T2O4GU
	StatusCode ConvertMIDIToSMFFTI (std::string inFile, std::string outFile, bool bOverwriteOutFile);
	std::string IsValidChordType (const std::vector<uint16_t>& vNotes, bool& bMinor);

	StatusCode GenRandMelodies (std::string filename, bool bOverwriteOutFile);

	std::string GetStatusMessage();

	static std::string _version;

	static std::map<std::string, uint8_t>& GetChromaticScale() { return _mChromaticScale; }
	bool GetChordIntervals (std::string sChordName, uint8_t& nRoot, std::vector<std::string>& vChordIntervals, std::string& sChordType);

	// Set parameter inside a SMFFTI file. This is to facilitate batch
	// command processing, eg. using a .bat file to execute multiple
	// SMFFTI operations.
	StatusCode SetParameter (std::vector<std::string>& vF, const std::string& sP);

	std::vector<std::string> GetFileVec();

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

	void InitChordBank (const std::string& sKey);
	bool _bChordBankInit = false;

	// File with MIDI content directives.
	std::string _sInputFile;
	std::string _sOutputFile;

	// Store MIDI input file content.
	// This is written to by VerifyFile.
	std::vector<std::string> _vFile;

	std::vector<std::string> _vNotePositions;
	std::vector<uint32_t> _vNotePosLineInFile;
	std::vector<uint32_t> _vRulerLineInFile;
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
	bool _bRootNoteOnly = false;
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

	bool _bAutoMelody = 0;
	uint32_t _autoMelodyLineNum = 0;
	std::vector<uint8_t> _vRandomMelodyNotes;
	std::vector<std::string> _vMelodyChordNames;

	// +AllMelodyNotes: To output ALL possible melody notes
	// as a "chord", in order to see all notes in MIDI files
	// and manually edit to create a melody.
	bool _bAllMelodyNotes = false;

	uint8_t _nAutoChordsNumBars = 4;

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
	std::string _sAutoRhythmNoteLenBias = "0, 0, 4, 8, 4, 2";
	std::string _sAutoRhythmGapLenBias = "0, 0, 0, 4, 8, 1";
	//
	// Percentage chance of *consecutive* notes.
	// 0 means alternating notes and gaps.
	// 50 means 50 % chance of consecutive notes, ie.no gap in - between.
	// 100 means no gaps (except even-numbered 32nds, when no note is possible).
	uint32_t _nAutoRhythmConsecutiveNoteChancePercentage = 25;

	// Improved ruler, displaying characters only at 1/16th notes.
	// 1/4 notes now aligned with dollar sign and vertical bars.
	// 1/8th notes are: dollar sign, middle dot in each 1/4 note,
	// and vertical bars. This is now the default ruler type: If you
	// wish to use the old ruler type, set parameter +UseOldRuler = 1.
	std::string sRulerNew = "$ . . . | . . . | . . . | . . . ";

	std::string sRulerOld = "[......|.......|.......|.......]";

	std::string sRuler = sRulerNew;

	// Auto-chords: For *minor* keys, percentage bias for
	// (i) root chord (2) other minor chords (3) major chords,
	// respectively. If value less than 100, the remainder is
	// alloted to diminished chords.
	std::string _sAutoChordsMinorChordBias = "22, 42, 32";
	std::vector<uint8_t> _vAutoChordsMinorChordBias;

	// Auto-chords: For *major* keys, percentage bias for
	// (i) root chord (2) other major chords (3) minor chords,
	// respectively. If value less than 100, the remainder is
	// alloted to diminished chords.
	std::string _sAutoChordsMajorChordBias = "22, 42, 32";
	std::vector<uint8_t> _vAutoChordsMajorChordBias;

	// Auto-chords: Percentage to bias shorter notes.
	// Zero means *no* short notes; 100 means *all* short notes.
	uint8_t _nAutoChordsShortNoteBiasPercent = 35;

	// Auto-chords: Factors for specifying the chances of the
	// various Chord Type Variations (CTV) occurring.
	std::vector<uint32_t> _vChordTypeVariationFactors;

	// Use this to make SMFFTI output the old-style ruler when it
	// creates a modfied command file (eg. Auto-chords).
	bool _bWriteOldRuler = 0;

	// T2015A
	std::vector<std::unique_ptr<CChordBank>> _vChordBank;
	uint8_t _iCB_Main = 1;
	uint8_t _iCB_ModInt = 0;
	std::vector<uint8_t> _vChordBankChoice;

	// For RandomChordReplacement (RCR)
	std::vector<std::string> _vInputCopy;
	bool _bRCR = false;
	std::string _sRCRKey;

	// 230424 Auto-Melody: Inclusion of pentatonic notes
	// now controlled by parameter +AutoMelodyUsePentatonic.
	bool _bAutoMelodyDontUsePentatonic = false;

	// T2015A 
	uint8_t _nModalInterchangeChancePercentage = 0;
	bool _bAutoChords = false;

	uint32_t _nFirstRuler = 0;

	uint32_t _nRCRHistoryCount = 0;

	// Which parameters are specified in the command file.
	// Vector stores line num of first occurrence of param.
	std::vector<uint16_t> _vParamsUsed;

	//---------------------------------------------------------------------
	// Static class members

	// Inner class hack for initializing class members.
	static struct ClassMemberInit { ClassMemberInit(); } cmi;

	static std::map<std::string, std::string>_mChordTypes;

	// For each chord type, list of notes from the scale (semitone values)
	// that can be used for auto-melody.
	static std::map<std::string, std::vector<uint8_t>> _mMelodyNotes;

	static std::map<std::string, uint8_t>_mChromaticScale;
	static std::map<std::string, uint8_t>_mChromaticScale2;

	static std::vector<std::string> _vRFGChords;

	static std::default_random_engine _eng;

	static std::map<ParamCode, std::string> _mParamCodes;
};

