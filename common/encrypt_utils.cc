#include <windows.h>
#include "gears/base/common/encrypt_utils.h"
//fully random pass/iv, make no sence
static const char kPasswordCanonicalFactor[48] =
{
  0xaf, 0x37, 0x19, 0xe3,
  0xb2, 0x6c, 0x55, 0xdf,
  0x40, 0x8c, 0x16, 0x3e,
  0x8a, 0x11, 0x3a, 0x28,
  0xd0, 0x0a, 0x76, 0xf2,
  0xd5, 0x7f, 0x2b, 0xdf,
  0xa8, 0x06, 0xd8, 0x52,
  0xca, 0x74, 0x64, 0x58,
  0x8d, 0x8d, 0xc2, 0xdd,
  0x04, 0xa8, 0xc8, 0xd8,
  0x49, 0x44, 0x94, 0x91,
  0xea, 0x71, 0x58, 0xaa
};

void EncryptUtils::MD5DigestString(unsigned char* src, size_t src_len, unsigned char* dst) {
  MD5_CTX md5ctx;
  MD5Init(&md5ctx);
  MD5Update(&md5ctx, src, src_len);
  MD5Final(dst, &md5ctx);
}

void EncryptUtils::DigestTo128BitBase16(unsigned char *digest, char *zBuf) {
  static char const zEncode[] = "0123456789abcdef";
  int i, j;

  for(j=i=0; i<16; i++){
    int a = digest[i];
    zBuf[j++] = zEncode[(a>>4)&0xf];
    zBuf[j++] = zEncode[a & 0xf];
  }
  zBuf[j] = 0;
}

std::string16 EncryptUtils::CanonicalPassword(const std::string16 &password ){
  //128 bits digest code
  unsigned char digest[16];
  //base16 encoded 128 digets and '\0', 128 / 4 + 1 = 33
  char digest_base16[33];

  std::string ascii_pwd = String16ToUTF8(password);

  MD5DigestString((unsigned char*)ascii_pwd.c_str(),
    ascii_pwd.length(),
    digest);

  DigestTo128BitBase16(digest, digest_base16);
  return UTF8ToString16(digest_base16);
}

std::string16 EncryptUtils::Encrypt( const std::string16 &password ) {
  static const int kAESKeyLength = 128;
  static const int kAESOctectPerCipherBlock = kAESKeyLength / 0x08;
  AES_KEY aes_key;
  std::string ascii_key;

  std::basic_string<unsigned char> iv(kPasswordCanonicalFactor + 0x09,
    kPasswordCanonicalFactor + kAESOctectPerCipherBlock + 0x09);
  std::basic_string<unsigned char> pwd(kPasswordCanonicalFactor + 0x12,
    kPasswordCanonicalFactor + kAESOctectPerCipherBlock + 0x12);

  (void)String16ToUTF8( password, &ascii_key );
  AES_set_encrypt_key( pwd.c_str(),  kAESKeyLength, &aes_key );
  AES_cbc_encrypt(reinterpret_cast< const unsigned char*>(ascii_key.c_str()),
    (unsigned char*)ascii_key.c_str(),
    ascii_key.size(),
    &aes_key,
    (unsigned char*)iv.c_str(),
    AES_ENCRYPT);

  Base64Encode(ascii_key, &ascii_key);
  return UTF8ToString16(ascii_key);
}


std::string16 EncryptUtils::Decrypt( const std::string16 &password ) {
  static const int kAESKeyLength = 128;
  static const int kAESOctectPerCipherBlock = kAESKeyLength / 0x08;
  AES_KEY aes_key;
  std::string ascii_key;
  std::basic_string<unsigned char> iv(kPasswordCanonicalFactor + 0x09,
    kPasswordCanonicalFactor + kAESOctectPerCipherBlock + 0x09);
  std::basic_string<unsigned char> pwd(kPasswordCanonicalFactor + 0x12,
    kPasswordCanonicalFactor + kAESOctectPerCipherBlock + 0x12);

  Base64Decode(password, &ascii_key);

  AES_set_decrypt_key( pwd.c_str(),  kAESKeyLength, &aes_key );
  AES_cbc_encrypt(reinterpret_cast< const unsigned char*>(ascii_key.c_str()),
    (unsigned char*)ascii_key.c_str(),
    ascii_key.size(),
    &aes_key,
    (unsigned char*)iv.c_str(),
    AES_DECRYPT);

  return UTF8ToString16(ascii_key);
}