#include "test.h"

int main() {
    std::string testFilePath = "test.xml";
    std::string publicKeyFile = PUBLIC_KEY_FILE; // Path to your public key file
    std::string encryptedFilePath = "test.xmle";

    // Step 1: Create a test XML file
    createTestXMLFile(testFilePath);
    
    std::string originalContent = readFileToString(testFilePath);

    // Step 2: Encrypt XML content
    Encryption encryption;
    if (!encryption.encryptFile(publicKeyFile, testFilePath)) {
        std::cerr << "Encryption failed" << std::endl;
        return 1;
    }

    // Step 3: Decrypt XML content
    Decryption decryption(encryptedFilePath);
    std::string decryptedContent = decryption.getDecryptedContent();

    // Step 4: Compare original XML content with decrypted XML content
    if (decryptedContent == originalContent) {
        std::cout << "Test passed: Decrypted content matches original content" << std::endl;
    } else {
        std::cerr << "Test failed: Decrypted content does not match original content" << std::endl;
        return 1;
    }

    return 0;
}