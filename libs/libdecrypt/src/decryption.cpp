#include "decryption.h"

#ifdef PASS_PHRASE
std::string passphrase = PASS_PHRASE; 
#else
std::string passphrase = ""; // Set your PEM pass phrase here
#endif

/**
 * @brief Constructs a Decryption object with the specified encrypted file.
 * 
 * @param encryptedFile The path to the encrypted file.
 */
Decryption::Decryption(const std::string& encryptedFile)
    : encryptedFile_(encryptedFile) {
    decryptFile();
}

/**
 * @brief Decrypts the contents of the encrypted file.
 * 
 * This function performs the decryption process, including loading the private key,
 * retrieving the encrypted data and session key from the XML file, decrypting the
 * session key, and decrypting the XML string.
 */
void Decryption::decryptFile() {
#ifdef PRIVATE_KEY
    const char* privateKeyString = AY_OBFUSCATE(PRIVATE_KEY);
#else
    const char* privateKeyString = AY_OBFUSCATE(
        "-----BEGIN RSA PRIVATE KEY-----\n"
        "dummykey\n"
        "-----END RSA PRIVATE KEY-----\n"); // Replace with your private key string
#endif
    EVP_PKEY *privateKey = loadPrivateKey(privateKeyString);
    if (!privateKey) {
        return;
    }

    // Load encrypted data and session key from XML file
    pugi::xml_document encryptedDocLoaded;
    pugi::xml_parse_result result = encryptedDocLoaded.load_file(encryptedFile_.c_str());
    if (!result) {
        std::cerr << "XML parse error: " << result.description() << std::endl;
        EVP_PKEY_free(privateKey);
        return;
    }

    pugi::xml_node root = encryptedDocLoaded.child("EncryptedData");
    std::string base64EncryptedSessionKeyLoaded = root.child_value("SessionKey");
    std::string base64EncryptedLoaded = root.child_value("Data");

    // Decrypt session key
    std::string decryptedSessionKey = decryptSessionKey(base64EncryptedSessionKeyLoaded, privateKey);

    // Decrypt XML string
    std::string decrypted = decryptData(base64EncryptedLoaded, reinterpret_cast<const unsigned char*>(decryptedSessionKey.c_str()));

    // Write the decrypted data to a file
    std::ofstream decryptedFile("decrypted.xml");
    decryptedFile << decrypted;
    decryptedFile.close();
    
    decryptedContent_ = decrypted;
    EVP_PKEY_free(privateKey);
}

/**
 * @brief Retrieves the decrypted content.
 * 
 * @return The decrypted content as a string.
 */
std::string Decryption::getDecryptedContent() const {
    return decryptedContent_;
}

/**
 * @brief Loads the private key from the given PEM string.
 * 
 * @param privateKeyString The PEM string representing the private key.
 * @return The loaded EVP private key.
 */
EVP_PKEY *Decryption::loadPrivateKey(const std::string& privateKeyString) {
    EVP_PKEY *pkey = nullptr;
    BIO* privateKeyBio = BIO_new_mem_buf(privateKeyString.data(), privateKeyString.size());

    if (!privateKeyBio) {
        std::cerr << "Error creating BIO for private key" << std::endl;
        return nullptr;
    }

    char* passphrase_cstr = const_cast<char*>(passphrase.c_str());
    if (!PEM_read_bio_PrivateKey(privateKeyBio, &pkey, NULL, passphrase_cstr)) {
        std::cerr << "Error reading private key" << std::endl;
        BIO_free(privateKeyBio);
        return nullptr;
    }

    BIO_free(privateKeyBio);
    return pkey;
}

/**
 * @brief 
 * 
 * @param encoded he base64-encoded input string.
 * @return std::vector<unsigned char> The decoded dynamic array of characters.
 */
std::vector<unsigned char> Decryption::base64Decode(const std::string& encoded) {
    BIO* bio = BIO_new_mem_buf(encoded.data(), -1);
    BIO* b64 = BIO_new(BIO_f_base64());
    BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
    bio = BIO_push(b64, bio);

    std::vector<unsigned char> decoded(encoded.length());
    int decodedLen = BIO_read(bio, decoded.data(), encoded.length());
    decoded.resize(decodedLen);

    BIO_free_all(bio);
    return decoded;
}

/**
 * @brief Decrypts the given encrypted session key using the provided EVP key.
 * 
 * @param encryptedSessionKey The encrypted session key.
 * @param privateKey The EVP key for decryption.
 * @return The decrypted session key.
 */
std::string Decryption::decryptSessionKey(const std::string& encryptedSessionKey, EVP_PKEY* privateKey) {
    std::vector<unsigned char> decodedKey = base64Decode(encryptedSessionKey);

    EVP_PKEY_CTX* ctx = EVP_PKEY_CTX_new(privateKey, NULL);
    if (!ctx) {
        std::cerr << "Failed to create EVP_PKEY_CTX" << std::endl;
        return "";
    }

    if (EVP_PKEY_decrypt_init(ctx) <= 0) {
        std::cerr << "EVP_PKEY_decrypt_init failed" << std::endl;
        EVP_PKEY_CTX_free(ctx);
        return "";
    }

    if (EVP_PKEY_CTX_set_rsa_padding(ctx, RSA_PKCS1_OAEP_PADDING) <= 0) {
        std::cerr << "EVP_PKEY_CTX_set_rsa_padding failed" << std::endl;
        EVP_PKEY_CTX_free(ctx);
        return "";
    }

    size_t outLen;
    if (EVP_PKEY_decrypt(ctx, NULL, &outLen, decodedKey.data(), decodedKey.size()) <= 0) {
        std::cerr << "EVP_PKEY_decrypt (determine length) failed" << std::endl;
        EVP_PKEY_CTX_free(ctx);
        return "";
    }

    std::vector<unsigned char> out(outLen);
    if (EVP_PKEY_decrypt(ctx, out.data(), &outLen, decodedKey.data(), decodedKey.size()) <= 0) {
        std::cerr << "EVP_PKEY_decrypt failed" << std::endl;
        EVP_PKEY_CTX_free(ctx);
        return "";
    }

    EVP_PKEY_CTX_free(ctx);

    return std::string(out.begin(), out.begin() + outLen);
}

/**
 * @brief 
 * 
 * @param encryptedData The encrypted data to decrypt.
 * @param sessionKey The session key for data decryption.
 * @return std::string The decrypted plaintext.
 */
std::string Decryption::decryptData(const std::string& encryptedData, const unsigned char* sessionKey) {
    std::vector<unsigned char> decodedData = base64Decode(encryptedData);

    // Extract the IV from the decoded data
    unsigned char iv[EVP_MAX_IV_LENGTH];
    int iv_len = EVP_CIPHER_iv_length(EVP_aes_128_cbc());
    std::copy(decodedData.begin(), decodedData.begin() + iv_len, iv);
    const unsigned char* ciphertext = decodedData.data() + iv_len;
    size_t ciphertextLen = decodedData.size() - iv_len;

    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) {
        std::cerr << "Failed to create EVP_CIPHER_CTX" << std::endl;
        return "";
    }

    if (EVP_DecryptInit_ex(ctx, EVP_aes_128_cbc(), NULL, sessionKey, iv) != 1) {
        std::cerr << "EVP_DecryptInit_ex failed" << std::endl;
        EVP_CIPHER_CTX_free(ctx);
        return "";
    }

    std::vector<unsigned char> plaintext(ciphertextLen + EVP_CIPHER_block_size(EVP_aes_128_cbc()));
    int len = 0;
    int plaintextLen = 0;

    if (EVP_DecryptUpdate(ctx, plaintext.data(), &len, ciphertext, ciphertextLen) != 1) {
        std::cerr << "EVP_DecryptUpdate failed" << std::endl;
        EVP_CIPHER_CTX_free(ctx);
        return "";
    }
    plaintextLen += len;

    if (EVP_DecryptFinal_ex(ctx, plaintext.data() + plaintextLen, &len) != 1) {
        std::cerr << "EVP_DecryptFinal_ex failed" << std::endl;
        EVP_CIPHER_CTX_free(ctx);
        return "";
    }
    plaintextLen += len;
    plaintext.resize(plaintextLen);

    EVP_CIPHER_CTX_free(ctx);

    return std::string(plaintext.begin(), plaintext.end());
}