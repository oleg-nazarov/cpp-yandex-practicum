#include "request_queue.h"

#include <deque>
#include <string>
#include <vector>

#include "search_server.h"

RequestQueue::RequestQueue(const SearchServer& search_server) : search_server_(search_server) {}

std::vector<Document> RequestQueue::AddFindRequest(const std::string& raw_query, DocumentStatus status) {
    std::vector<Document> top_documents = search_server_.FindTopDocuments(raw_query, status);
    AddRequest(raw_query, top_documents.size());

    return top_documents;
}

std::vector<Document> RequestQueue::AddFindRequest(const std::string& raw_query) {
    std::vector<Document> top_documents = search_server_.FindTopDocuments(raw_query);
    AddRequest(raw_query, top_documents.size());

    return top_documents;
}

int RequestQueue::GetNoResultRequests() const {
    return empty_count_;
}

void RequestQueue::AddRequest(const std::string& raw_query, const size_t result_count) {
    if (requests_.size() == sec_in_day_) {
        if (requests_.front().result_count == 0u) {
            --empty_count_;
        }

        requests_.pop_front();
    }

    requests_.push_back({raw_query, result_count});

    if (result_count == 0u) {
        ++empty_count_;
    }
}
