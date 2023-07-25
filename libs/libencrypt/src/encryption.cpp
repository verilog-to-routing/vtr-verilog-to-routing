#include "encryption.h"
#ifdef PASSPHRASE
std::string passphrase_enc = PASSPHRASE; 
#else
std::string passphrase_enc = ""; // Set your PEM pass phrase here
#endif
/**
 * @brief Loads a public key from a file.
 *
 * @param filename The name of the file containing the public key.
 * @return RSA* A pointer to the loaded public key.
 *         Returns nullptr if the key file cannot be opened or there is an error reading the key.
 */
RSA* Encryption::loadPublicKey(const std::string& filename) {

    RSA* key = nullptr;
    FILE* keyFile = fopen(filename.c_str(), "r");
    if (!keyFile) {
        std::cerr << "Unable to open key file: " << filename << std::endl;
        return nullptr;
    }

    if (!PEM_read_RSA_PUBKEY(keyFile, &key, NULL, (void*)passphrase_enc.c_str())) {
        std::cerr << "Error reading public key from file: " << filename << std::endl;
        fclose(keyFile);
        return nullptr;
    }

    fclose(keyFile);
    return key;
}

/**
 * @brief Encrypts a session key using the provided public key.
 *
 * @param key The public key used for encryption.
 * @return std::string The encrypted session key.
 *         Returns an empty string if there is an error generating or encrypting the session key.
 */
std::string Encryption::encryptSessionKey(RSA* key) {
    unsigned char sessionKey[128];
    if (!RAND_bytes(sessionKey, sizeof(sessionKey))) {
        std::cerr << "Error generating session key" << std::endl;
        return "";
    }

    std::vector<unsigned char> encryptedSessionKey(RSA_size(key));
    if (RSA_public_encrypt(sizeof(sessionKey), sessionKey, encryptedSessionKey.data(), key, RSA_PKCS1_OAEP_PADDING) == -1) {
        std::cerr << "Session key encryption error: " << ERR_error_string(ERR_get_error(), nullptr) << std::endl;
        return "";
    }

    return std::string(reinterpret_cast<char*>(encryptedSessionKey.data()), encryptedSessionKey.size());
}

/**
 * @brief Encrypts the given plaintext using the provided public key.
 *
 * @param plaintext The plaintext to be encrypted.
 * @param key The public key used for encryption.
 * @return std::string The encrypted ciphertext.
 *         Returns an empty string if there is an error during encryption.
 */
std::string Encryption::encrypt(const std::string& plaintext, RSA* key) {
    int rsaLen = RSA_size(key);
    int blockLen = rsaLen - 42; // PKCS1_OAEP_PADDING reduces max data length
    int len = plaintext.size();
    std::string ciphertext;

    for (int i = 0; i < len; i += blockLen) {
        std::vector<unsigned char> buffer(rsaLen);
        std::string substr = plaintext.substr(i, blockLen);

        if (RSA_public_encrypt(substr.size(), reinterpret_cast<const unsigned char*>(substr.data()), buffer.data(), key, RSA_PKCS1_OAEP_PADDING) == -1) {
            std::cerr << "Encryption error: " << ERR_error_string(ERR_get_error(), nullptr) << std::endl;
            return "";
        }

        ciphertext.append(reinterpret_cast<char*>(buffer.data()), rsaLen);
    }

    return ciphertext;
}

/**
 * @brief Base64 encodes the given input string.
 *
 * @param input The input string to be encoded.
 * @return std::string The base64-encoded string.
 */
std::string Encryption::base64_encode(const std::string& input) {
    BIO* b64 = BIO_new(BIO_f_base64());
    BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);

    BIO* bmem = BIO_new(BIO_s_mem());
    b64 = BIO_push(b64, bmem);

    BIO_write(b64, input.data(), input.size());
    BIO_flush(b64);

    BUF_MEM* bptr;
    BIO_get_mem_ptr(b64, &bptr);

    std::string output(bptr->data, bptr->length);
    BIO_free_all(b64);

    return output;
}

/**
 * @brief Encrypts a file using the provided public key.
 *
 * @param filePath The path to the file to be encrypted.
 * @param publicKeyFile The path to the file containing the public key.
 * @return bool True if the file encryption is successful, false otherwise.
 */
bool Encryption::encryptFile(const std::string& publicKeyFile, std::string& filePath ) {

    // Load public key
    RSA* publicKey = loadPublicKey(publicKeyFile);
    if (!publicKey) {
        return false;
    }

    // Read file contents
    std::ifstream file(filePath, std::ios::binary);
    if (!file) {
        std::cerr << "Unable to open file: " << filePath << std::endl;
        RSA_free(publicKey);
        return false;
    }

    std::string plaintext((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    file.close();

    // Encrypt session key
    std::string encryptedSessionKey = encryptSessionKey(publicKey);
    if (encryptedSessionKey.empty()) {
        RSA_free(publicKey);
        return false;
    }

    // Base64 encode session key
    std::string base64EncryptedSessionKey = base64_encode(encryptedSessionKey);

    // Encrypt file contents
    std::string encrypted = encrypt(plaintext, publicKey);
    RSA_free(publicKey);

    // Base64 encode encrypted data
    std::string base64Encrypted = base64_encode(encrypted);

    // Create an XML document for the encrypted data and session key
    pugi::xml_document encryptedDoc;
    auto root = encryptedDoc.append_child("EncryptedData");
    auto sessionKeyNode = root.append_child("SessionKey");
    sessionKeyNode.append_child(pugi::node_pcdata).set_value(base64EncryptedSessionKey.c_str());
    auto dataNode = root.append_child("Data");
    dataNode.append_child(pugi::node_pcdata).set_value(base64Encrypted.c_str());

    
    size_t lastDotPos = filePath.find_last_of('.');

    if (lastDotPos != std::string::npos && filePath.substr(lastDotPos) == ".xml") {
        // Remove the .xml extension
        filePath.erase(lastDotPos);
        // Append the .xmle extension
        filePath.append(".xmle");
        }
    // Save the encrypted data to a new file
    std::string encryptedFilePath = filePath;
    encryptedDoc.save_file(encryptedFilePath.c_str(), "  ");

    std::cout << "File encrypted successfully. Encrypted file saved as: " << encryptedFilePath << std::endl;

    return true;
}
