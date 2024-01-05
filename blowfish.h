#ifndef __BLOWFISH_H__
#define __BLOWFISH_H__
#include <string>
#include <vector>

void blowfish_encrypt(const std::string& ain, std::string& out, const std::string& key);
int blowfish_decrypt(const std::string& ain, std::string& out, const std::string& key);

#endif