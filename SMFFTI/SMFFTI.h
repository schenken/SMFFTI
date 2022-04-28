#pragma once

#include "resource.h"
#include "CMIDIHandler.h"

void DoStuff (int argc, char* argv[], const std::string& sInFile, const std::string sOutFile);

void PrintUsage();
void PrintError (std::string sMsg);
