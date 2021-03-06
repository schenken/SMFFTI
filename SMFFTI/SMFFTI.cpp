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
            if (argc < 3)
            {
                MessageBeep (MB_ICONERROR);
                PrintUsage();
                return 1;
            }

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

    std::string sInFile (argv[1]);
    std::ifstream ifs (sInFile, std::fstream::in);
    if (!ifs.is_open())
    {
        PrintError ("Unable to open input file.");
        return;
    }

    std::string sOutFile (argv[2]);

    CMIDIHandler midiH (sInFile);

    if (midiH.Verify() != CMIDIHandler::StatusCode::Success)
    {
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
    const char* text =

        "\n"
        "Simple MIDI Files From Text Input (SMFFTI)\n"
        "------------------------------------------\n\n"

        "Usage 1 - Create MIDI file from text command file:\n\n"

        "    SMFFTI.exe <infile> <outfile>\n\n"

        "where <infile> is a text file containing instructions and directives\n"
        "and <outfile> is the name of the MIDI file (.mid) to create.\n\n"

        "Usage 2 - Generate Random Funk Groove text command file:\n\n"

        "    SMFFTI.exe -rfg <outfile>\n\n"

        "where <outfile> is the name of the text command file.\n\n"
        ;

    printf (text);
}

void PrintError (std::string sMsg)
{
    MessageBeep (MB_ICONERROR);
    std::cout << "\nERROR! " << sMsg << "\n\n";
}
