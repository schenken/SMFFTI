// SMFFTI.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "pch.h"
#include "framework.h"
#include "SMFFTI.h"

#include <mmsystem.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// The one and only application object

CWinApp theApp;

using namespace std;

int main (int argc, char* argv[])
{
    int nRetCode = 0;

    HMODULE hModule = ::GetModuleHandle(nullptr);

    if (hModule != nullptr)
    {
        // initialize MFC and print and error on failure
        if (!AfxWinInit(hModule, nullptr, ::GetCommandLine(), 0))
        {
            // TODO: code your application's behavior here.
            wprintf(L"Fatal Error: MFC initialization failed\n");
            nRetCode = 1;
        }
        else
        {
            // TODO: code your application's behavior here.
            DoStuff (argc, argv);
        }
    }
    else
    {
        // TODO: change error code to suit your needs
        wprintf(L"Fatal Error: GetModuleHandle failed\n");
        nRetCode = 1;
    }

    return nRetCode;
}

void DoStuff (int argc, char* argv[])
{
    std::vector<std::string> vArgs;
    for (int i = 0; i < argc; ++i) {
        vArgs.push_back (argv[i]);
    }

    if (argc < 2)
    {
        MessageBeep (MB_ICONERROR);
        PrintUsage();
        return;
    }

    if (std::string (argv[1]) == "-v" || std::string (argv[1]) == "-version")
    {
        std::ostringstream ss;
        ss << "Version " << CMIDIHandler::_version << "\n\n";
        std::cout << ss.str();
        return;
    }

    // T2P7E7 Wizard mode
    if (std::string (argv[1]) == "-w")
    {
        CMyUI myUI;
        myUI.Run();
        return;
    }

    // T2RQLW Set Parameter From Command Line
    if (vArgs[1] == "-p")
    {
        if (vArgs.size() != 4)
        {
            std::ostringstream ss;
            ss << "Command specified incorrectly. The Set Parameter command should\n"
                << "have 4 arguments, eg.:\n"
                << "\n"
                << "    SMFFTI.exe -p \"+AutoMelody = 1\" mymidi.txt\n"
                << "\n"
                ;
            PrintError (ss.str());
            return;
        }

        std::string sInFile (vArgs[3]);

        std::unique_ptr<CMIDIHandler> pMidiH = std::make_unique<CMIDIHandler> (sInFile);
        if (pMidiH->VerifyFile() != CMIDIHandler::StatusCode::Success)
        {
            PrintError (pMidiH->GetStatusMessage());
            return;
        }

        std::vector<std::string>& vFile = pMidiH->GetFileVec();

        if (pMidiH->SetParameter (vFile, vArgs[2]) != CMIDIHandler::StatusCode::Success)
        {
            PrintError (pMidiH->GetStatusMessage());
            return;
        }

        // Get reference to update file content vector.
        // (SetParameter only updates the file vector, and does NOT write to file.)
        //const std::vector<std::string>& vFile = pMidiH->GetFileVec();

        // Validate the updated content. Bit hacky here, but we make a new 
        // instance of CMIDIHandler to do this, in order for the verify process
        // to begin with everything initialised. Purists: Don't fret it - it's
        // not missile guidance software!
        pMidiH.reset();
        pMidiH = std::make_unique<CMIDIHandler> ("");
        if (pMidiH->VerifyMemFile (vFile) != CMIDIHandler::StatusCode::Success)
        {
            PrintError (pMidiH->GetStatusMessage());
            return;
        }

    	akl::WriteVectorToTextFile (sInFile, vFile);

        return;
     }


    if (argc < 3)
    {
        MessageBeep (MB_ICONERROR);
        PrintUsage();
        return;
    }

    bool bOverwriteOutFile = false;
    if (argc > 3)
    {
        for (uint8_t i = 3; i < argc; i++)
        {
            std::string sArg (argv[i]);
            if (sArg == "-o")
            {
                bOverwriteOutFile = true;
                continue;
            }
        }
    }

    // Random Funk Groove: -rfg switch
    // We generate a input MIDI command file.
    if (std::string (argv[1]) == "-rfg")
    {
        std::string sOutFile (argv[2]);
        CMIDIHandler midiH ("");
        if (midiH.CreateRandomFunkGrooveMIDICommandFile (sOutFile, bOverwriteOutFile) != CMIDIHandler::StatusCode::Success)
        {
            PrintError (midiH.GetStatusMessage());
        }
        return;
    }

    int8_t iInFile = 1, iOutFile = 2;

    // Auto Rhythm: Create modified version of command file
    // to contain a rhythm (which is not connected with the
    // funk groove concept).
    bool bAutoRhythm = false;
    if (std::string (argv[1]) == "-ar")
    {
        if (argc < 4)
        {
            std::ostringstream ss;
            ss << "Command specified incorrectly. The Auto-Rhythm command should be\n"
                << "something like:\n\n"
                << "    SMFFTI.exe -ar mymidi.txt mymidi_groove.txt\n";
            PrintError (ss.str());
            return;
        }

        bAutoRhythm = true;
        iInFile = 2;
        iOutFile = 3;
    }

    // 2303090952 Auto-chords (-ac) command.
    bool bAutoChords = false;
    if (std::string (argv[1]) == "-ac")
    {
        if (argc < 4)
        {
            std::ostringstream ss;
            ss << "Command specified incorrectly. The Auto-Chords command should be\n"
                << "something like:\n\n"
                << "    SMFFTI.exe -ac mymidi.txt mymidi_ac.txt\n";
            PrintError (ss.str());
            return;
        }
        bAutoChords = true;
        iInFile = 2;
        iOutFile = 3;
    }

    // T2O4GU MIDI To SMFFTI (mode -m)
    bool bMIDIToSMFFTI = false;
    if (std::string(argv[1]) == "-m")
    {
        if (argc < 4)
        {
            std::ostringstream ss;
            ss << "Command specified incorrectly. The MIDI-To-SMFFTI command should be\n"
                << "something like:\n\n"
                << "    SMFFTI.exe -m mymidi.mid mymidi.txt\n";
            PrintError (ss.str());
            return;
        }

        bMIDIToSMFFTI = true;
        iInFile = 2;
        iOutFile = 3;
    }


    // Generic Randomized Melodies (-grm)
    // (No input file required.)
    if (std::string (argv[1]) == "-grm")
    {
        if (argc < 3)
        {
            std::ostringstream ss;
            ss << "Command specified incorrectly. The Generic Random Melodies\n"
                << "command should be something like:\n\n"
                << "    SMFFTI.exe -grm generic_rand_melodies.txt\n";
            PrintError (ss.str());
            return;
        }

        CMIDIHandler midiH ("");
        if (midiH.GenRandMelodies (argv[iOutFile], bOverwriteOutFile) != CMIDIHandler::StatusCode::Success)
            PrintError (midiH.GetStatusMessage());
        return;
    }

    // Input file expected.
    std::string sInFile = argv[iInFile];
    std::ifstream ifs (sInFile, std::fstream::in);
    if (!ifs.is_open())
    {
        PrintError ("Unable to open input file.");
        return;
    }

    std::string sOutFile (argv[iOutFile]);
    CMIDIHandler midiH (sInFile);

    // T2015A We need to inform the CMIDIHandler object if we are using Auto-Chords mode.
    // This is because, if so, we must ignore +RandomChordReplacementKey if set. In other
    // words, Auto-Chords trumps RCR.
    if (bAutoChords)
        midiH.UsingAutoChords();

    // Safety
    if (sInFile == sOutFile)
    {
        PrintError ("Input and output filenames must not be the same.");
        return;
    }

    // T2O4GU MIDI-To-SMFFTI
    if (bMIDIToSMFFTI)
    {
        if (midiH.ConvertMIDIToSMFFTI (sInFile, sOutFile, bOverwriteOutFile) != CMIDIHandler::StatusCode::Success)
            PrintError (midiH.GetStatusMessage());
        return;
    }

    if (midiH.VerifyFile() != CMIDIHandler::StatusCode::Success)
    {
        PrintError (midiH.GetStatusMessage());
        return;
    }

    if (bAutoRhythm)
    {
        if (midiH.CopyFileWithAutoRhythm (sOutFile, bOverwriteOutFile) != CMIDIHandler::StatusCode::Success)
            PrintError (midiH.GetStatusMessage());
        return;
    }

    // 2303090952 Auto-Chords
    if (bAutoChords)
    {
        if (midiH.CopyFileWithAutoChords (sOutFile, bOverwriteOutFile) != CMIDIHandler::StatusCode::Success)
            PrintError (midiH.GetStatusMessage());
        return;
    }

    if (midiH.CreateMIDIFile (sOutFile, bOverwriteOutFile) != CMIDIHandler::StatusCode::Success)
    {
        PrintError (midiH.GetStatusMessage());
        return;
    }
}

void PrintUsage()
{
    std::ostringstream ss;
    ss << 

        "\n"
        "Simple MIDI Files From Text Input (SMFFTI v" << CMIDIHandler::_version << ")\n"
        "-----------------------------------------------\n\n"

        "Usage 1 - Create MIDI file from text command file:\n\n"

        "    SMFFTI.exe <infile> <outfile>\n\n"

        "where <infile> is a SMFFTI command file containing a chord progression and parameters\n"
        "and <outfile> is the name of the MIDI file (.mid) to create.\n\n"

        "Usage 2 - Generate Random Funk Groove SMFFTI command file:\n\n"

        "    SMFFTI.exe -rfg <outfile>\n\n"

        "where <outfile> is the name of the SMFFTI command file.\n\n"

        "Usage 3 - Generate modified SMFFTI command file containing a randomized rhythm pattern:\n\n"

        "    SMFFTI.exe -ar <infile> <outfile>\n\n"

        "where <infile> is a SMFFTI command file containing a chord progression and parameters\n"
        "and <outfile> is the name of a modified version of <infile> that \n"
        "contains an automatically-generated rhythm.\n\n"

        "Usage 4 - Generate modified SMFFTI command file containing a randomized chord\n"
        "progression along with a randomized rhythm pattern:\n\n"

        "    SMFFTI.exe -ac <infile> <outfile>\n\n"

        "Usage 5 - Generate text file to contain generic random melodies:\n\n"

        "    SMFFTI.exe -grm <outfile>\n\n"

        "Usage 6 - Generate SMFFTI-format chord progression data from a MIDI file:\n\n"

        "    SMFFTI.exe -m <infile> <outfile>\n\n"

        "where <infile> is a MIDI file and <outfile> is an existing SMFFTI command file\n"
        "to be updated, or a plain text file.\n\n"

        "Usage 7 - Set a parameter in a SMFFTI command file:\n\n"

        "    SMFFTI.exe -p \"<parameter>\" <infile>\n\n"

        "Usage 8 - Command File Wizard:\n\n"

        "    SMFFTI.exe -w\n\n"

        "Consult the manual for more information on all the above operations.\n\n"
        ;

    std::cout << ss.str();
}

void PrintError (std::string sMsg)
{
    MessageBeep (MB_ICONERROR);
    std::cout << "\nERROR! " << sMsg << "\n\n";
}
