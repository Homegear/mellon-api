//
// Created by SLaufer on 12.08.20.
//

#ifndef MELLON_API_SRC_REST_MELLON_H_
#define MELLON_API_SRC_REST_MELLON_H_

#include "../RestInfo.h"

namespace MellonApi::Rest {

class Mellon {
 public:
  Mellon() = delete;

  static BaseLib::PVariable get(const PRestInfo &restInfo);
};

}

#endif //MELLON_API_SRC_REST_MELLON_H_
