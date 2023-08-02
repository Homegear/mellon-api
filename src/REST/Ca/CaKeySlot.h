//
// Created by SLaufer on 12.08.20.
//

#ifndef MELLON_API_SRC_REST_CA_CAKEYSLOT_H_
#define MELLON_API_SRC_REST_CA_CAKEYSLOT_H_

#include "../../RestInfo.h"

#include <functional>

namespace MellonApi::Rest::CaChild
{

class CaKeySlot
{
 private:
  static std::shared_ptr<std::unordered_map<std::string, std::function<BaseLib::PVariable(const PRestInfo&, uint32_t certificateSlot)>>> _postHandlersPrivate;
 public:
  CaKeySlot() = delete;

  static void init();

  static BaseLib::PVariable post(const PRestInfo& restInfo, uint32_t certificateSlot);
};

}

#endif //MELLON_API_SRC_REST_CA_CAKEYSLOT_H_
