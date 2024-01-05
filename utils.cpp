#include "utils.h"
#include <openssl/bio.h>
#include <openssl/evp.h>
#include <openssl/buffer.h>

namespace util {
	std::vector<std::string> split(const std::string& str, const std::string& delim) {
		std::vector<std::string> v;
		if (delim.empty() || str.empty()) {
			return v;
		}
		std::string::size_type pos = 0, start = 0;
		while (pos != std::string::npos) {
			pos = str.find(delim, start);
			v.push_back(str.substr(start, pos - start));
			start = pos + delim.size();
		}
		return v;
	}

	void remove_bad_chars(std::string& str) {
		std::string::size_type i;
		while (i = str.find('\x00', 0), i != std::string::npos)
			str.erase(i, 1);
		while (i = str.find_first_of("\x0d\x0a"), i != std::string::npos)
			str.erase(i, 1);
	}


	std::string Base64_Encode(const std::string& a_input)
	{
		BIO* l_mem, * l_b64;
		std::string l_result;

		if (a_input.size() == 0) return l_result;

		l_b64 = BIO_new(BIO_f_base64());
		if (!l_b64)
		{
			return l_result;
		}

		BIO_set_flags(l_b64, BIO_FLAGS_BASE64_NO_NL);

		l_mem = BIO_new(BIO_s_mem());
		if (!l_mem)
		{
			BIO_free_all(l_b64);
			return l_result;
		}

		l_b64 = BIO_push(l_b64, l_mem);

		if (BIO_write(l_b64, a_input.c_str(), a_input.size()) == (int)a_input.size())
		{
			BUF_MEM* l_ptr;

			BIO_flush(l_b64);
			BIO_get_mem_ptr(l_b64, &l_ptr);

			l_result.append(l_ptr->data, l_ptr->length);
		}

		BIO_free_all(l_b64);

		return l_result;
	}


	std::string Base64_Decode(const std::string& a_input)
	{
		BIO* l_b64;
		std::string l_result;

		if (a_input.size() == 0) return l_result;

		l_b64 = BIO_new(BIO_f_base64());
		if (l_b64)
		{
			char* l_buf = new char[256];
			BIO_set_flags(l_b64, BIO_FLAGS_BASE64_NO_NL);

			BIO* l_mem = BIO_new_mem_buf((void*)a_input.c_str(), a_input.size());
			if (l_mem)
			{
				int l_bytesRead;
				l_b64 = BIO_push(l_b64, l_mem);
				while ((l_bytesRead = BIO_read(l_b64, l_buf, 256)) > 0)
				{
					l_result.append(l_buf, l_bytesRead);
				}
			}

			BIO_free_all(l_b64);
			delete[] l_buf;
		}

		return l_result;
	}

	void StrTrimRight(std::string& a_str)
	{
		std::string::size_type l_pos = a_str.find_last_not_of(" \t\r\n");

		if (l_pos != std::string::npos)
		{
			a_str.erase(l_pos + 1);
		}
		else
		{
			a_str.clear();
		}
	}
};