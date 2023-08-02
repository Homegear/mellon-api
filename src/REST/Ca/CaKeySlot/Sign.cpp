//
// Created by SLaufer on 12.08.20.
//

#include "Sign.h"
#include "../../../Gd.h"
#include "../../../HelperFunctions.h"

namespace MellonApi::Rest::CaChild::CaKeySlotChild {

BaseLib::PVariable Sign::post(const MellonApi::PRestInfo &restInfo, uint32_t certificateSlot) {
  auto csrIterator = restInfo->data->structValue->find("csr");
  if (csrIterator == restInfo->data->structValue->end()) return RestServer::getJsonErrorResult("request_error");

  auto &crqPem = csrIterator->second->stringValue;

  std::string crqDn;

  {
    gnutls_x509_crq_t crq;
    gnutls_x509_crq_init(&crq);

    gnutls_datum_t certificateData;
    certificateData.data = (unsigned char *)crqPem.data();
    certificateData.size = crqPem.size();

    if (gnutls_x509_crq_import(crq, &certificateData, gnutls_x509_crt_fmt_t::GNUTLS_X509_FMT_PEM) == GNUTLS_E_SUCCESS) {
      std::array<char, 1024> buffer{};
      size_t bufferSize = buffer.size();
      if (gnutls_x509_crq_get_dn(crq, buffer.data(), &bufferSize) == 0) {
        crqDn = std::string(buffer.begin(), buffer.begin() + bufferSize);
      }
    } else {
      Gd::out.printError("Error (" + restInfo->clientInfo->certificateSerial + "): Could not import CSR.");
    }

    gnutls_x509_crq_deinit(crq);
  }

  std::string type;
  auto signedInfo = HelperFunctions::getDataFromDn(restInfo->clientInfo->certificateDn);
  if (signedInfo) {
    auto typeIterator = signedInfo->structValue->find("type");
    if (typeIterator != signedInfo->structValue->end()) {
      type = typeIterator->second->stringValue;
    }
  }

  if (crqDn.empty() || (crqDn != restInfo->clientInfo->certificateDn && !restInfo->clientInfo->acl->hasTypeSignAccess(type) && !restInfo->clientInfo->acl->hasTypeSignAccess("*"))) {
    if (crqDn != restInfo->clientInfo->certificateDn) Gd::out.printError("Error (" + restInfo->clientInfo->certificateSerial + "): DN of certificate and CSR do not match: " + crqDn + " != " + restInfo->clientInfo->certificateDn);
    return RestServer::getJsonErrorResult("request_error");
  }

  auto signedCertificate = Gd::mellon->signx509csr(crqPem, certificateSlot);
  if (signedCertificate.empty()) return RestServer::getJsonErrorResult("request_error");

  int64_t expirationTime = 0;
  std::string serial;
  bool error = false;

  {
    gnutls_x509_crt_t certificate;
    gnutls_x509_crt_init(&certificate);

    gnutls_datum_t certificateData;
    certificateData.data = (unsigned char *)signedCertificate.data();
    certificateData.size = signedCertificate.size();

    if (gnutls_x509_crt_import(certificate, &certificateData, gnutls_x509_crt_fmt_t::GNUTLS_X509_FMT_PEM) == GNUTLS_E_SUCCESS) {
      std::array<char, 1024> buffer{};
      size_t bufferSize = buffer.size();
      expirationTime = gnutls_x509_crt_get_expiration_time(certificate);
      if(gnutls_x509_crt_get_serial(certificate, buffer.data(), &bufferSize) == GNUTLS_E_SUCCESS) {
        if (bufferSize > buffer.size()) bufferSize = buffer.size();
        serial = BaseLib::HelperFunctions::getHexString(buffer.data(), bufferSize);
      }
    } else {
      Gd::out.printError("Error (" + restInfo->clientInfo->certificateSerial + "): Could not import signed certificate.");
      error = true;
    }

    gnutls_x509_crt_deinit(certificate);
  }

  if (error || expirationTime == 0 || serial.empty()) return RestServer::getJsonErrorResult("request_error");

  std::string serializedAcl;
  if(crqDn == restInfo->clientInfo->certificateDn) {
    //Read ACL only when we are renewing the certificate.
    auto acl = Acl(Gd::db->getCertificateAcl(restInfo->clientInfo->certificateSerial));
    serializedAcl = acl.serialize();
  }

  if (!Gd::db->insertCertificate(serial, crqPem, signedCertificate, expirationTime, serializedAcl)) {
    return RestServer::getJsonErrorResult("request_error");
  }

  return RestServer::getJsonSuccessResult(std::make_shared<BaseLib::Variable>(signedCertificate));
}

}