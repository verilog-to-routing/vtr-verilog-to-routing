#include "encryption.h"

#ifdef SESSION_KEY_SIZE
unsigned sessionKeySize = SESSION_KEY_SIZE;
#else
unsigned sessionKeySize = 16;
#endif

/**
 * @brief Generates random session key
 * 
 * @param sessionKey A pointer to session key
 * @param keySize Session key length (128 bits for AES-128)
 */
void Encryption::generateSessionKey(unsigned char* sessionKey, size_t keySize) {
    if (RAND_bytes(sessionKey, keySize) != 1) {
        std::cerr << "Error generating session key." << std::endl;
        exit(1);
    }
}

/**
 * @brief Loads a public key from a file.
 *
 * @param filename The name of the file containing the public key.
 * @return EVP_PKEY* A pointer to the loaded public key.
 *         Returns nullptr if the key file cannot be opened or there is an error reading the key.
 */
EVP_PKEY* Encryption::loadPublicKey(const std::string& filename) {
    EVP_PKEY *key = nullptr;
    FILE* keyFile = fopen(filename.c_str(), "r");
    if (!keyFile) {
        std::cerr << "Unable to open key file: " << filename << std::endl;
        return nullptr;
    }

    // Create a BIO object from the file
    BIO* keyBio = BIO_new_fp(keyFile, BIO_NOCLOSE);
        if (!keyBio) {
        std::cerr << "Error creating BIO from file" << std::endl;
        fclose(keyFile);
        return nullptr;
    }

    // Use PEM_read_bio_PUBKEY 
    key = PEM_read_bio_PUBKEY(keyBio, nullptr, nullptr, nullptr);

    BIO_free(keyBio);
    fclose(keyFile);
    if (!key) {
        std::cerr << "Error reading public key from file: " << filename << std::endl;
        return nullptr;
    }
    return key;
}

/**
 * @brief 
 * 
 * @param sessionKey Key to be encrypted
 * @param keySize Session key length
 * @param publicKey The public key used for encryption
 * @return std::string Encrypted session key
 */
std::string Encryption::encryptSessionKey(const unsigned char* sessionKey, size_t keySize, EVP_PKEY* publicKey) {
    EVP_PKEY_CTX* ctx = EVP_PKEY_CTX_new(publicKey, NULL);
    if (!ctx) {
        std::cerr << "Failed to create EVP_PKEY_CTX" << std::endl;
        return "";
    }

    if (EVP_PKEY_encrypt_init(ctx) <= 0) {
        std::cerr << "EVP_PKEY_encrypt_init failed" << std::endl;
        EVP_PKEY_CTX_free(ctx);
        return "";
    }

    if (EVP_PKEY_CTX_set_rsa_padding(ctx, RSA_PKCS1_OAEP_PADDING) <= 0) {
        std::cerr << "EVP_PKEY_CTX_set_rsa_padding failed" << std::endl;
        EVP_PKEY_CTX_free(ctx);
        return "";
    }

    size_t outLen;
    if (EVP_PKEY_encrypt(ctx, NULL, &outLen, sessionKey, keySize) <= 0) {
        std::cerr << "EVP_PKEY_encrypt (determine length) failed" << std::endl;
        EVP_PKEY_CTX_free(ctx);
        return "";
    }

    std::vector<unsigned char> out(outLen);
    if (EVP_PKEY_encrypt(ctx, out.data(), &outLen, sessionKey, keySize) <= 0) {
        std::cerr << "EVP_PKEY_encrypt failed" << std::endl;
        EVP_PKEY_CTX_free(ctx);
        return "";
    }

    EVP_PKEY_CTX_free(ctx);

    std::string base64EncryptedSessionKey = base64Encode(out.data(), out.size());
    return base64EncryptedSessionKey;
}

/**
 * @brief 
 * 
 * @param buffer pointer to data to be encoded
 * @param length data length
 * @return std::string encoded data
 */
std::string Encryption::base64Encode(const unsigned char* buffer, size_t length) {
    BIO* bio = BIO_new(BIO_s_mem());
    BIO* b64 = BIO_new(BIO_f_base64());
    BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
    BIO_push(b64, bio);
    BIO_write(b64, buffer, length);
    BIO_flush(b64);

    BUF_MEM* bufferPtr;
    BIO_get_mem_ptr(b64, &bufferPtr);

    std::string encoded(bufferPtr->data, bufferPtr->length);
    BIO_free_all(b64);
    return encoded;
}

/**
 * @brief 
 * 
 * @param plaintext The plaintext to be encrypted.
 * @param sessionKey The session key used for encryption.
 * @param iv Initialization vector.
 * @return std::string The encrypted ciphertext.
 *         Returns an empty string if there is an error during encryption.
 */
std::string Encryption::encryptData(const std::string& plaintext, const unsigned char* sessionKey, const unsigned char* iv) {
    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) {
        std::cerr << "Failed to create EVP_CIPHER_CTX" << std::endl;
        return "";
    }

    // Initialize the encryption operation with AES-128-CBC
    if (EVP_EncryptInit_ex(ctx, EVP_aes_128_cbc(), NULL, sessionKey, iv) != 1) {
        std::cerr << "EVP_EncryptInit_ex failed" << std::endl;
        EVP_CIPHER_CTX_free(ctx);
        return "";
    }

    std::vector<unsigned char> ciphertext(plaintext.size() + EVP_CIPHER_block_size(EVP_aes_128_cbc()));
    int len = 0;
    int ciphertextLen = 0;

    // Encrypt the data in chunks
    if (EVP_EncryptUpdate(ctx, ciphertext.data(), &len, reinterpret_cast<const unsigned char*>(plaintext.data()), plaintext.size()) != 1) {
        std::cerr << "EVP_EncryptUpdate failed" << std::endl;
        EVP_CIPHER_CTX_free(ctx);
        return "";
    }
    ciphertextLen += len;

    // Finalize the encryption
    if (EVP_EncryptFinal_ex(ctx, ciphertext.data() + ciphertextLen, &len) != 1) {
        std::cerr << "EVP_EncryptFinal_ex failed" << std::endl;
        EVP_CIPHER_CTX_free(ctx);
        return "";
    }
    ciphertextLen += len;
    ciphertext.resize(ciphertextLen);

    EVP_CIPHER_CTX_free(ctx);

    // Combine IV and ciphertext into a single string
    std::vector<unsigned char> combinedData(iv, iv + EVP_MAX_IV_LENGTH);
    combinedData.insert(combinedData.end(), ciphertext.begin(), ciphertext.end());

    // Base64 encode the combined data
    return base64Encode(combinedData.data(), combinedData.size());
}

/**
 * @brief Encrypts a file using the provided public key.
 *
 * @param filePath The path to the file to be encrypted.
 * @param publicKeyFile The path to the file containing the public key.
 * @return bool True if the file encryption is successful, false otherwise.
 */
bool Encryption::encryptFile(const std::string& publicKeyFile, std::string& filePath ) {

    OpenSSL_add_all_algorithms();
    ERR_load_crypto_strings();
    unsigned char sessionKey[sessionKeySize];
    generateSessionKey(sessionKey, sizeof(sessionKey));

    //load public key
    EVP_PKEY* publicKey = loadPublicKey(publicKeyFile);
    if (!publicKey) {
       std::cout << "Unable to open publicKey: " << filePath << std::endl;
        return false;
    }

    // Read file contents
    std::ifstream file(filePath, std::ios::binary);
    if (!file) {
        std::cerr << "Unable to open file: " << filePath << std::endl;
        EVP_PKEY_free(publicKey);
        return false;
    }

    std::string plaintext((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    file.close();

    // Encrypt session key
    std::string encryptedSessionKey = encryptSessionKey(sessionKey, sizeof(sessionKey), publicKey);
    if (encryptedSessionKey.empty()) {
        EVP_PKEY_free(publicKey);
        return false;
    }

    unsigned char iv[EVP_MAX_IV_LENGTH];
    if (RAND_bytes(iv, sizeof(iv)) != 1) {
        std::cerr << "Error generating IV." << std::endl;
        return false;
    }

    // Encrypt file contents
    std::string encrypted = encryptData(plaintext, sessionKey, iv);

    // Create an XML document for the encrypted data and session key
    pugi::xml_document encryptedDoc;
    auto root = encryptedDoc.append_child("EncryptedData");
    auto sessionKeyNode = root.append_child("SessionKey");
    sessionKeyNode.append_child(pugi::node_pcdata).set_value(encryptedSessionKey.c_str());
    auto dataNode = root.append_child("Data");
    dataNode.append_child(pugi::node_pcdata).set_value(encrypted.c_str());

    
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
