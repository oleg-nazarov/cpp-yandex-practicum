#pragma once

#include <string>
#include <vector>

#include "document.h"
#include "search_server.h"

void AddDocument(SearchServer& search_server, int document_id, const std::string& document,
                 DocumentStatus status, const std::vector<int>& ratings);