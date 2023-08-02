//
// Created by SLaufer on 12.08.20.
//

#include "Ca.h"
#include "Ca/CaKeySlot.h"
#include "Ca/Token.h"

#include "../Gd.h"

namespace MellonApi::Rest {

std::shared_ptr<std::unordered_map<std::string, std::function<BaseLib::PVariable(const PRestInfo &, bool &handled)>>> Ca::_getHandlersPublic;
std::shared_ptr<std::unordered_map<std::string, std::function<BaseLib::PVariable(const PRestInfo &, bool &handled)>>> Ca::_postHandlersPublic;
std::shared_ptr<std::unordered_map<std::string, std::function<BaseLib::PVariable(const PRestInfo &, uint32_t certificateSlot)>>> Ca::_postHandlersPrivate;

void Ca::init() {
  _getHandlersPublic = std::make_shared<std::unordered_map<std::string, std::function<BaseLib::PVariable(const PRestInfo &, bool &handled)>>>();
  _getHandlersPublic->emplace("token", std::bind(&CaChild::Token::getPublic, std::placeholders::_1, std::placeholders::_2));

  _postHandlersPublic = std::make_shared<std::unordered_map<std::string, std::function<BaseLib::PVariable(const PRestInfo &, bool &handled)>>>();
  _postHandlersPublic->emplace("token", std::bind(&CaChild::Token::postPublic, std::placeholders::_1, std::placeholders::_2));

  _postHandlersPrivate = std::make_shared<std::unordered_map<std::string, std::function<BaseLib::PVariable(const PRestInfo &, uint32_t certificateSlot)>>>();
  _postHandlersPrivate->emplace(":slot", std::bind(&CaChild::CaKeySlot::post, std::placeholders::_1, std::placeholders::_2));
  _postHandlersPrivate->emplace("token", std::bind(&CaChild::Token::postPrivate, std::placeholders::_1, std::placeholders::_2));

  CaChild::CaKeySlot::init();
  CaChild::Token::init();
}

BaseLib::PVariable Ca::getPublic(const MellonApi::PRestInfo &restInfo, bool &handled) {
  if (restInfo->path.size() >= 5) {
    auto handlerIterator = _getHandlersPublic->find(restInfo->path.at(4));
    if (handlerIterator != _getHandlersPublic->end()) {
      return handlerIterator->second(restInfo, handled);
    }
  }

  return RestServer::getJsonErrorResult("request_error");
}

BaseLib::PVariable Ca::postPublic(const MellonApi::PRestInfo &restInfo, bool &handled) {
  if (restInfo->path.size() >= 5) {
    auto handlerIterator = _postHandlersPublic->find(restInfo->path.at(4));
    if (handlerIterator != _postHandlersPublic->end()) {
      return handlerIterator->second(restInfo, handled);
    }
  }

  return RestServer::getJsonErrorResult("request_error");
}

BaseLib::PVariable Ca::postPrivate(const MellonApi::PRestInfo &restInfo) {
  if (restInfo->path.size() >= 5) {
    auto handlerIterator = _postHandlersPrivate->find(restInfo->path.at(4));
    if (handlerIterator != _postHandlersPrivate->end()) {
      return handlerIterator->second(restInfo, 0);
    } else {
      auto certificateSlot = BaseLib::Math::getUnsignedNumber(restInfo->path.at(4));
      handlerIterator = _postHandlersPrivate->find(":slot");
      if (handlerIterator != _postHandlersPrivate->end()) {
        return handlerIterator->second(restInfo, certificateSlot);
      }
    }
  }

  return RestServer::getJsonErrorResult("request_error");
}

}