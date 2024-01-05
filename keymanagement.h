#include <iostream>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <mutex>

enum FishMode {
	FISH_ECB_MODE,
	FISH_CBC_MODE
};

class KeyManagement {
private:
	std::string conffile;
	std::string password;
	std::unordered_map<std::string, std::string> keystore;
	std::string keystorePassword;
	std::mutex mutex;

	std::string escapeNickname(const std::string&);
	
	bool saveKeystore();
	bool loadKeys();
	void createConfigFile();
	
public:
	KeyManagement(const std::string&, const std::string& pass = "");
	~KeyManagement();
	bool keystoreDeleteNick(const std::string&, const std::string& nick);
	std::string keystoreGetKey(const std::string& network, const std::string& nick, FishMode& mode);
	std::string getNickValue(const std::string&, const std::string& item);
	bool keystoreStoreKey(const std::string network, const std::string& nick, const std::string& key, FishMode mode);

	
};

