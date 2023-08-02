//
// Created by SLaufer on 12.08.20.
//

#ifndef MELLON_API_SRC_HELPERFUNCTIONS_H_
#define MELLON_API_SRC_HELPERFUNCTIONS_H_

#include <homegear-base/Variable.h>

namespace MellonApi {

class HelperFunctions {
 public:
  HelperFunctions() = delete;

  static BaseLib::PVariable getDataFromDn(const std::string &dn);
};

}

#endif //MELLON_API_SRC_HELPERFUNCTIONS_H_
