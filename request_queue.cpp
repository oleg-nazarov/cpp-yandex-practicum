#include "request_queue.h"

#include <deque>
#include <string>
#include <vector>

#include "search_server.h"

RequestQueue::RequestQueue(const SearchServer& search_server) : search_server_(search_server) {}

std::vector<Document> RequestQueue::AddFindRequest(const std::string& raw_query, DocumentStatus status) {
    std::vector<Document> top_documents = search_server_.FindTopDocuments(raw_query, status);
    RequestQueue::AddRequest(raw_query, top_documents.size());

    return top_documents;
}

std::vector<Document> RequestQueue::AddFindRequest(const std::string& raw_query) {
    std::vector<Document> top_documents = search_server_.FindTopDocuments(raw_query);
    RequestQueue::AddRequest(raw_query, top_documents.size());

    return top_documents;
}

int RequestQueue::GetNoResultRequests() const {
    return RequestQueue::empty_count_;
}

void RequestQueue::AddRequest(const std::string& raw_query, const size_t result_count) {
    if (RequestQueue::requests_.size() == RequestQueue::sec_in_day_) {
        if (RequestQueue::requests_.front().result_count == 0u) {
            --RequestQueue::empty_count_;
        }

        RequestQueue::requests_.pop_front();
    }

    RequestQueue::requests_.push_back({raw_query, result_count});

    if (result_count == 0u) {
        ++RequestQueue::empty_count_;
    }
}
