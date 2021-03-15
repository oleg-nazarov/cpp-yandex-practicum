#pragma once
#include <string>

#include "search_server.h"

const std::string DUPLICATE_ID_INFO_TEXT = "Found duplicate document id";

void RemoveDuplicates(SearchServer& search_server);
