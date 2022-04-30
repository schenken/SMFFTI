#include "pch.h"
#include "Common.h"

namespace akl {

size_t LoadTextFileIntoVector(const std::string& filename, std::vector<std::string>& v)
{
    std::ifstream f;
    f.open(filename.c_str());
    std::string line;
    while (getline(f, line))
    {
        v.push_back(line);
    }
    f.close();

    return v.size();
}

std::string RemoveWhitespace (const std::string& s, uint8_t mode)
{
	// Modes: Leading = 1, Trailing = 2, All = 4, Condense = 8

	if (s.size() == 0)
		return "";

	const char* pChar = &s[0];
	std::string s1;
	s1.resize (s.size());

	int lastChar = 0;	// last char in copy string
	bool bNewWhiteSpaceBlock = true;
	bool bLeadingWhitespace = true;
	while (*pChar)
	{
		if (_istspace ((unsigned char)*pChar))
		{
			// Whitespace

			if (mode & 0x04)
			{
				// skip
			}
			else if ((mode & 0x01) && bLeadingWhitespace)
			{
				// skip
			}
			else if ((mode & 0x08) && !bNewWhiteSpaceBlock)
			{
				// skip if not 1st ws char in contiguous series
			}
			else
				s1[lastChar++] = ' ';

			bNewWhiteSpaceBlock = false;
		}
		else
		{
			// NOT whitespace, so copy
			bLeadingWhitespace = false;
			bNewWhiteSpaceBlock = true;
			s1[lastChar++] = *pChar;
		}

		pChar++;
	}

	if (mode & 0x02)
	{
		// strip trailing spaces
		while (s1[lastChar - 1] == ' ')
			lastChar--;
	}

	s1.resize (lastChar);

    return s1;
}

std::vector<std::string> Explode (const std::string& s, const std::string& delim)
{
	std::vector<std::string> v;

	std::size_t found = s.find_first_of (delim);
	int prev = 0;
	while (found != std::string::npos)
	{
		v.push_back (s.substr (prev, found - prev));
		prev = found + delim.length();
		found = s.find_first_of (delim, prev);
	}

	std::string x = s.substr(prev, s.length() - prev);
	if (x.length() > 0)
		v.push_back(x);

	return v;
}

bool VerifyTextInteger (std::string sNum, int32_t& nReturnValue, int32_t nFrom, int32_t nTo)
{
	bool bOK = true;

	nReturnValue = 0;

	if (sNum.size() == 0)
		return false;

	if (nFrom >= 0 && !std::all_of (sNum.begin(), sNum.end(), ::isdigit))
		return false;

	if (nFrom < 0)
	{
		// Negative min value allowed
		if (sNum[0] == '-')
		{
			if (!std::all_of (sNum.begin() + 1, sNum.end(), ::isdigit))
				return false;
		}
		else
		{
			if (!std::all_of (sNum.begin(), sNum.end(), ::isdigit))
				return false;
		}
	}

	try {
		nReturnValue = std::stoi (sNum);
	}
	catch (std::invalid_argument & e) {
		// if no conversion could be performed
		e;
		bOK = false;
	}
	catch (...) {
		// everything else
		bOK = false;
	}

	if (nReturnValue < nFrom || nReturnValue > nTo)
	{
		nReturnValue = 0;
		bOK = false;
	}

	return bOK;
}

bool VerifyDoubleInteger (std::string sNum, double& nReturnValue, double nFrom, double nTo)
{
	bool bOK = true;

	nReturnValue = 0.0;

	if (sNum.size() == 0)
		return false;

	if (nFrom >= 0 && !std::all_of (sNum.begin(), sNum.end(), [](char c) { return ::isdigit (c) || c == '.'; }))
		return false;

	if (nFrom < 0)
	{
		// Negative min value allowed
		int n = 0;
		if (sNum[0] == '-')
			n = 1;
		if (!std::all_of (sNum.begin() + n, sNum.end(), [](char c) { return ::isdigit (c) || c == '.'; }))
			return false;
	}

	// Only a single decimal point
	if (std::count (sNum.begin(), sNum.end(), '.') > 1)
		return false;

	try {
		nReturnValue = std::stod (sNum);
	}
	catch (std::invalid_argument & e) {
		// if no conversion could be performed
		e;
		bOK = false;
	}
	catch (...) {
		// everything else
		bOK = false;
	}

	if (nReturnValue < nFrom || nReturnValue > nTo)
	{
		nReturnValue = 0;
		bOK = false;
	}

	return bOK;
}

bool MyFileExists (const std::string& name)
{
	std::ifstream f (name.c_str());
	return f.good();
}

}
