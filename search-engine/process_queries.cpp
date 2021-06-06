#include "process_queries.h"

#include <algorithm>
#include <execution>
#include <iterator>
#include <string>
#include <vector>

#include "document.h"
#include "search_server.h"

std::vector<std::vector<Document>> ProcessQueries(const SearchServer& search_server, const std::vector<std::string>& queries) {
    std::vector<std::vector<Document>> documents(queries.size());

    std::transform(
        std::execution::par,
        queries.begin(), queries.end(),
        documents.begin(),
        [&search_server](const std::string& query) {
            return search_server.FindTopDocuments(query);
        });

    return documents;
}

std::vector<Document> ProcessQueriesJoined(const SearchServer& search_server, const std::vector<std::string>& queries) {
    auto documents = ProcessQueries(search_server, queries);
    std::vector<Document> joined_documents;

    for (auto& doc : documents) {
        joined_documents.insert(
            joined_documents.end(),
            std::make_move_iterator(doc.begin()), std::make_move_iterator(doc.end()));
    }

    return joined_documents;
}
