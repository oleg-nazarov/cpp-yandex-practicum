#include <string>
#include <vector>

#include "document.h"
#include "search_server.h"

void AddDocument(SearchServer& search_server, int document_id, std::string_view document,
                 DocumentStatus status, const std::vector<int>& ratings) {
    search_server.AddDocument(document_id, document, status, ratings);
}
