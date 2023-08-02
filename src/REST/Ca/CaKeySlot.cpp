//
// Created by SLaufer on 12.08.20.
//

#include "CaKeySlot.h"
#include "CaKeySlot/Sign.h"

#include "../../Gd.h"

namespace MellonApi::Rest::CaChild
{

std::shared_ptr<std::unordered_map<std::string, std::function<BaseLib::PVariable(const PRestInfo&, uint32_t certificateSlot)>>> CaKeySlot::_postHandlersPrivate;

void CaKeySlot::init() {
  _postHandlersPrivate = std::make_shared<std::unordered_map<std::string, std::function<BaseLib::PVariable(const PRestInfo&, uint32_t certificateSlot)>>>();
  _postHandlersPrivate->emplace("sign", std::bind(&CaKeySlotChild::Sign::post, std::placeholders::_1, std::placeholders::_2));
}

BaseLib::PVariable CaKeySlot::post(const PRestInfo &restInfo, uint32_t certificateSlot) {
  if(restInfo->path.size() >= 6) {
    auto handlerIterator = _postHandlersPrivate->find(restInfo->path.at(5));
    if (handlerIterator != _postHandlersPrivate->end()) {
      return handlerIterator->second(restInfo, certificateSlot);
    }
  }

  return RestServer::getJsonErrorResult("request_error");
}

}