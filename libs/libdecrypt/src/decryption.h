#ifndef DECRYPTION_H
#define DECRYPTION_H

#include <openssl/rsa.h>
#include <openssl/pem.h>
#include <openssl/err.h>
#include <openssl/bio.h>
#include <openssl/evp.h>
#include <openssl/bn.h>
#include <openssl/rand.h>
#include <openssl/evp.h>
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
     * @brief 
     * 
     * @param encryptedData The encrypted data to decrypt.
     * @param sessionKey The session key for data decryption.
     * @return std::string The decrypted plaintext.
     */
    static std::string decryptData(const std::string& encryptedData, const unsigned char* sessionKey);

    /**
     * @brief 
     * 
     * @param encoded he base64-encoded input string.
     * @return std::vector<unsigned char> The decoded dynamic array of characters.
     */
    static std::vector<unsigned char> base64Decode(const std::string& encoded);

    /**
     * @brief Loads the private key from the given PEM string.
     * 
     * @param privateKeyString The PEM string representing the private key.
     * @return The loaded EVP private key.
     */
    static EVP_PKEY* loadPrivateKey(const std::string& privateKeyString);

    /**
     * @brief Decrypts the given encrypted session key using the provided EVP key.
     * 
     * @param encryptedSessionKey The encrypted session key.
     * @param privateKey The EVP key for decryption.
     * @return The decrypted session key.
     */
    static std::string decryptSessionKey(const std::string& encryptedSessionKey, EVP_PKEY* privateKey);
};

#endif // DECRYPTION_H
