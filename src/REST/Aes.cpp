//
// Created by SLaufer on 12.08.20.
//

#include "Aes.h"
#include "Aes/AesKeySlot.h"

#include "../Gd.h"

namespace MellonApi::Rest {

std::shared_ptr<std::unordered_map<std::string, std::function<BaseLib::PVariable(const PRestInfo&, uint32_t aesKeySlot)>>> Aes::_postHandlersPrivate;

void Aes::init() {
  _postHandlersPrivate = std::make_shared<std::unordered_map<std::string, std::function<BaseLib::PVariable(const PRestInfo&,  uint32_t certificateSlot)>>>();
  _postHandlersPrivate->emplace(":slot", std::bind(&AesChild::AesKeySlot::post, std::placeholders::_1, std::placeholders::_2));

  AesChild::AesKeySlot::init();
}

BaseLib::PVariable Aes::post(const MellonApi::PRestInfo &restInfo) {
  if(restInfo->path.size() >= 5) {
    auto aesKeySlot = BaseLib::Math::getUnsignedNumber(restInfo->path.at(4));
    auto handlerIterator = _postHandlersPrivate->find(":slot");
    if (handlerIterator != _postHandlersPrivate->end()) {
      return handlerIterator->second(restInfo, aesKeySlot);
    }
  }

  return RestServer::getJsonErrorResult("request_error");
}

}