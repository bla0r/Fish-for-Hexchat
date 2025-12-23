#include "ircmessage.h"
#include "utils.h"
#include <iostream>
#include <sstream>
#include <locale>

IrcMessage::IrcMessage(const std::string& message) {
	original = message;
	timestamp = time(NULL);
	doParse();
}

IrcMessage::~IrcMessage() {

}

std::string IrcMessage::getBuffer() {
	return original;
}

void IrcMessage::setDecrypted(const std::string& decr)
{
	decrypted = decr;
}

std::string IrcMessage::getDecryptedRECV() {
	std::string recv = hostmask + " " + callinfo + " ";
	if (!extension.empty()) {
		recv += extension + " ";
	}
	recv += recipient + " :" + decrypted;

	return recv;
}

std::vector<std::string> IrcMessage::getBufferVector() {
	return buffers;
}

std::string IrcMessage::getBufferIndex(unsigned int x)
{
	if (x > buffers.size() - 1) {
		return "";
	}

	return buffers.at(x);
}

void IrcMessage::setArgument(std::string& arg) {
	argument = arg;
	arguments = util::split(arg, " ");
}

std::string IrcMessage::getArgument() {
	return argument;
}

std::string IrcMessage::getOriginal() {
	return stripped;
}

std::string IrcMessage::getStripped() {
	return stripped;
}

time_t IrcMessage::getTimestamp() {
	return timestamp;
}

size_t IrcMessage::getArgSize() {
	return arguments.size();
}

std::string IrcMessage::getArgIndex(unsigned int x) {
	if (x > arguments.size() - 1) {
		return "";
	}

	return arguments.at(x);
}

std::string IrcMessage::getSender() {
	if (hostmask.empty())
		return std::string();

	const char* end = hostmask.c_str();
	while (*end != '\0' && *end != '!' && *end != '@')
		end++;

	if (*hostmask.c_str() == ':') {
		return std::string(hostmask.c_str() + 1, end - hostmask.c_str() - 1);
	}
	else {
		return std::string(hostmask.c_str(), end - hostmask.c_str());
	}
}


bool IrcMessage::isCommandDigit() {

	if ((getBufferIndex(1).length() == 3) && (isdigit(getBufferIndex(1)[0])) && (isdigit(getBufferIndex(1)[1]))
		&& (isdigit(getBufferIndex(1)[2]))) {
		return true;
	}

	return false;
}

bool IrcMessage::isCrypted() {
	if (getArgIndex(0).compare("+OK") == 0) {
		return true;
	}

	if (getArgIndex(0).compare("mcps") == 0) {
		return true;
	}

	return false;
}

bool IrcMessage::isCBC() {
	if (getArgIndex(0).compare("+OK *") == 0) {
		return true;
	}

	if (getArgIndex(0).compare("mcps *") == 0) {
		return true;
	}

	return false;
}

int IrcMessage::removePrefix() {
	if (argument.find("+OK ") == 0)
		argument.erase(0, 4);
	else if (argument.find("mcps ") == 0)
		argument.erase(0, 5);

	if (argument[0] == '*') {
		argument.erase(0, 1);
		return 1;
	}
		
	return 0;

}

std::string IrcMessage::getRecipient() {
	return recipient;
}

std::vector<std::string> IrcMessage::getArguments() {
	return arguments;
}

void IrcMessage::doParse() {
	buffers = util::split(original, " ");
	try {
		hostmask = getBufferIndex(0);
		callinfo = getBufferIndex(1);
		if (isCommandDigit()) {
			extension = getBufferIndex(2);
			recipient = getBufferIndex(3);
		}
		else {
			recipient = getBufferIndex(2);
		}
		
		std::string nb = original;
		if (extension.empty()) {
			nb = nb.erase(0, hostmask.size() + callinfo.size() + recipient.size() + 4);
		}
		else {
			nb = nb.erase(0, hostmask.size() + callinfo.size() + extension.size() + recipient.size() + 5);
		}
		
		argument = nb;
		arguments = util::split(nb, " ");
	}
	catch (...) {
		return;
	}


}

