//
// Created by SLaufer on 12.08.20.
//

#ifndef MELLON_API_SRC_REST_CA_TOKEN_H_
#define MELLON_API_SRC_REST_CA_TOKEN_H_

#include "../../RestInfo.h"

#include <functional>

#include <gnutls/x509.h>

namespace MellonApi::Rest::CaChild
{

class Token
{
 private:
  static std::shared_ptr<std::unordered_map<std::string, std::function<BaseLib::PVariable(const PRestInfo&)>>> _postHandlersPrivate;
 public:
  Token() = delete;

  static void init();

  static BaseLib::PVariable getPublic(const PRestInfo& restInfo, bool &handled);
  static BaseLib::PVariable postPublic(const PRestInfo& restInfo, bool &handled);
  static BaseLib::PVariable postPrivate(const PRestInfo& restInfo, uint32_t dummy);
};

}

#endif //MELLON_API_SRC_REST_CA_CAKEYSLOT_H_
