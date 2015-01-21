#ifndef GEARS_NTES_ENCRYPT_UTILS_H__
#define GEARS_NTES_ENCRYPT_UTILS_H__

#include "third_party/BigFileUpload/BigFileUpload/md5.h"
#include "gears/base/common/string_utils.h"
#include "gears/base/common/base64.h"
#include "third_party/openssl/aes.h"

class EncryptUtils {
public:
  static void MD5DigestString(unsigned char* src, size_t src_len, unsigned char* dst);
  static void DigestTo128BitBase16(unsigned char *digest, char *zBuf);
  static std::string16 CanonicalPassword(const std::string16 &password );
  static std::string16 Encrypt( const std::string16 &password );
  static std::string16 Decrypt( const std::string16 &password );
};

#endif //GEARS_NTES_ENCRYPT_UTILS_H__