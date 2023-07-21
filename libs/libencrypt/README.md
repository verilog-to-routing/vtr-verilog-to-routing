 # libencrypt (XML Encryption)

libencrypt is a C++ library that provides XML encryption functionality using the RSA encryption algorithm. It allows you to encrypt XML data and files using a public key, perform encryption-related operations, and save the encrypted data in an XML format.

## Prerequisites

Before using the libencrypt library, make sure you have the following prerequisites installed:

- C++ compiler with C++17 support
- OpenSSL library (version 1.1 or higher)

## Installation

To use the libencrypt library in your project, follow these steps:

1. Clone the libencrypt repository or download the source code.

2. Configure the library by replacing the contents of the `public_key.pem` file with your own RSA public key. Also, update the `config.txt` file with your desired passphrase.

3. Build the library using CMake:

   ```bash
   cmake .
   make

## Usage

To use the libencrypt library in your code, follow these steps:

1. Include the `encryption.h` header in your source file.

2. Use the provided functions to perform XML encryption operations:

   - `loadPublicKey`: Loads a public key from a file.
   - `encryptSessionKey`: Encrypts a session key using the provided public key.
   - `encrypt`: Encrypts XML data using the provided public key.
   - `base64_encode`: Base64 encodes a string.
   - `encryptFile`: Encrypts an XML file using the provided public key.

3. Customize the encryption process as needed based on your XML structure and encryption requirements.

## Example

Here's an example of how to use the libencrypt library to encrypt an XML file:

```cpp
#include "encryption.h"

int main() {
    std::string filePath = "data.xml";
    
    // Encrypt the XML file using the provided public key
    bool success = Encryption::encryptFile(filePath);
    
    if (success) {
        std::cout << "XML file encryption successful!" << std::endl;
    } else {
        std::cerr << "XML file encryption failed." << std::endl;
    }
    
    return 0;
    }


##License

This project is licensed under the MIT License

##Contributing

Contributions are welcome! If you find any issues or have suggestions for improvements, please feel free to open an issue or submit a pull request. We appreciate your contributions to make this project better.