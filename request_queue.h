#pragma once
#include <deque>
#include <string>
#include <vector>

#include "search_server.h"

class RequestQueue {
   public:
    explicit RequestQueue(const SearchServer& search_server);

    template <typename Comparator>
    std::vector<Document> AddFindRequest(const std::string& raw_query, Comparator comparator);
    std::vector<Document> AddFindRequest(const std::string& raw_query, DocumentStatus status);
    std::vector<Document> AddFindRequest(const std::string& raw_query);

    int GetNoResultRequests() const;

   private:
    struct QueryResult {
        std::string query;
        size_t result_count;
    };
    int empty_count_ = 0;
    std::deque<QueryResult> requests_;
    const static int sec_in_day_ = 1440;
    const SearchServer& search_server_;

    void AddRequest(const std::string& raw_query, const size_t result_count);
};

#include "request_queue.cpp"
