#include "FacilitatorClient.h"

FacilitatorClient::~FacilitatorClient() = default;


FacilitatorClient::FacilitatorClient(
    std::string _base_url, std::string _auth, long _connect_timeout_ms, long _total_timeout_ms )
    : base_url(  _base_url  ),
      authHeaderValue(  _auth  ),
      connect_timeout_ms( _connect_timeout_ms ),
      total_timeout_ms( _total_timeout_ms ) {
}