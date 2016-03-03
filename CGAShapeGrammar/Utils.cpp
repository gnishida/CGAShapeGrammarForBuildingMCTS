#include "Utils.h"
#include <regex>
#include <sstream>

namespace utils {

	bool isNumber(const std::string& str) {
		std::regex e("^-?\\d*\\.?\\d+");
		if (std::regex_match(str, e)) return true;
		else return false;
	}

}