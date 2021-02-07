#pragma once
#include <algorithm>
#include <map>
#include <set>
#include <string>
#include <tuple>

#include "document.h"
#include "string_processing.h"

const int MAX_RESULT_DOCUMENT_COUNT = 5;
const double EPS = 1e-6;

class SearchServer {
   public:
    SearchServer();
    SearchServer(const std::string& text);
    template <typename Container>
    SearchServer(const Container& stop_words);

    void AddDocument(int document_id, const std::string& document, DocumentStatus status, const std::vector<int>& ratings);

    template <typename Comparator>
    std::vector<Document> FindTopDocuments(const std::string& raw_query, Comparator comparator) const;
    std::vector<Document> FindTopDocuments(const std::string& raw_query, DocumentStatus status) const;

    std::tuple<std::vector<std::string>, DocumentStatus> MatchDocument(const std::string& raw_query, int document_id) const;

    int GetDocumentCount() const;
    int GetDocumentId(int order) const;

   private:
    std::set<std::string> stop_words_;
    std::map<std::string, std::map<int, double>> word_to_document_freqs_;
    std::map<int, Document> document_ratings_status_;
    std::vector<int> document_id_by_order_;

    static bool HasSpecialCharacters(const std::string& word);

    bool IsStopWord(const std::string& word) const;

    std::vector<std::string> SplitIntoWordsNoStopAndValid(const std::string& text) const;

    static int ComputeAverageRating(const std::vector<int>& ratings);

    struct QueryWord {
        std::string data;
        bool is_minus;
        bool is_stop;
    };

    QueryWord ParseQueryWord(std::string text) const;

    struct Query {
        std::set<std::string> plus_words;
        std::set<std::string> minus_words;
    };

    Query ParseQuery(const std::string& text) const;

    // Existence required
    double ComputeWordInverseDocumentFreq(const std::string& word) const;

    template <typename Comparator>
    std::vector<Document> FindAllDocuments(const Query& query, Comparator comparator) const;
};

#include "search_server.cpp"
