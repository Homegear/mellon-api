//
// Created by SLaufer on 12.08.20.
//

#ifndef MELLON_API_SRC_ACL_H_
#define MELLON_API_SRC_ACL_H_

#include <homegear-base/Variable.h>
#include <unordered_set>

namespace MellonApi {

class Acl {
 private:
  std::string _serializedAcl;
  std::unordered_set<std::string> _typeSignAccess;
  std::unordered_map<std::string, std::unordered_set<std::string>> _pathAccess;
  std::unordered_set<std::string> _tokenCreationAccess;
 public:
  Acl();
  explicit Acl(const BaseLib::PVariable &aclStruct);

  static std::shared_ptr<Acl> fromSerial(const std::string &serial);
  static std::shared_ptr<Acl> fromType(const std::string &type);
  std::string serialize();

  bool isEmpty();
  bool hasPathAccess(const std::string &path, const std::string &method);
  bool hasTypeSignAccess(const std::string &type);
  bool hasTokenCreationAccess(const std::string &type);
};

}

#endif //MELLON_API_SRC_ACL_H_
