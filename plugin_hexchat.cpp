#include "config.h"
#include <iostream>
#include <string>
#include <cstring>
#include <cstdlib>
#include <unordered_map>
#include <glib.h>
#include "hexchat-plugin.h"
#include "ircmessage.h"
#include "keymanagement.h"
#include "blowfish.h"
#include "blowfish_cbc.h"
#include "dh1080.h"
#include "utils.h"

KeyManagement* keymgm;

const char* fish_modes[] = {
	"ECB",
	"CBC"
};

static const char plugin_name[] = "Blowfish";
static const char plugin_desc[] = "Encryption plugin for the FiSH protocol.";
static const char plugin_version[] = "1.0.1";
static const char usage_marker[] = "Usage: MARKER [<append|prepend|disabled>";
static const char usage_setkey[] = "Usage: SETKEY [<nick or #channel>] [<mode>:]<password>, sets the key for a channel or nick. Modes: ECB, CBC";
static const char usage_delkey[] = "Usage: DELKEY [<nick or #channel>], deletes the key for a channel or nick";
static const char usage_keyx[] = "Usage: KEYX [<nick>], performs DH1080 key-exchange with <nick>";
static const char usage_key[] = "Usage: KEY [<nick> or #channel], prints the key";
static const char usage_topic[] = "Usage: SETTOPIC+ <topic>, sets a new encrypted topic for the current channel";

static hexchat_plugin* ph;
static std::unordered_map < std::string, std::string > pending_exchanges;
volatile int modus = 2;

std::string getKeyFromPending(const std::string& key) {
	auto iter = pending_exchanges.find(key);
	if (iter != pending_exchanges.end()) {
		std::string value = iter->second;
		pending_exchanges.erase(iter);
		return value;
	}
	return "";
}

/**
 * Compare two nicks using the current plugin
 */
int irc_nick_cmp(const char* a, const char* b) {
	return hexchat_nickcmp(ph, a, b);
}

bool irc_is_query(const std::string& name) {
	const char* chantypes = hexchat_list_str(nullptr, nullptr, "chantypes");

	return strchr(chantypes, name[0]) == nullptr;
}

std::string get_config_filename() {
	const char* configDir = hexchat_get_info(ph, "configdir");
	const char* filenameUtf8 = "blowfish.conf";

	// Combine paths manually
	std::string filenameFs = std::string(configDir) + "/" + filenameUtf8;

	return filenameFs;
}

static hexchat_context* find_context_on_network(const char* name) {
	hexchat_list* channels;
	hexchat_context* ret = NULL;
	int id;

	if (hexchat_get_prefs(ph, "id", NULL, &id) != 2)
		return NULL;

	channels = hexchat_list_get(ph, "channels");
	if (!channels)
		return NULL;

	while (hexchat_list_next(ph, channels)) {
		int chan_id = hexchat_list_int(ph, channels, "id");
		const char* chan_name = hexchat_list_str(ph, channels, "channel");

		if (chan_id == id && chan_name && irc_nick_cmp(chan_name, name) == 0) {
			ret = (hexchat_context*)hexchat_list_str(ph, channels, "context");
			break;
		}
	};

	hexchat_list_free(ph, channels);
	return ret;
}

static int handle_outgoing(char* word[], char* word_eol[], void* userdata) {
	FishMode mode;
	std::string network = hexchat_get_info(ph, "network");
	std::string channel = hexchat_get_info(ph, "channel");
	
	std::string key = keymgm->keystoreGetKey(network, channel, mode);
	if (key.empty()) {
		return HEXCHAT_EAT_NONE;
	}
	
	std::string plaintext = word_eol[1];
	std::string encryptedcontext;
	if (mode == FISH_CBC_MODE) {
		blowfish_encrypt_cbc(plaintext, encryptedcontext, key);
	}
	else {
		blowfish_encrypt(plaintext, encryptedcontext, key);
	}

	if (encryptedcontext.empty()) {
		return HEXCHAT_EAT_NONE;
	}

	std::string command = "PRIVMSG " + channel + " :+OK ";
	if (mode == FISH_CBC_MODE)
		command += "*";

	command += encryptedcontext;
	
	std::string l_mark = "[" + std::string(fish_modes[mode]) + "]";
	if (modus == 1)
		plaintext.append(" ").append(l_mark);
	else if (modus == 2)
		plaintext.insert(0, l_mark).insert(l_mark.length(), " ");

	std::string message = plaintext;

	/* Display message */
	hexchat_emit_print(ph, "Your Message", hexchat_get_info(ph, "nick"), message.c_str(), NULL, NULL);

	hexchat_commandf(ph, "%s", command.c_str());


	return HEXCHAT_EAT_HEXCHAT;
}

int handle_incoming(char* word[], char* word_eol[], hexchat_event_attrs* attrs, void* userdata) {
	std::string raw_message = word_eol[1];

	auto msg = std::make_shared<IrcMessage>(raw_message);
	if (!msg->isCrypted()) {
		return HEXCHAT_EAT_NONE;
	}

	FishMode mode = FISH_ECB_MODE;
	std::string network = hexchat_get_info(ph, "network");
	std::string key;
	if (msg->getRecipient()[0] == '#')
		key = keymgm->keystoreGetKey(network, msg->getRecipient(), mode);
	else
		key = keymgm->keystoreGetKey(network, msg->getSender(), mode);
	std::string l_mark;

	if (key.empty()) {
		return HEXCHAT_EAT_NONE;
	}

	if (mode == FISH_CBC_MODE || mode == FISH_ECB_MODE) {
		if (msg->removePrefix() == 1) {
			mode = FISH_CBC_MODE;
			l_mark = "[CBC]";
		}
		else {
			l_mark = "[ECB]";
		}
	}

	std::string l_message = msg->getArgument();
	util::StrTrimRight(l_message);
	std::string decrypted;
	
	int decryptstatus;
	if (mode == FISH_ECB_MODE)
		decryptstatus = blowfish_decrypt(l_message, decrypted, key);
	else {
		decryptstatus = blowfish_decrypt_cbc(l_message, decrypted, key);
	}

	switch (decryptstatus) {
		case -1:
			decrypted = l_message + "=[FiSH: DECRYPTION FAILED!]=";
			break;
		case 1:
			decrypted += "\x02&\x02";
		case 0:
			//we put here the marker if we want to...
			if (modus == 1)
				decrypted.append(" ").append(l_mark);
			else if (modus == 2) 
				decrypted.insert(0, l_mark).insert(l_mark.length(), " ");
	}
	
	if (decrypted.empty()) {
		return HEXCHAT_EAT_NONE;
	}
	msg->setDecrypted(decrypted);

	/* decrypted + 'RECV ' + '@time=YYYY-MM-DDTHH:MM:SS.fffffZ ' */
	std::string message = "RECV ";

	// Add timestamp if available
	if (attrs->server_time_utc) {
		// Dummy implementation for timestamp formatting
		char timestamp[34];
		snprintf(timestamp, sizeof(timestamp), "time=%llu", (unsigned long long)attrs->server_time_utc);
		message += '@' + std::string(timestamp) + ' ';
	}
	
	message += msg->getDecryptedRECV();

	/* Fake server message
	 * RECV command will throw this function again, if message have multiple
	 * encrypted data, we will decrypt all */
	hexchat_command(ph, message.c_str());

	return HEXCHAT_EAT_HEXCHAT;
}

static int handle_key(char* word[], char* word_eol[], void* userdata) {
	std::string recipient;
	int ctx_type = 0;
	FishMode mode = FISH_ECB_MODE;

	if (*word[2] != '\0') {
		recipient = word_eol[2];
		size_t pos = recipient.find_last_not_of(" \t");
		if (pos != std::string::npos) {
			recipient.erase(pos + 1);
		}
	}
	else {
		recipient = hexchat_get_info(ph, "channel");
		ctx_type = hexchat_list_int(ph, NULL, "type");

		/* Only allow channel or dialog */
		if (ctx_type < 2 || ctx_type > 3) {
			hexchat_printf(ph, "%s\n", "Usage: /KEY <nickname>");
			return HEXCHAT_EAT_HEXCHAT;
		}
	}
	std::string network = hexchat_get_info(ph, "network");
	std::string key = keymgm->keystoreGetKey(network, recipient, mode);
	if (key.empty()) {
		hexchat_printf(ph, "FiSH: No valid key found for %s!\n", recipient.c_str());
	}
	else {
		hexchat_printf(ph, "FiSH: Key for %s : %s\n", recipient.c_str(), key.c_str());
	}

	return HEXCHAT_EAT_HEXCHAT;
}

static int handle_marker(char* word[], char* word_eol[], void* userdata) {
	std::string mode = word_eol[2];
	//// 1 = append, 2 = prepend, 0 = disabled
	if (mode.find("append") == 0) {
		modus = 1;
		hexchat_printf(ph, "FiSH: Prefix set to append\n");
	}
	else if (mode.find("prepend") == 0) {
		modus = 2;
		hexchat_printf(ph, "FiSH: Prefix set to prepend\n");
	}
	else {
		modus = 0;
		hexchat_printf(ph, "FiSH: Prefix disabled\n");
	}

	return HEXCHAT_EAT_HEXCHAT;
}

static int handle_setkey(char* word[], char* word_eol[], void* userdata) {
	const char* nick;
	const char* key;
	FishMode mode;
	std::string network = hexchat_get_info(ph, "network");
	/* Check syntax */
	if (*word[2] == '\0') {
		hexchat_printf(ph, "%s\n", "Usage: /SETKEY <nickname> <password>");
		return HEXCHAT_EAT_HEXCHAT;
	}

	if (*word[3] == '\0') {
		/* /setkey password */
		nick = hexchat_get_info(ph, "channel");
		key = word_eol[2];
	}
	else {
		/* /setkey #channel password */
		nick = word[2];
		key = word_eol[3];
	}

	mode = FISH_ECB_MODE;
	if (strncmp("cbc:", key, 4) == 0) {
		key += 4;
		mode = FISH_CBC_MODE;
	}
	else if (strncmp("ecb:", key, 4) == 0) {
		key += 4;
	}

	/* Set password */
	
	if (keymgm->keystoreStoreKey(network, nick, key, mode)) {
		hexchat_printf(ph, "FiSH: Key for %s set to *censored*\n", nick);
	}
	else {
		hexchat_printf(ph, "\00305FiSH: Failed to store key in addon_blowfish.conf\n");
	}

	return HEXCHAT_EAT_HEXCHAT;
}

static int handle_delkey(char* word[], char* word_eol[], void* userdata) {
	std::string nick;
	std::string network;
	int ctx_type = 0;

	/* Delete key from input */
	if (*word[2] != '\0') {
		nick = std::string(word_eol[2]);
		nick = nick.substr(0, nick.find_last_not_of(" \t") + 1);
		network = hexchat_get_info(ph, "network");
	}
	else { /* Delete key from current context */
		nick = hexchat_get_info(ph, "channel");
		network = hexchat_get_info(ph, "network");
		ctx_type = hexchat_list_int(ph, NULL, "type");

		/* Only allow channel or dialog */
		if (ctx_type < 2 || ctx_type > 3) {
			hexchat_printf(ph, "%s\n", usage_delkey);
			return HEXCHAT_EAT_HEXCHAT;
		}
	}

	/* Delete the given nick from the key store */
	if (keymgm->keystoreDeleteNick(network, nick)) {
		hexchat_printf(ph, "FiSH: Removed key for %s\n", nick.c_str());
	}

	return HEXCHAT_EAT_HEXCHAT;
}

static int handle_keyx_notice(char* word[], char* word_eol[], void* userdata) {
	std::string network = hexchat_get_info(ph, "network");
	std::string raw_message = word_eol[1];
	FishMode mode = FISH_ECB_MODE;
	auto msg = std::make_shared<IrcMessage>(raw_message);
	std::string privkey;

	if (!msg->getArgIndex(0).compare("DH1080_FINISH")) 
	{

		 if (!msg->getArgIndex(2).compare("CBC")) {
			 mode = FISH_CBC_MODE;
		 }

		 privkey = getKeyFromPending(msg->getSender());
		 if (privkey.empty()) {
			 return HEXCHAT_EAT_ALL;
		 }

	} 
	else if (!msg->getArgIndex(0).compare("DH1080_INIT")) 
	{
		if (!msg->getArgIndex(2).compare("CBC")) {
			mode = FISH_CBC_MODE;
		}

		std::string pubkey;
		hexchat_printf(ph, "FiSH: Received public key from %s (%s), sending mine...", msg->getSender().c_str(), std::string(fish_modes[mode]).c_str());
		if (DH1080_Generate(privkey, pubkey)) {
			hexchat_commandf(ph, "quote NOTICE %s :DH1080_FINISH %s%s", msg->getSender().c_str(), pubkey.c_str(), (mode == FISH_CBC_MODE) ? " CBC" : "");
		}
		else {
			hexchat_print(ph, "FiSH: Failed to generate keys");
		}
	}
	else {
		return HEXCHAT_EAT_ALL;
	}

	std::string secretkey = DH1080_Compute(privkey, msg->getArgIndex(1));
	if (secretkey.empty()) {
		hexchat_print(ph, "FiSH: Failed to create secret key!");
		return HEXCHAT_EAT_ALL;
	}

	if (keymgm->keystoreStoreKey(network, msg->getSender(), secretkey, mode)) {
		hexchat_printf(ph, "FiSH: Stored new key for %s (%s)", msg->getSender().c_str(), std::string(fish_modes[mode]));
	}

	return HEXCHAT_EAT_ALL;
}

static int handle_crypt_topic(char* word[], char* word_eol[], void* userdata) {
	std::string topic = word_eol[2];
	FishMode mode;

	if (hexchat_list_int(ph, NULL, "type") != 2) {
		hexchat_printf(ph, "Please change to the channel window where you want to set the topic!");
		return HEXCHAT_EAT_ALL;
	}

	std::string chan = hexchat_get_info(ph, "channel");
	std::string network = hexchat_get_info(ph, "network");

	std::string key = keymgm->keystoreGetKey(network, chan, mode);

	/* Check if we can encrypt */
	if (key.empty()) {
		hexchat_printf(ph, "/settopic+ error, no key found for %s", chan);
		return HEXCHAT_EAT_ALL;
	}

	std::string encryptedcontext;
	if (mode == FISH_CBC_MODE) {
		blowfish_encrypt_cbc(topic, encryptedcontext, key);
	}
	else {
		blowfish_encrypt(topic, encryptedcontext, key);
	}

	if (encryptedcontext.empty()) {
		return HEXCHAT_EAT_ALL;
	}

	std::string command = "TOPIC " + chan + " +OK ";
	if (mode == FISH_CBC_MODE)
		command += "*";

	command += encryptedcontext;

	hexchat_commandf(ph, "%s", command.c_str());

	return HEXCHAT_EAT_ALL;
}

int handle_keyx(char* word[], char* word_eol[], void* userdata) {
	const char* target = word[2];
	hexchat_context* query_ctx = nullptr;
	std::string privkey;
	std::string pubkey;
	int ctx_type = 0;

	if (*target)
		query_ctx = find_context_on_network(target);
	else {
		target = hexchat_get_info(ph, "channel");
		query_ctx = hexchat_get_context(ph);
	}

	if (query_ctx) {
		hexchat_set_context(ph, query_ctx);
		ctx_type = hexchat_list_int(ph, nullptr, "type");
	}

	if ((query_ctx && ctx_type != 3) || (!query_ctx && !irc_is_query(target))) {
		hexchat_print(ph, "FiSH: You can only exchange keys with individuals");
		return HEXCHAT_EAT_ALL; // 
	}

	if (DH1080_Generate(privkey, pubkey)) {
		pending_exchanges[std::string(target)] = privkey;
		hexchat_commandf(ph, "quote NOTICE %s :DH1080_INIT %s CBC", target, pubkey.c_str());
		hexchat_printf(ph, "Sent public key to %s (CBC), waiting for reply...", target);
	}
	else {
		hexchat_print(ph, "Failed to generate keys");
	}

	return HEXCHAT_EAT_ALL; // 
}

// Main entry point for the HexChat plugin
void hexchat_plugin_get_info(const char** name, const char** desc,
	const char** version, void** reserved) {
	*name = plugin_name;
	*desc = plugin_desc;
	*version = plugin_version;
}

// Plugin initialization function
int hexchat_plugin_init(hexchat_plugin * plugin_handle,
	const char** name,
	const char** desc,
	const char** version,
	char* arg) {
	ph = plugin_handle;

	*name = plugin_name;
	*desc = plugin_desc;
	*version = plugin_version;

	keymgm = new KeyManagement(get_config_filename());

	hexchat_hook_command(ph, "DELKEY", HEXCHAT_PRI_NORM, handle_delkey, usage_delkey, NULL);
	hexchat_hook_command(ph, "KEY", HEXCHAT_PRI_NORM, handle_key, usage_key, NULL);
	hexchat_hook_command(ph, "KEYX", HEXCHAT_PRI_NORM, handle_keyx, usage_keyx, NULL);
	hexchat_hook_command(ph, "SETKEY", HEXCHAT_PRI_NORM, handle_setkey, usage_setkey, NULL);
	hexchat_hook_command(ph, "", HEXCHAT_PRI_NORM, handle_outgoing, NULL, NULL);
	hexchat_hook_command(ph, "SETTOPIC+", HEXCHAT_PRI_NORM, handle_crypt_topic, usage_topic, NULL);
	hexchat_hook_command(ph, "MARKER", HEXCHAT_PRI_NORM, handle_marker, usage_marker, NULL);
	hexchat_hook_server(ph, "NOTICE", HEXCHAT_PRI_HIGHEST, handle_keyx_notice, NULL);
	hexchat_hook_server_attrs(ph, "PRIVMSG", HEXCHAT_PRI_NORM, handle_incoming, NULL);
	hexchat_hook_server_attrs(ph, "TOPIC", HEXCHAT_PRI_NORM, handle_incoming, NULL);
	hexchat_hook_server_attrs(ph, "332", HEXCHAT_PRI_NORM, handle_incoming, NULL);
	hexchat_printf(ph, "%s plugin loaded\n", plugin_name);
	return 1;
}

// Plugin deinitialization function
int hexchat_plugin_deinit(void) {
	// Remaining cleanup code remains the same...
	delete keymgm;
	hexchat_printf(ph, "%s plugin unloaded\n", plugin_name);
	return 1;
}
