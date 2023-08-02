//
// Created by SLaufer on 12.08.20.
//

#ifndef MELLON_API_SRC_REST_CA_CA_SIGN_H_
#define MELLON_API_SRC_REST_CA_CA_SIGN_H_

#include "../../../RestInfo.h"

#include <functional>

#include <gnutls/x509.h>

namespace MellonApi::Rest::CaChild::CaKeySlotChild
{

class Sign
{
 public:
  Sign() = delete;

  static BaseLib::PVariable post(const PRestInfo& restInfo, uint32_t certificateSlot);
};

}

#endif //MELLON_API_SRC_REST_CA_CA_SIGN_H_
