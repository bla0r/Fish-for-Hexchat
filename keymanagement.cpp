#include "keymanagement.h"
#include "blowfish_cbc.h"


KeyManagement::KeyManagement(const std::string& conf, const std::string& pass) :
	conffile(conf),
	password(pass)
{
	if (!loadKeys()) {
		createConfigFile();
	}
}

KeyManagement::~KeyManagement() {
	saveKeystore();
}

bool KeyManagement::loadKeys() {
	std::ifstream configFile(conffile);
	if (!configFile.is_open()) {
		createConfigFile();
		return true;
	}

	std::string line;
	std::string currentNick;
	while (std::getline(configFile, line)) {
		if (!line.empty() && line[0] == '[') {
			currentNick = line.substr(1, line.size() - 2);
		}
		else {
			if (line.find("key=") == 0) {
				std::string keyString = line.substr(4);
				std::string decryptedKey;

				if (keyString.find("+OK") == 0) {
					std::string encryptedKey = keyString.substr(4);
					if (!password.empty()) {
						blowfish_decrypt_cbc(encryptedKey, decryptedKey, password);
					}
					else {
						decryptedKey = encryptedKey;
					}
				}
				else {
					decryptedKey = keyString;
				}

				keystore[currentNick] += "key=" + decryptedKey + "\n";
			}
			else {
				keystore[currentNick] += line + "\n";
			}
		}
	}

	return true;
}



void KeyManagement::createConfigFile() {
	std::ofstream configFile(conffile);
}

std::string KeyManagement::escapeNickname(const std::string& nick) {
	std::string escaped = nick;
	for (char& c : escaped) {
		if (c == '[')
			c = '~';
		else if (c == ']')
			c = '!';
	}
	return escaped;
}

std::string KeyManagement::getNickValue(const std::string& nick, const std::string& item) {
	std::lock_guard<std::mutex> lock(mutex);
	auto it = keystore.find(nick);
	if (it != keystore.end()) {
		if (it->first == nick) {
			auto group = it->second;
			auto pos = group.find(item + "=");
			if (pos != std::string::npos) {
				pos += item.length() + 1;
				auto endPos = group.find('\n', pos);
				return group.substr(pos, endPos - pos);
			}
		}
	}
	return "";
}

bool KeyManagement::saveKeystore() {
	std::ofstream configFile(conffile);
	if (!configFile.is_open()) {
		return false;
	}

	std::lock_guard<std::mutex> lock(mutex);

	for (const auto& entry : keystore) {
		configFile << "[" << entry.first << "]\n";

		std::istringstream iss(entry.second);
		std::string line;
		while (std::getline(iss, line)) {
			if (line.find("key=") == 0) {
				if (!password.empty()) {
					std::string encryptedKey;
					blowfish_encrypt_cbc(line.substr(4), encryptedKey, password);
					configFile << "key=+OK " + encryptedKey + "\n";
				}
				else {
					configFile << line + "\n";
				}
			}
			else {
				configFile << line + "\n";
			}
		}
	}

	return true;
}


std::string KeyManagement::keystoreGetKey(const std::string& network, const std::string& nick, FishMode& mode) 
{
	{
		std::lock_guard<std::mutex> lock(mutex);
		std::string escapedNick = escapeNickname(nick);

		auto it = keystore.find(escapedNick);
		if (it != keystore.end()) {
			auto value = it->second;
			auto keyPos = value.find("key=");
			auto networkPos = value.find("network=");
			auto modePos = value.find("mode=");

			if (networkPos != std::string::npos) {
				auto networkEndPos = value.find('\n', networkPos);
				auto storedNetwork = value.substr(networkPos + 8, networkEndPos - networkPos - 8);

				if (network == storedNetwork) {
					if (keyPos != std::string::npos) {
						auto keyEndPos = value.find('\n', keyPos);
						auto key = value.substr(keyPos + 4, keyEndPos - keyPos - 4);

						if (modePos != std::string::npos) {
							auto modeEndPos = value.find('\n', modePos);
							auto modeStr = value.substr(modePos + 5, modeEndPos - modePos - 5);

							if (!modeStr.empty()) {
								mode = (modeStr[0] == '1') ? FISH_ECB_MODE : FISH_CBC_MODE;
							}
						}

						return key;
					}
				}
			}
		}

		return "";
	}
}

bool KeyManagement::keystoreStoreKey(const std::string network, const std::string& nick, const std::string& key, FishMode mode) {
	{
		std::lock_guard<std::mutex> lock(mutex);
		std::ostringstream entry;
		entry << "network=" << network << "\n";
		entry << "key=" << key << "\n";
		entry << "mode=" << ((mode == FISH_ECB_MODE) ? '1' : '2') << "\n";

		std::string escapedNick = escapeNickname(nick);
		keystore[escapedNick] = entry.str();
	}
	return saveKeystore();
}

bool KeyManagement::keystoreDeleteNick(const std::string& network, const std::string& nick) {
	std::string escapedNick = escapeNickname(nick);
	{
		std::lock_guard<std::mutex> lock(mutex);

		auto it = keystore.find(escapedNick);
		if (it != keystore.end()) {
			auto value = it->second;
			auto networkPos = value.find("network=");
			if (networkPos != std::string::npos) {
				auto networkEndPos = value.find('\n', networkPos);
				auto storedNetwork = value.substr(networkPos + 8, networkEndPos - networkPos - 8);

				if (network == storedNetwork) {
					keystore.erase(it);
				}
			}
		}
	}
	saveKeystore();
	return false;
}

