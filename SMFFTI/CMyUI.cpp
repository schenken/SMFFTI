#include "pch.h"
#include "CMyUI.h"

void CMyUI::InitUI ()
{
	// Init all the menus, etc.

	// Main (0 from main menu is also 'quit').
	menuMain.AddActionLink ("Main Menu", [&](){ Quit(); });
	menuMain.AddFormLink ("Create SMFFTI Command File", formCreateSMFFTIFile);
	//menuMain.AddFormLink ("Create MIDI File", formCreateMIDIFile);
	//menuMain.AddActionLink ("bill", [&](){ DummyAction::Bill(); });
	//menuMain.AddFormLink ("Form", form1);
	AddMenu ((uint32_t)Menu::Main, menuMain);

	// Menu 2
	//menu2.AddMenuLink ("Hello", (uint32_t)Menu::Main);
	//menu2.AddActionLink ("Sub Bill", [&](){ DummyAction::Bill(); });
	//AddMenu ((uint32_t)Menu::menu2, menu2);

	// Form
	formCreateSMFFTIFile.AddTextPrompt ((uint32_t)FormCreateSMFFTIFile::FormTitle, "Create SMFFTI Command File", 0, 256, "");	// First field is the title.
	formCreateSMFFTIFile.AddTextPrompt ((uint32_t)FormCreateSMFFTIFile::Filename, "Name of output file", 5, 256, "scf.txt");
	formCreateSMFFTIFile.AddYNPrompt ((uint32_t)FormCreateSMFFTIFile::Overwrite, "Overwrite file if it already exists", "N");
	formCreateSMFFTIFile.AddTextPrompt ((uint32_t)FormCreateSMFFTIFile::ChordProgession, "Chord progression", 1, 100, "C, Am, F, G");
	formCreateSMFFTIFile.AddYNPrompt ((uint32_t)FormCreateSMFFTIFile::ExtraBassNote, "Extra bass note", "N");
	formCreateSMFFTIFile.AddYNPrompt ((uint32_t)FormCreateSMFFTIFile::RandVel, "Randomized velocity", "Y");
	formCreateSMFFTIFile.AddYNPrompt ((uint32_t)FormCreateSMFFTIFile::OffsetNotePos, "Offset note positions", "Y");
	formCreateSMFFTIFile.FormCompletedPrompt ((uint32_t)FormCreateSMFFTIFile::ReadyToAction, "Are you ready to create the file");
	formCreateSMFFTIFile.FormAction ([&](){ CreateSMFFTIFile(); });
	formCreateSMFFTIFile.Validation ([&](Prompt& pr, std::string& s) { return Validate_CreateSMFFTIFile (pr, s);});

	// Form
	Form& f = formCreateMIDIFile;
	f.AddTextPrompt ((uint32_t)FormCreateMIDIFile::FormTitle, "Create MIDI File", 0, 256, "");
	f.AddTextPrompt ((uint32_t)FormCreateMIDIFile::InputSMFFTIFile, "SMFFTI Command File", 5, 256, "");
	f.AddTextPrompt ((uint32_t)FormCreateMIDIFile::OutputMIDIFilename, "MIDI Output File", 5, 256, "myMidi.mid");
	f.AddYNPrompt ((uint32_t)FormCreateMIDIFile::Overwrite, "Overwrite file if it already exists", "N");
	f.FormCompletedPrompt ((uint32_t)FormCreateMIDIFile::ReadyToAction, "Are you ready to create the file");
	f.FormAction ([&](){ CreateMIDIFile(); });
	f.Validation ([&](Prompt& pr, std::string& s) { return Validate_CreateMIDIFile (pr, s);});
}

void CMyUI::PressAnyKey()
{
	std::cout << "\n      Press any key to return to the menu";
	int c = _getch();
}

void CMyUI::CreateSMFFTIFile()
{
	std::string sOutFile = formCreateSMFFTIFile.vPrompts[(uint32_t)FormCreateSMFFTIFile::Filename].sAnswer;
	bool bOverwrite = formCreateSMFFTIFile.vPrompts[(uint32_t)FormCreateSMFFTIFile::Overwrite].sAnswer == "Y" ? true : false;
	if (akl::MyFileExists(sOutFile) && !bOverwrite)
	{
		std::cout << "\n      Error: File already exists.\n";
		PressAnyKey();
		return;
	}

	std::string sParam_BassNote = 
		formCreateSMFFTIFile.vPrompts[(uint32_t)FormCreateSMFFTIFile::ExtraBassNote].sAnswer == "Y" ?
		"1" : "0";
	std::string sParam_RandVelVariation = 
		formCreateSMFFTIFile.vPrompts[(uint32_t)FormCreateSMFFTIFile::RandVel].sAnswer == "Y" ?
		"25" : "0";
	std::string sParam_NotePosOffsetAmt = 
		formCreateSMFFTIFile.vPrompts[(uint32_t)FormCreateSMFFTIFile::OffsetNotePos].sAnswer == "Y" ?
		"2" : "0";

	std::cout << "\n      Creating SMFFTI file...";

	std::ostringstream ss;
	ss << 

    "# SMFFTI Command File: " << sOutFile << "\n\n"

	"+TrackName=SMFFTI\n\n"

	"+BassNote=" << sParam_BassNote << "\n"
	"+RootNoteOnly=0\n\n"

	"+Velocity=80\n"
	"+RandVelVariation=" << sParam_RandVelVariation << "\n\n"

	"+OctaveRegister=3\n"
	"+TransposeThreshold=11\n\n"

	"+RandNoteStartOffset=" << sParam_NotePosOffsetAmt << "\n"
	"+RandNoteEndOffset=" << sParam_NotePosOffsetAmt << "\n"
	"+RandNoteOffsetTrim=1\n\n"

	"+AutoMelody=0\n"
	"+AutoRhythmNoteLenBias=0, 0, 4, 8, 4, 2\n"
	"+AutoRhythmGapLenBias=0, 0, 0, 4, 8, 1\n"
	"+AutoRhythmConsecutiveNoteChancePercentage=25\n\n"

	//"(# This comment block shows the default values that are used - \n"
	//"you can uncomment this block and adjust the values.\n"
	//"#\n"
	"# Parameters relating to Auto-Chords (-ac) feature.\n"
	"#\n"
	"+AutoChordsNumBars=4\n"
	"+AutoChordsMinorChordBias=22, 42, 32\n"
	"+AutoChordsMajorChordBias=22, 42, 32\n"
	"+AutoChordsShortNoteBiasPercent=35\n"
	"#\n"
	"# Chance of Chord Type Variations (CTV) for Major chords.\n"
	"# Note that you can specify a chance of suspended chords\n"
	"# replacing a major chord.\n"
	"#\n"
	"+AutoChords_CTV_maj     = 1000\n"
	"+AutoChords_CTV_7       = 100\n"
	"+AutoChords_CTV_maj7    = 10\n"
	"+AutoChords_CTV_9       = 10\n"
	"+AutoChords_CTV_maj9    = 10\n"
	"+AutoChords_CTV_add9    = 80\n"
	"+AutoChords_CTV_sus2    = 15\n"
	"+AutoChords_CTV_7sus2   = 15\n"
	"+AutoChords_CTV_sus4    = 10\n"
	"+AutoChords_CTV_7sus4   = 10\n"
	"#\n"
	"# Chance of Chord Type Variations (CTV) for Minor chords\n"
	"+AutoChords_CTV_min     = 1000\n"
	"+AutoChords_CTV_m7      = 100\n"
	"+AutoChords_CTV_m9      = 10\n"
	"+AutoChords_CTV_madd9   = 10\n"
	"#\n"
	"+AutoChords_CTV_dim     = 0\n"
	"+AutoChords_CTV_dim7    = 1\n"
	"+AutoChords_CTV_m7b5    = 1\n"
	//"#)\n"
	"\n"

	"+RandomChordReplacementKey = Gm\n"
	"+ModalInterchangeChancePercentage = 0\n"

	;

	// Output the chord progression data
	uint32_t nBarsPerLine = 4;
	for (size_t i = 0; i < _vChordProgression.size(); i += nBarsPerLine)
	{
		ss << "\n";

		uint32_t nRemaining = _vChordProgression.size() - i;
		uint32_t nCount = nRemaining >= 4 ? nBarsPerLine : nRemaining;

		for (uint8_t j = 0; j < nCount; j++)
			ss << "$ . . . | . . . | . . . | . . . ";
		ss << "\n";

		for (uint8_t j = 0; j < nCount; j++)
			ss << "+############################## ";
		ss << "\n";

		std::string sComma = "";
		for (uint8_t j = 0; j < nCount; j++)
		{
			ss << sComma << _vChordProgression[i + j];
			sComma = ", ";
		}
		ss << "\n";
	}

	std::ofstream f;
	f.open (sOutFile);
	if (f)
	{
		f << ss.str();
		f.close();
	}

	// stroke the user
	Sleep (1000);
	std::cout << "completed." << std::endl;

	// Check integrity of file and report error
	CMIDIHandler midiH (sOutFile);
    if (midiH.VerifyFile() != CMIDIHandler::StatusCode::Success)
    {
	    //MessageBeep (MB_ICONERROR);
		std::cout << "      Error! " << midiH.GetStatusMessage() << "\n\n";
    }
	
	PressAnyKey();
}

bool CMyUI::Validate_CreateSMFFTIFile (Prompt& pr, std::string& sInput)
{
	bool result = true;

	if (pr.nSeq == (uint32_t)FormCreateSMFFTIFile::ChordProgession)
	{
		// Check chord progression string
		_sChordProgression = akl::RemoveWhitespace (sInput, 4);
		_vChordProgression = akl::Explode (_sChordProgression, ",");

		// Make the note letter uppercase.
		for (auto& c : _vChordProgression)
			c[0] = std::toupper (c[0]);

		CMIDIHandler midiH ("");
		for (auto c : _vChordProgression)
		{
			std::vector<std::string> vChordIntervals;
			uint8_t nRoot = 0;
			std::string sChordType;
			if (!midiH.GetChordIntervals(c, nRoot, vChordIntervals, sChordType))
			{
				std::cout << "      Invalid chord: " << c << "\n";
				return false;
			}
		}
	}

	return result;
}

void CMyUI::CreateMIDIFile()
{
	std::string sInFile = formCreateMIDIFile.vPrompts[(uint32_t)FormCreateMIDIFile::InputSMFFTIFile].sAnswer;
	CMIDIHandler midiH (sInFile);
	if (midiH.VerifyFile() != CMIDIHandler::StatusCode::Success)
	{
		std::cout << "\n      Error! " << midiH.GetStatusMessage() << "\n";
		PressAnyKey();
		return;
	}

	std::string sOutFile = formCreateMIDIFile.vPrompts[(uint32_t)FormCreateMIDIFile::OutputMIDIFilename].sAnswer;
	bool bOverwrite = formCreateMIDIFile.vPrompts[(uint32_t)FormCreateMIDIFile::Overwrite].sAnswer == "Y" ? true : false;
	if (akl::MyFileExists(sOutFile) && !bOverwrite)
	{
		std::cout << "\n      Error: File already exists.\n";
		PressAnyKey();
		return;
	}

	std::cout << "\n      Creating MIDI file...";

	CMIDIHandler::StatusCode res;
	res = (midiH.CreateMIDIFile (sOutFile, bOverwrite));

	Sleep (1000);
	std::cout << "completed." << std::endl;

	if (res != CMIDIHandler::StatusCode::Success)
		std::cout << "\n      Error! " << midiH.GetStatusMessage() << "\n";

	PressAnyKey();
}

bool CMyUI::Validate_CreateMIDIFile (Prompt& pr, std::string& sInput)
{
	bool result = true;

	if (pr.nSeq == (uint32_t)FormCreateMIDIFile::InputSMFFTIFile)
	{
		if (!akl::MyFileExists(sInput))
		{
			std::cout << "      File does not exist.\n";
			return false;
		}
	}
	return result;
}
