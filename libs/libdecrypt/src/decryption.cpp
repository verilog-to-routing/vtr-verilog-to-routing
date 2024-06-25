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
    RSA* privateKey = loadPrivateKey(privateKeyString);
    if (!privateKey) {
        return;
    }

    // Load encrypted data and session key from XML file
    pugi::xml_document encryptedDocLoaded;
    pugi::xml_parse_result result = encryptedDocLoaded.load_file(encryptedFile_.c_str());
    if (!result) {
        std::cerr << "XML parse error: " << result.description() << std::endl;
        RSA_free(privateKey);
        return;
    }

    pugi::xml_node root = encryptedDocLoaded.child("EncryptedData");
    std::string base64EncryptedSessionKeyLoaded = root.child_value("SessionKey");
    std::string base64EncryptedLoaded = root.child_value("Data");

    // Base64 decode encrypted session key and data
    std::string encryptedSessionKeyLoaded = base64_decode(base64EncryptedSessionKeyLoaded);
    std::string encryptedLoaded = base64_decode(base64EncryptedLoaded);

    // Decrypt session key
    std::string decryptedSessionKey = decryptSessionKey(encryptedSessionKeyLoaded, privateKey);

    // Decrypt XML string
    std::string decrypted = decrypt(encryptedLoaded, privateKey);

    // Write the decrypted data to a file
    // std::ofstream decryptedFile("decrypted.xml");
    // decryptedFile << decrypted;
    // decryptedFile.close();
    
    decryptedContent_ = decrypted;
    RSA_free(privateKey);
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
 * @brief Decrypts the given ciphertext using the provided RSA key.
 * 
 * @param ciphertext The ciphertext to decrypt.
 * @param key The RSA key for decryption.
 * @return The decrypted plaintext.
 */
std::string Decryption::decrypt(const std::string& ciphertext, RSA* key) {
    int rsaLen = RSA_size(key);
    int len = ciphertext.size();
    std::string plaintext;

    for (int i = 0; i < len; i += rsaLen) {
        std::vector<unsigned char> buffer(rsaLen);
        std::string substr = ciphertext.substr(i, rsaLen);

        int result = RSA_private_decrypt(substr.size(), reinterpret_cast<const unsigned char*>(substr.data()), buffer.data(), key, RSA_PKCS1_OAEP_PADDING);
        if (result == -1) {
            std::cerr << "Decryption error: " << ERR_error_string(ERR_get_error(), NULL) << std::endl;
            return "";
        }

        plaintext.append(reinterpret_cast<char*>(buffer.data()), result);
    }

    return plaintext;
}

/**
 * @brief Decodes the given base64-encoded string.
 * 
 * @param input The base64-encoded input string.
 * @return The decoded output string.
 */
std::string Decryption::base64_decode(const std::string& input) {
    BIO* b64 = BIO_new(BIO_f_base64());
    BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);

    BIO* bmem = BIO_new_mem_buf(input.data(), input.size());
    b64 = BIO_push(b64, bmem);

    std::string output;
    output.resize(input.size());

    int decoded_size = BIO_read(b64, &output[0], input.size());
    output.resize(decoded_size);
    BIO_free_all(b64);

    return output;
}

/**
 * @brief Loads the private key from the given PEM string.
 * 
 * @param privateKeyString The PEM string representing the private key.
 * @return The loaded RSA private key.
 */
RSA* Decryption::loadPrivateKey(const std::string& privateKeyString) {
    RSA* key = nullptr;
    BIO* privateKeyBio = BIO_new_mem_buf(privateKeyString.data(), privateKeyString.size());

    if (!privateKeyBio) {
        std::cerr << "Error creating BIO for private key" << std::endl;
        return nullptr;
    }

    char* passphrase_cstr = const_cast<char*>(passphrase.c_str());
    if (!PEM_read_bio_RSAPrivateKey(privateKeyBio, &key, NULL, passphrase_cstr)) {
        std::cerr << "Error reading private key" << std::endl;
        BIO_free(privateKeyBio);
        return nullptr;
    }

    BIO_free(privateKeyBio);
    return key;
}

/**
 * @brief Decrypts the given encrypted session key using the provided RSA key.
 * 
 * @param encryptedSessionKey The encrypted session key.
 * @param key The RSA key for decryption.
 * @return The decrypted session key.
 */
std::string Decryption::decryptSessionKey(const std::string& encryptedSessionKey, RSA* key) {
    std::vector<unsigned char> decryptedSessionKey(RSA_size(key));
    if (RSA_private_decrypt(encryptedSessionKey.size(), reinterpret_cast<const unsigned char*>(encryptedSessionKey.data()), decryptedSessionKey.data(), key, RSA_PKCS1_OAEP_PADDING) == -1) {
        std::cerr << "Session key decryption error: " << ERR_error_string(ERR_get_error(), NULL) << std::endl;
        return "";
    }

    return std::string(reinterpret_cast<char*>(decryptedSessionKey.data()), decryptedSessionKey.size());
}

// int main(int argc, char* argv[]) {
//     if (argc < 2) {
//         std::cerr << "Usage: " << argv[0] << " <encrypted file> <public key>\n";
//         return -1;
//     }

//     std::string encryptedFile = argv[1];

//     Decryption decryption(encryptedFile);

//     return 0;
// }
