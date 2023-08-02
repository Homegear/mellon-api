//
// Created by SLaufer on 12.08.20.
//

#ifndef MELLON_API_SRC_REST_AES_H_
#define MELLON_API_SRC_REST_AES_H_

#include "../RestInfo.h"

#include <functional>

namespace MellonApi::Rest {

class Aes {
 private:
  static std::shared_ptr<std::unordered_map<std::string, std::function<BaseLib::PVariable(const PRestInfo&, const uint32_t aesKeySlot)>>> _postHandlersPrivate;
 public:
  Aes() = delete;

  static void init();

  static BaseLib::PVariable post(const PRestInfo &restInfo);
};

}

#endif //MELLON_API_SRC_REST_CA_H_
