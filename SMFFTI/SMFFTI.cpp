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

            std::string sMIDICommandFile (argv[1]);
            std::ifstream ifs (sMIDICommandFile, std::fstream::in);
            if (!ifs.is_open())
            {
                PrintError ("Unable to open input file.");
                return 1;
            }

            std::string sOutputMIDIFile (argv[2]);

            DoStuff (sMIDICommandFile, sOutputMIDIFile);
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

void DoStuff (const std::string& sInFile, const std::string sOutFile)
{
    CMIDIHandler midiH (sInFile);

    uint8_t nInFileStatus = midiH.Verify();
    if (nInFileStatus != 0)
    {
        PrintError (midiH.GetStatusMessage());
        return;
    }
 
    midiH.CreateMIDIFile (sOutFile);
}

void PrintUsage()
{
    const char* text =

        "\n"
        "Simple MIDI Files From Text Input (SMFFTI)\n"
        "------------------------------------------\n\n"

        "Usage:\n\n"

        "    SMFFTI.exe <infile> <outfile>\n\n"

        "where <infile> is a text file containing instructions and directives\n"
        "and <outfile> is the name of the MIDI file (.mid) to create.\n\n"

        ;

    printf (text);
}

void PrintError (std::string sMsg)
{
    MessageBeep (MB_ICONERROR);
    std::cout << "\nERROR! " << sMsg << "\n\n";
}
