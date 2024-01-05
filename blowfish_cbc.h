#ifndef __BLOWFISH_CBC_H__
#define __BLOWFISH_CBC_H__
#include <string>
#include <vector>

void blowfish_encrypt_cbc(const std::string& a_in, std::string& ar_out, const std::string& a_key);
int blowfish_decrypt_cbc(const std::string& a_in, std::string& ar_out, const std::string& a_key);

#endif
