//
// Created by sathya on 08.10.19.
//

#ifndef MELLON_API_PING_H
#define MELLON_API_PING_H

#include "../RestInfo.h"

namespace MellonApi::Rest {

class Ping {
 public:
  Ping() = delete;

  static BaseLib::PVariable getPublic(const PRestInfo &restInfo, bool &handled);
};

}

#endif
