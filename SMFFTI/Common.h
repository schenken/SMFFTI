#pragma once

namespace akl {

size_t LoadTextFileIntoVector(const std::string& filename, std::vector<std::string>& v);
int WriteVectorToTextFile (const std::string filename, const std::vector<std::string> v);

std::string RemoveWhitespace (const std::string& s, uint8_t mode);

std::vector<std::string> Explode (const std::string& s, const std::string& delim);

bool VerifyTextInteger (std::string sNum, int32_t& nReturnValue, int32_t nFrom, int32_t nTo);
bool VerifyDoubleInteger (std::string sNum, double& nReturnValue, double nFrom, double nTo);

bool MyFileExists (const std::string& name);
std::string TimeStamp();

}
