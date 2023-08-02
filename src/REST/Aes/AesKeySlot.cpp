//
// Created by SLaufer on 12.08.20.
//

#include "AesKeySlot.h"
#include "AesKeySlot/Encrypt.h"
#include "AesKeySlot/Decrypt.h"

#include "../../Gd.h"

namespace MellonApi::Rest::AesChild
{

std::shared_ptr<std::unordered_map<std::string, std::function<BaseLib::PVariable(const PRestInfo&, uint32_t aesKeySlot)>>> AesKeySlot::_postHandlersPrivate;

void AesKeySlot::init() {
  _postHandlersPrivate = std::make_shared<std::unordered_map<std::string, std::function<BaseLib::PVariable(const PRestInfo&, uint32_t certificateSlot)>>>();
  _postHandlersPrivate->emplace("decrypt", std::bind(&AesKeySlotChild::Decrypt::post, std::placeholders::_1, std::placeholders::_2));
  _postHandlersPrivate->emplace("encrypt", std::bind(&AesKeySlotChild::Encrypt::post, std::placeholders::_1, std::placeholders::_2));
}

BaseLib::PVariable AesKeySlot::post(const PRestInfo &restInfo, uint32_t aesKeySlot) {
  if(restInfo->path.size() >= 6) {
    auto handlerIterator = _postHandlersPrivate->find(restInfo->path.at(5));
    if (handlerIterator != _postHandlersPrivate->end()) {
      return handlerIterator->second(restInfo, aesKeySlot);
    }
  }

  return RestServer::getJsonErrorResult("request_error");
}

}