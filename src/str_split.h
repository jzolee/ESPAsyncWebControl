#pragma once

#include <Arduino.h>
#include <vector>

void str_split(std::vector<String>& vector, const char* buf, size_t len, const char delimiter);
void str_split(std::vector<String>& vector, const String& str, const char delimiter);
std::vector<String> str_split(const char* buf, size_t len, const char delimiter);
std::vector<String> str_split(const String& str, const char delimiter);