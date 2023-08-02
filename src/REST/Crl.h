//
// Created by SLaufer on 13.08.20.
//

#ifndef MELLON_API_SRC_REST_CRL_H_
#define MELLON_API_SRC_REST_CRL_H_

#include "../RestInfo.h"

namespace MellonApi::Rest {

class Crl {
 public:
  Crl() = delete;

  static BaseLib::PVariable get(const PRestInfo &restInfo);
};

}

#endif //MELLON_API_SRC_REST_CRL_H_
