// SMFFTI.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "pch.h"
#include "framework.h"
#include "SMFFTI.h"

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
            if (argc < 2)
            {
                std::cout << "Please specify a command file.\0";
                return 1;
            }

            std::string sMIDICommandFile (argv[1]);
            std::ifstream ifs (sMIDICommandFile, std::fstream::in);
            if (!ifs.is_open())
            {
                std::cout << "ERROR! Unable to open input file.\0";
                return 1;
            }

            DoStuff (sMIDICommandFile);
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

void DoStuff (std::string sFilename)
{
    CMIDIHandler midiH (sFilename);

    uint8_t nInFileStatus = midiH.Verify();
    if (nInFileStatus == 1)
    {
        std::cout << "ERROR! Number of chords does not match number of notes. Sort it out, numpty!\n";
        return;
    }

    if (nInFileStatus == 2)
    {
        std::cout << "ERROR! Invalid NoteCenter value. Sort it out, chump!\n";
        return;
    }

    midiH.CreateMIDIFile ("dummy.mid");
}