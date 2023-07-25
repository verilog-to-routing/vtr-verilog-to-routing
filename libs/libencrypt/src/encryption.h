#ifndef ENCRYPTION_H
#define ENCRYPTION_H

#include <openssl/rsa.h>
#include <openssl/pem.h>
#include <openssl/err.h>
#include <openssl/bio.h>
#include <openssl/evp.h>
#include <openssl/bn.h>
#include "pugixml.hpp"
#include <iostream>
#include <sstream>
#include <vector>
#include <fstream>
#include <openssl/rand.h>
#include "obfuscate.h"
//#include "config.h"
/**
 * @class Encryption
 * @brief Provides encryption and decryption functionality using RSA encryption algorithm.
 */
class Encryption {
public:
    /**
     * @brief Loads a public key from a file.
     *
     * @param filename The name of the file containing the public key.
     * @return RSA* A pointer to the loaded public key.
     *         Returns nullptr if the key file cannot be opened or there is an error reading the key.
     */
    static RSA *loadPublicKey(const std::string &filename);

    /**
     * @brief Encrypts a session key using the provided public key.
     *
     * @param key The public key used for encryption.
     * @return std::string The encrypted session key.
     *         Returns an empty string if there is an error generating or encrypting the session key.
     */
    static std::string encryptSessionKey(RSA *key);

    /**
     * @brief Encrypts the given plaintext using the provided public key.
     *
     * @param plaintext The plaintext to be encrypted.
     * @param key The public key used for encryption.
     * @return std::string The encrypted ciphertext.
     *         Returns an empty string if there is an error during encryption.
     */
    static std::string encrypt(const std::string &plaintext, RSA *key);

    /**
     * @brief Base64 encodes the given input string.
     *
     * @param input The input string to be encoded.
     * @return std::string The base64-encoded string.
     */
    static std::string base64_encode(const std::string &input);

    /**
     * @brief Encrypts a file using the provided public key.
     *
     * @param filePath The path to the file to be encrypted.
     * @param publicKeyFile The path to the file containing the public key.
     * @return bool True if the file encryption is successful, false otherwise.
     */
    static bool encryptFile(const std::string& publicKeyFile, std::string &filePath);
};

#endif // ENCRYPTION_H
