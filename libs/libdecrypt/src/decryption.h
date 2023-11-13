#ifndef DECRYPTION_H
#define DECRYPTION_H

#include <openssl/rsa.h>
#include <openssl/pem.h>
#include <openssl/err.h>
#include <openssl/bio.h>
#include <openssl/evp.h>
#include <openssl/bn.h>
#include <openssl/rand.h>
#include <iostream>
#include <sstream>
#include <vector>
#include <fstream>
#include "pugixml.hpp"
#include "obfuscate.h"

/**
 * @brief The Decryption class for decrypting encrypted files.
 */
class Decryption
{
public:
    /**
     * @brief Constructs a Decryption object with the specified encrypted file.
     * 
     * @param encryptedFile The path to the encrypted file.
     */
    Decryption(const std::string &encryptedFile);

    /**
     * @brief Decrypts the contents of the encrypted file.
     * 
     * This function performs the decryption process, including loading the private key,
     * retrieving the encrypted data and session key from the XML file, decrypting the
     * session key, and decrypting the XML string.
     */
    void decryptFile();

    /**
     * @brief Retrieves the decrypted content.
     * 
     * @return The decrypted content as a string.
     */
    std::string getDecryptedContent() const;

private:
    std::string encryptedFile_;    /**< The path to the encrypted file. */
    std::string decryptedContent_; /**< The decrypted content of the file. */

    /**
     * @brief Decrypts the given ciphertext using the provided RSA key.
     * 
     * @param ciphertext The ciphertext to decrypt.
     * @param key The RSA key for decryption.
     * @return The decrypted plaintext.
     */
    static std::string decrypt(const std::string &ciphertext, RSA *key);

    /**
     * @brief Decodes the given base64-encoded string.
     * 
     * @param input The base64-encoded input string.
     * @return The decoded output string.
     */
    static std::string base64_decode(const std::string &input);

    /**
     * @brief Loads the private key from the given PEM string.
     * 
     * @param privateKeyString The PEM string representing the private key.
     * @return The loaded RSA private key.
     */
    static RSA *loadPrivateKey(const std::string &privateKeyString);

    /**
     * @brief Decrypts the given encrypted session key using the provided RSA key.
     * 
     * @param encryptedSessionKey The encrypted session key.
     * @param key The RSA key for decryption.
     * @return The decrypted session key.
     */
    static std::string decryptSessionKey(const std::string &encryptedSessionKey, RSA *key);
};

#endif // DECRYPTION_H
