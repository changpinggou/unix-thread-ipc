#ifndef GEARS_BASE_COMMON_DISPATCHER_EXCEPTION_H__
#define GEARS_BASE_COMMON_DISPATCHER_EXCEPTION_H__

#include "gears/base/common/string16.h"
#include "gears/base/common/string_utils.h"
#include "third_party/jsoncpp/json.h"

class Exception16 {
public:
  Exception16 () {}
  Exception16 (const std::string16 &desc) : what_(desc) {}
  virtual ~Exception16 () {}

  const std::string16& what16() const {
    return what_;
  }
 protected:
  std::string16 what_;
};

class DispException : public Exception16 {
 public:
  DispException () {}
  DispException (const std::string16 &desc) : Exception16(desc) {}
  virtual ~DispException () {}
};

inline void DispThrow(const std::string16 &desc){
  throw DispException(desc);
}

inline void DispThrow(int32 error_code, const std::string16 &desc) {
  Json::Value result_json;
  Json::FastWriter writer;
  result_json["error_code"] = error_code;
  result_json["description"] = desc;
  std::string16 result = UTF8ToString16(writer.write(result_json));
  throw DispException(result );
}


#endif //GEARS_BASE_COMMON_DISPATCHER_EXCEPTION_H__
