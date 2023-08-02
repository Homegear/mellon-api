//
// Created by SLaufer on 12.08.20.
//

#ifndef MELLON_API_SRC_REST_AES_AESKEYSLOT_H_
#define MELLON_API_SRC_REST_AES_AESKEYSLOT_H_

#include "../../RestInfo.h"

#include <functional>

namespace MellonApi::Rest::AesChild
{

class AesKeySlot
{
 private:
  static std::shared_ptr<std::unordered_map<std::string, std::function<BaseLib::PVariable(const PRestInfo&, uint32_t aesKeySlot)>>> _postHandlersPrivate;
 public:
  AesKeySlot() = delete;

  static void init();

  static BaseLib::PVariable post(const PRestInfo& restInfo, uint32_t aesKeySlot);
};

}

#endif //MELLON_API_SRC_REST_CA_CAKEYSLOT_H_
