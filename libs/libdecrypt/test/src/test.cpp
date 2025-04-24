#include "test.h"

#ifndef PUBLIC_KEY_FILE
#define PUBLIC_KEY_FILE "public_key.pem" // Default public key file
#endif

// Function to create a sample XML file for testing
void createTestXMLFile(const std::string& filePath) {
    std::ofstream outFile(filePath);
    outFile << "<Test>\n";
    outFile << "  <Data>Encryption Data</Data>\n";
    outFile << "</Test>";
    outFile.close();
}

// Function to read the contents of a file into a string
std::string readFileToString(const std::string& filePath) {
    std::ifstream inFile(filePath, std::ios::binary);
    if (!inFile) {
        throw std::runtime_error("Unable to open file: " + filePath);
    }

    std::ostringstream ss;
    ss << inFile.rdbuf();
    return ss.str();
}