//
// Created by SLaufer on 12.08.20.
//

#include "Crl.h"
#include "../Gd.h"

namespace MellonApi::Rest {

BaseLib::PVariable Crl::get(const PRestInfo &restInfo) {
  BaseLib::PVariable resultJson = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tStruct);
  resultJson->structValue->emplace("success", std::make_shared<BaseLib::Variable>(true));
  resultJson->structValue->emplace("result", Gd::db->getCrl());

  return resultJson;
}

}