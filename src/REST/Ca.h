//
// Created by SLaufer on 12.08.20.
//

#ifndef MELLON_API_SRC_REST_CA_H_
#define MELLON_API_SRC_REST_CA_H_

#include "../RestInfo.h"

#include <functional>

namespace MellonApi::Rest {

class Ca {
 private:
  static std::shared_ptr<std::unordered_map<std::string, std::function<BaseLib::PVariable(const PRestInfo&, bool &handled)>>> _getHandlersPublic;
  static std::shared_ptr<std::unordered_map<std::string, std::function<BaseLib::PVariable(const PRestInfo&, bool &handled)>>> _postHandlersPublic;
  static std::shared_ptr<std::unordered_map<std::string, std::function<BaseLib::PVariable(const PRestInfo&, const uint32_t certificateSlot)>>> _postHandlersPrivate;
 public:
  Ca() = delete;

  static void init();

  static BaseLib::PVariable getPublic(const PRestInfo &restInfo, bool &handled);
  static BaseLib::PVariable postPublic(const PRestInfo &restInfo, bool &handled);
  static BaseLib::PVariable postPrivate(const PRestInfo &restInfo);
};

}

#endif //MELLON_API_SRC_REST_CA_H_
