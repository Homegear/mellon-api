//
// Created by SLaufer on 12.08.20.
//

#ifndef MELLON_API_SRC_REST_CA_TOKEN_CREATE_H_
#define MELLON_API_SRC_REST_CA_TOKEN_CREATE_H_

#include "../../../RestInfo.h"

#include <functional>

namespace MellonApi::Rest::CaChild::TokenChild
{

class Create
{
 public:
  Create() = delete;

  static BaseLib::PVariable post(const PRestInfo& restInfo);
};

}

#endif //MELLON_API_SRC_REST_CA_CA_SIGN_H_
