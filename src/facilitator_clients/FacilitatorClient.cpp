#include "FacilitatorClient.h"

FacilitatorClient::~FacilitatorClient() = default;


FacilitatorClient::FacilitatorClient(
    std::string _base_url, std::string _auth, long _connect_timeout_ms, long _total_timeout_ms )
    : baseUrl_(  _base_url  ),
      authHeaderValue_(  _auth  ),
      connectTimeoutMs_( _connect_timeout_ms ),
      totalTimeoutMs_( _total_timeout_ms ) {
}