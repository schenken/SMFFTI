#pragma once
#include "CConsoleUI.h"
#include "CMIDIHandler.h"

enum class Menu : uint32_t
{
	Main
	//menu2
};

// Field ids for formCreateSMFFTIFile
enum class FormCreateSMFFTIFile : uint32_t
{
	FormTitle,
	Filename,
	Overwrite,
	ChordProgession,
	ExtraBassNote,
	RandVel,
	OffsetNotePos,
	ReadyToAction
};

enum class FormCreateMIDIFile : uint32_t
{
	FormTitle,
	InputSMFFTIFile,
	OutputMIDIFilename,
	Overwrite,
	ReadyToAction
};

class CMyUI : public CConsoleUI
{
public:

protected:
	void InitUI() override;

	MenuDescriptor menuMain;
	//MenuDescriptor menu2;

	Form formCreateSMFFTIFile;
	Form formCreateMIDIFile;

	void CreateSMFFTIFile();
	bool Validate_CreateSMFFTIFile (Prompt& pr, std::string& sInput);

	void CreateMIDIFile();
	bool Validate_CreateMIDIFile (Prompt& pr, std::string& sInput);

	void PressAnyKey();

	std::string _sChordProgression;
	std::vector<std::string> _vChordProgression;
};

