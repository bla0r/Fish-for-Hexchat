# FiSH HexChat Encryption Plugin

## Introduction
This is the FiSH encryption plugin for HexChat, providing encryption support for the FiSH protocol. FiSH is a simple encryption protocol used in IRC for secure communication. This plugin allows users to encrypt and decrypt messages, set encryption keys, and perform Diffie-Hellman key exchanges.

## Features
- Message Encryption: Encrypt outgoing messages using the FiSH encryption protocol.
- Message Decryption: Decrypt incoming encrypted messages and display the decrypted content.
- Key Management: Set, delete, and view encryption keys for channels or individual users.
- Diffie-Hellman Key Exchange: Perform key exchanges to establish secure communication channels.

## Dependencies
- **gio**

- **openssl**

- **hexchat**

- **meson**

### Windows - Visual Studio

1. Clone or download the repository.
2. Open the project in Visual Studio.
3. Build the solution.
4. Copy the compiled DLL file (e.g., `Blowfish.dll`) to the HexChat plugins folder (usually located in the HexChat installation directory).

## Meson User Install

For local installation, follow these steps:

```bash
meson builddir -Dlocal_install=true
ninja -C builddir test
ninja -C builddir install
```
## Meson System Install

For system-wide installation, use the following commands:

```bash
meson builddir
ninja -C builddir
ninja -C builddir test
sudo ninja -C builddir install
```

These commands assume you have Meson and Ninja installed on your system. Adjust the options according to your project's requirements.

## Commands
- `/SETKEY [<nick or #channel>] [<mode>:]<password>`: Set the encryption key for a channel or user. Modes: ECB, CBC.
- `/DELKEY [<nick or #channel>]`: Delete the encryption key for a channel or user.
- `/KEY [<nick or #channel>]`: View the encryption key for a channel or user.
- `/KEYX [<nick>]`: Initiate a Diffie-Hellman key exchange with the specified user.
- `/SETTOPIC+ <topic>`: Set an encrypted topic for the current channel.
- `/MARKER [<append|prepend|disabled>]`: Set the message marker mode for encrypted messages.

## Configuration
- Configuration File: The plugin uses a configuration file named `blowfish.conf` to store encryption keys. It is located in the HexChat config directory.
- You can set a password in keymanagement.h or in hexchat_plugin.cpp

## Usage
1. Load the plugin using the command `/load blowfish`.
2. Set encryption keys using `/SETKEY`.
3. Communicate securely with encrypted messages.
4. Perform Diffie-Hellman key exchanges with `/KEYX`.
5. Manage encryption keys with `/KEY` and `/DELKEY`.

## Version
- Version: 1.0.1

## Disclaimer
This plugin is provided as-is and does not guarantee absolute security. Use it responsibly and be aware of its limitations.

## Credits
- Plugin developed by bla0r
- This plugin based on [flakes mirc_fish_10](https://github.com/flakes/mirc_fish_10) and on [BakasuraRCE FiSHLiM Plugin](https://github.com/BakasuraRCE/hexchat-fishlim-reloaded).

Feel free to contribute to the development or report issues on the [GitHub repository](https://github.com/your/repository).
