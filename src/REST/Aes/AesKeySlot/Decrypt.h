//
// Created by SLaufer on 12.08.20.
//

#ifndef MELLON_API_SRC_REST_AES_AES_DECRYPT_H_
#define MELLON_API_SRC_REST_AES_AES_DECRYPT_H_

#include "../../../RestInfo.h"

#include <functional>

namespace MellonApi::Rest::AesChild::AesKeySlotChild
{

class Decrypt
{
 public:
  Decrypt() = delete;

  static BaseLib::PVariable post(const PRestInfo& restInfo, uint32_t aesKeySlot);
};

}

#endif //MELLON_API_SRC_REST_CA_CA_SIGN_H_
