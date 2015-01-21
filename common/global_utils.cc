#include "gears/base/common/global_utils.h"

#define BOOST_NO_TYPEID
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/uuid/uuid_generators.hpp>

std::string16 GetGlobalUUID() {
  boost::uuids::random_generator gen;
  boost::uuids::uuid request_uuid = gen();
  std::ostringstream16 uuid_stream;
  boost::uuids::operator<<(uuid_stream,request_uuid);
  return uuid_stream.str();
}

