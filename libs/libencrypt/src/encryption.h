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
#include <string>
#include <openssl/ecdh.h>
#include <openssl/buffer.h>
#include <iomanip>
//#include "config.h"
/**
 * @class Encryption
 * @brief Provides encryption and decryption functionality using RSA encryption algorithm.
 */
class Encryption {
public:
    /**
     * @brief Generates random session key
     * 
     * @param sessionKey A pointer to session key
     * @param keySize Session key length (128 bits for AES-128)
     */
    static void generateSessionKey(unsigned char* sessionKey, size_t keySize);

    /**
     * @brief Loads a public key from a file.
     *
     * @param filename The name of the file containing the public key.
     * @return EVP_PKEY* A pointer to the loaded public key.
     *         Returns nullptr if the key file cannot be opened or there is an error reading the key.
     */
    static EVP_PKEY* loadPublicKey(const std::string& filename);

    /**
     * @brief 
     * 
     * @param sessionKey Key to be encrypted
     * @param keySize Session key length
     * @param publicKey The public key used for encryption
     * @return std::string Encrypted session key
     */
    static std::string encryptSessionKey(const unsigned char* sessionKey, size_t keySize, EVP_PKEY* publicKey);

    /**
     * @brief 
     * 
     * @param buffer pointer to data to be encoded
     * @param length data length
     * @return std::string encoded data
     */
    static std::string base64Encode(const unsigned char* buffer, size_t length);

    /**
     * @brief 
     * 
     * @param plaintext The plaintext to be encrypted.
     * @param sessionKey The session key used for encryption.
     * @param iv Initialization vector.
     * @return std::string The encrypted ciphertext.
     *         Returns an empty string if there is an error during encryption.
     */
    static std::string encryptData(const std::string& plaintext, const unsigned char* sessionKey, const unsigned char* iv);

    /**
     * @brief Encrypts a file using the provided public key.
     *
     * @param filePath The path to the file to be encrypted.
     * @param publicKeyFile The path to the file containing the public key.
     * @return bool True if the file encryption is successful, false otherwise.
     */
    static bool encryptFile(const std::string& publicKeyFile, std::string& filePath);
};

#endif // ENCRYPTION_H
