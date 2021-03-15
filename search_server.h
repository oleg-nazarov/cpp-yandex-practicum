#pragma once
#include <algorithm>
#include <map>
#include <set>
#include <string>
#include <tuple>
#include <vector>

#include "document.h"
#include "log_duration.h"
#include "string_processing.h"

const int MAX_RESULT_DOCUMENT_COUNT = 5;
const double EPS = 1e-6;
const std::string OPERATION_TIME_STRING = "Operation time";
const std::map<std::string, double> EMPTY_MAP = {};

class SearchServer {
   public:
    template <typename Container>
    SearchServer(const Container& stop_words);
    SearchServer();
    SearchServer(const std::string& text);

    void AddDocument(int document_id, const std::string& document, DocumentStatus status, const std::vector<int>& ratings);
    void RemoveDocument(int document_id);

    template <typename Comparator>
    std::vector<Document> FindTopDocuments(const std::string& raw_query, Comparator comparator) const;
    std::vector<Document> FindTopDocuments(const std::string& raw_query) const;
    std::vector<Document> FindTopDocuments(const std::string& raw_query, DocumentStatus status) const;

    std::tuple<std::vector<std::string>, DocumentStatus> MatchDocument(const std::string& raw_query, int document_id) const;

    int GetDocumentCount() const;

    const std::map<std::string, double>& GetWordFrequencies(int document_id) const;

    std::set<int>::const_iterator begin() const;
    std::set<int>::const_iterator end() const;

   private:
    std::set<std::string> stop_words_;
    std::map<std::string, std::map<int, double>> word_to_document_freqs_;
    std::map<int, std::map<std::string, double>> document_to_word_freqs_;
    std::map<int, Document> document_ratings_status_;
    std::set<int> document_ids_;

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

template <typename Container>
SearchServer::SearchServer(const Container& stop_words) {
    using namespace std::string_literals;

    for (const std::string& word : stop_words) {
        if (word.empty()) {
            continue;
        }

        if (HasSpecialCharacters(word)) {
            throw std::invalid_argument("Step words mustn't include special characters"s);
        }

        stop_words_.insert(word);
    }
}

template <typename Comparator>
std::vector<Document> SearchServer::FindTopDocuments(const std::string& raw_query, Comparator comparator) const {
    LOG_DURATION_STREAM(OPERATION_TIME_STRING, std::cerr);

    const Query query = ParseQuery(raw_query);
    auto matched_documents = FindAllDocuments(query, comparator);

    std::sort(
        matched_documents.begin(),
        matched_documents.end(),
        [](const Document& lhs, const Document& rhs) {
            if (std::abs(lhs.relevance - rhs.relevance) < EPS) {
                return lhs.rating > rhs.rating;
            } else {
                return lhs.relevance > rhs.relevance;
            }
        });

    if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
        matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
    }

    return matched_documents;
}

template <typename Comparator>
std::vector<Document> SearchServer::FindAllDocuments(const Query& query, Comparator comparator) const {
    std::map<int, double> document_to_relevance;

    for (const std::string& word : query.plus_words) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }

        const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);

        for (const auto [document_id, term_freq] : word_to_document_freqs_.at(word)) {
            Document document_data = document_ratings_status_.at(document_id);

            bool should_add_document = comparator(
                document_id,
                document_data.status,
                document_data.rating);

            if (!should_add_document) {
                continue;
            }

            document_to_relevance[document_id] += term_freq * inverse_document_freq;
        }
    }

    for (const std::string& word : query.minus_words) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }

        for (const auto [document_id, _] : word_to_document_freqs_.at(word)) {
            document_to_relevance.erase(document_id);
        }
    }

    std::vector<Document> matched_documents;
    for (const auto [document_id, relevance] : document_to_relevance) {
        Document document_data = document_ratings_status_.at(document_id);

        matched_documents.push_back({document_id,
                                     relevance,
                                     document_data.rating,
                                     document_data.status});
    }
    return matched_documents;
}
