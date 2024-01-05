#ifndef __IRCMESSAGE_H__
#define __IRCMESSAGE_H__

#include <string>
#include <vector>

class IrcMessage {
public:
	IrcMessage(const std::string&);
	~IrcMessage();

	void doParse();

	void setArgument(std::string&);
	int removePrefix();

	std::string getArgument();
	std::string getArgIndex(unsigned int x);
	std::vector<std::string> getArguments();
	std::string getStripped();
	std::string getOriginal();
	std::string getRecipient();
	std::string getBuffer();
	void setDecrypted(const std::string&);
	std::vector<std::string> getBufferVector();
	std::string getBufferIndex(unsigned int x);
	time_t getTimestamp();
	size_t getArgSize();
	std::string getDecryptedRECV();
	std::string getSender();
	bool isCommandDigit();
	bool isCrypted();
	bool isCBC();

private:
	std::string original;
	std::string stripped;
	std::string recipient;
	std::string hostmask;
	std::string callinfo;
	std::string extension;
	std::string channel;
	std::string argument;
	std::string decrypted;
	std::vector<std::string> arguments;
	std::vector<std::string> buffers;
	std::string network;
	std::string echostring;
	time_t timestamp;

};

#endif
