#ifndef TEST_H
#define TEST_H

#include <iostream>
#include <fstream>
#include <string>
#include "encryption.h"
#include "decryption.h"

void createTestXMLFile(const std::string& filePath);
std::string readFileToString(const std::string& filePath);


#endif // TEST_H