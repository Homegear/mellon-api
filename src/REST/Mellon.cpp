//
// Created by SLaufer on 12.08.20.
//

#include "Mellon.h"
#include "../Gd.h"

namespace MellonApi::Rest {

BaseLib::PVariable Mellon::get(const PRestInfo &restInfo) {
  BaseLib::PVariable mellonInfo = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tStruct);
  mellonInfo->structValue->emplace("version", std::make_shared<BaseLib::Variable>(Gd::mellon->getVersion()));
  mellonInfo->structValue->emplace("serialNumber", std::make_shared<BaseLib::Variable>(Gd::mellon->getSerialNumber()));
  mellonInfo->structValue->emplace("name", std::make_shared<BaseLib::Variable>(Gd::mellon->getName()));
  mellonInfo->structValue->emplace("ready", std::make_shared<BaseLib::Variable>(Gd::mellon->isReady()));
  mellonInfo->structValue->emplace("loggedIn", std::make_shared<BaseLib::Variable>(Gd::mellon->isLoggedIn()));

  mellonInfo->structValue->emplace("secondsInUse1h", std::make_shared<BaseLib::Variable>(Gd::mellon->getUsageHour()));
  mellonInfo->structValue->emplace("secondsInUse24h", std::make_shared<BaseLib::Variable>(Gd::mellon->getUsageDay()));

  BaseLib::PVariable resultJson = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tStruct);
  resultJson->structValue->emplace("success", std::make_shared<BaseLib::Variable>(true));
  resultJson->structValue->emplace("result", mellonInfo);

  return resultJson;
}

}