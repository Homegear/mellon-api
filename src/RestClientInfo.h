/* Copyright 2013-2019 Homegear GmbH */

#ifndef MELLON_API_RESTCLIENTINFO_H
#define MELLON_API_RESTCLIENTINFO_H

#include "Acl.h"
#include <homegear-base/Variable.h>

namespace MellonApi {

class RestClientInfo {
 public:
  int32_t id = -1;
  std::string ipAddress;
  uint16_t port = 0;
  int64_t lastOauthCheck = 0;
  bool authenticated = false;
  bool closeConnection = true;
  std::string certificateDn;
  std::string certificateSerial;
  std::shared_ptr<Acl> acl;
};

typedef std::shared_ptr<RestClientInfo> PClientInfo;
}

#endif