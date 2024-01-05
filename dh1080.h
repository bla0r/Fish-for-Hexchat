#ifndef _dh1080_h_
#define _dh1080_h_
#include <string>
bool DH1080_Generate(std::string& ar_priv, std::string& ar_pub);
std::string DH1080_Compute(const std::string& a_priv, const std::string& a_pub);

#endif //_dh1080_h_
