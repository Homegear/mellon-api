//
// Created by SLaufer on 12.08.20.
//

#include "HelperFunctions.h"
#include "Gd.h"

BaseLib::PVariable MellonApi::HelperFunctions::getDataFromDn(const std::string &dn) {
  try {
    //The DN looks like this: CN=<Base64-encoded JSON>
    std::string cn;
    auto dnParts = BaseLib::HelperFunctions::splitAll(dn, ',');
    for (auto &dnPart : dnParts) {
      auto dnPair = BaseLib::HelperFunctions::splitFirst(dnPart, '=');
      BaseLib::HelperFunctions::toLower(dnPair.first);
      if (dnPair.first == "cn") {
        BaseLib::Base64::decode(dnPair.second, cn);
        break;
      }
    }

    if (!cn.empty()) return BaseLib::Rpc::JsonDecoder::decode(cn);
  }
  catch (const std::exception &ex) {
    Gd::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  return {};
}
