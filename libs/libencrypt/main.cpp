#include <iostream>
#include "encryption.h"

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <publicKeyFile> <file_path> \n";
        return -1;
    }

    
    std::string publicKeyFile = argv[1];
    std::string filePath = argv[2];

    if (Encryption::encryptFile(publicKeyFile,filePath)) {
        std::cout << "Encryption completed successfully." << std::endl;
        return 0;
    } else {
        std::cerr << "Encryption failed." << std::endl;
        return -1;
    }
}
 
