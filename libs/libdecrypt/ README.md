 # libdecrypt

libdecrypt is a C++ library for decrypting encrypted files. It provides a simple interface to decrypt files using OpenSSL for encryption and decryption operations and pugixml for XML parsing.

## Features

- Decrypts encrypted files using RSA encryption algorithm.
- Supports loading private key from PEM string.
- Retrieves encrypted data and session key from XML file.
- Decrypts session key using RSA private key.
- Decrypts XML string using the decrypted session key.
- Outputs the decrypted content as a string.

## Installation

1. Build the library using CMake

    cd libdecrypt
    mkdir build
    cd build
    cmake ..
    make
2.  Install the library (optional):

    make install
## Dependencies

Make sure you have the following dependencies installed on your system:

- **OpenSSL**: The library depends on OpenSSL for encryption and decryption operations. Make sure you have OpenSSL installed on your system.

- **pugixml**: The library uses pugixml for XML parsing. It is included as a submodule in the project.

## Usage

To use the `libdecrypt` library in your C++ projects, follow the steps below:

1. Include the `decryption.h` header file in your source code:

   ```cpp
   #include "decryption.h"

2. Create a `Decryption` object with the path to the encrypted file:

    std::string encryptedFile = "path/to/encrypted/file";
    Decryption decryption(encryptedFile);

3. Decrypt the contents of the encrypted file:

    decryption.decryptFile();

4. Retrieve the decrypted content as a string:
 
    std::string decryptedContent = decryption.getDecryptedContent();

5. Optional: Write the decrypted content to a file:
 
    std::ofstream outputFile("output.txt");
    outputFile << decryptedContent;
    outputFile.close();

## Configuration

To configure `libdecrypt` for your specific use case, follow these steps:

### Private Key:

- **Option 1: Define `PRIVATE_KEY` in the code:**
  - Locate the `decryption.h` file in the project.
  - Uncomment the `#define PRIVATE_KEY` line.
  - Replace the dummy private key string with your actual private key in PEM format.

- **Option 2: Provide a `private_key.pem` file:**
  - Place your private key file in the project directory.
  - Make sure the file is named `private_key.pem`.
  - `libdecrypt` will automatically load the private key from this file.

### Passphrase:

- **Option 1: Define `PASSPHRASE` in the code:**
  - Locate the `decryption.h` file in the project.
  - Uncomment the `#define PASSPHRASE` line.
  - Replace the empty string with your actual passphrase.

- **Option 2: Provide a `config.txt` file:**
  - Create a plain text file named `config.txt` in the project directory.
  - Write your passphrase in the file.
  - `libdecrypt` will read the passphrase from this file.

Make sure to configure the private key and passphrase according to your specific requirements before using the `libdecrypt` library.

## License

This project is licensed under the MIT License

## Contributing

Contributions are welcome! If you find any issues or have suggestions for improvements, please feel free to open an issue or submit a pull request. We appreciate your contributions to make this project better.
