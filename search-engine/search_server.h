#pragma once
#include <algorithm>
#include <execution>
#include <map>
#include <mutex>
#include <set>
#include <string>
#include <string_view>
#include <tuple>
#include <vector>

#include "document.h"
#include "log_duration.h"
#include "string_processing.h"

const int MAX_RESULT_DOCUMENT_COUNT = 5;
const double EPS = 1e-6;
const std::string OPERATION_TIME_STRING = "Operation time";

class SearchServer {
   public:
    template <typename Container>
    SearchServer(const Container& stop_words);
    SearchServer();
    SearchServer(const std::string& text);
    SearchServer(std::string_view text);

    void AddDocument(int document_id, std::string_view document, DocumentStatus status, const std::vector<int>& ratings);

    template <typename ExecutionPolicy>
    void RemoveDocument(ExecutionPolicy&& policy, int document_id);
    void RemoveDocument(int document_id);

    template <typename ExecutionPolicy, typename Comparator>
    std::vector<Document> FindTopDocuments(ExecutionPolicy&& policy, std::string_view raw_query, Comparator comparator) const;
    template <typename ExecutionPolicy>
    std::vector<Document> FindTopDocuments(ExecutionPolicy&& policy, std::string_view raw_query) const;
    template <typename ExecutionPolicy>
    std::vector<Document> FindTopDocuments(ExecutionPolicy&& policy, std::string_view raw_query, DocumentStatus status) const;
    template <typename Comparator>
    std::vector<Document> FindTopDocuments(std::string_view raw_query, Comparator comparator) const;
    std::vector<Document> FindTopDocuments(std::string_view raw_query) const;
    std::vector<Document> FindTopDocuments(std::string_view raw_query, DocumentStatus status) const;

    template <typename ExecutionPolicy>
    std::tuple<std::vector<std::string_view>, DocumentStatus> MatchDocument(ExecutionPolicy&& policy,
                                                                            std::string_view raw_query, int document_id) const;
    std::tuple<std::vector<std::string_view>, DocumentStatus> MatchDocument(std::string_view raw_query, int document_id) const;

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

    static bool HasSpecialCharacters(std::string_view word);

    bool IsStopWord(std::string_view word) const;

    std::vector<std::string_view> SplitIntoWordsNoStopAndValid(std::string_view text) const;

    static int ComputeAverageRating(const std::vector<int>& ratings);

    struct QueryWord {
        std::string_view data;
        bool is_minus;
        bool is_stop;
    };

    QueryWord ParseQueryWord(std::string_view text) const;

    struct Query {
        std::set<std::string_view> plus_words;
        std::set<std::string_view> minus_words;
    };

    Query ParseQuery(std::string_view text) const;

    // Existence required
    double ComputeWordInverseDocumentFreq(const std::string& word) const;

    template <typename ExecutionPolicy, typename Comparator>
    std::vector<Document> FindAllDocuments(ExecutionPolicy&& policy, const Query& query, Comparator comparator) const;
};

template <typename Container>
SearchServer::SearchServer(const Container& stop_words) {
    using namespace std::string_literals;

    for (std::string_view word : stop_words) {
        if (word.empty()) {
            continue;
        }

        if (HasSpecialCharacters(word)) {
            throw std::invalid_argument("Step words mustn't include special characters"s);
        }

        stop_words_.insert(std::string(word));
    }
}

template <typename ExecutionPolicy>
void SearchServer::RemoveDocument(ExecutionPolicy&& policy, int document_id) {
    if (document_to_word_freqs_.count(document_id) == 0) {
        return;
    }

    const std::map<std::string, double>& word_freqs = document_to_word_freqs_.at(document_id);
    std::for_each(
        policy,
        word_freqs.begin(), word_freqs.end(),
        [&](const auto& kv) {
            word_to_document_freqs_.at(kv.first).erase(document_id);
        });

    document_to_word_freqs_.erase(document_id);
    document_ratings_status_.erase(document_id);
    document_ids_.erase(document_id);
}

template <typename ExecutionPolicy, typename Comparator>
std::vector<Document> SearchServer::FindTopDocuments(ExecutionPolicy&& policy,
                                                     std::string_view raw_query, Comparator comparator) const {
    const Query query = ParseQuery(raw_query);
    auto matched_documents = FindAllDocuments(policy, query, comparator);

    std::sort(
        policy,
        matched_documents.begin(), matched_documents.end(),
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

template <typename ExecutionPolicy>
std::vector<Document> SearchServer::FindTopDocuments(ExecutionPolicy&& policy, std::string_view raw_query) const {
    return FindTopDocuments(policy, raw_query, DocumentStatus::ACTUAL);
}

template <typename ExecutionPolicy>
std::vector<Document> SearchServer::FindTopDocuments(ExecutionPolicy&& policy, std::string_view raw_query, DocumentStatus status) const {
    return FindTopDocuments(policy, raw_query, [status](int document_id, DocumentStatus stat, int rating) {
        return stat == status;
    });
}

template <typename Comparator>
std::vector<Document> SearchServer::FindTopDocuments(std::string_view raw_query, Comparator comparator) const {
    return FindTopDocuments(std::execution::seq, raw_query, comparator);
}

template <typename ExecutionPolicy>
std::tuple<std::vector<std::string_view>, DocumentStatus> SearchServer::MatchDocument(
    ExecutionPolicy&& policy, std::string_view raw_query, int document_id) const {
    const Query query = ParseQuery(raw_query);
    std::vector<std::string_view> match_words;

    for (std::string_view word : query.plus_words) {
        // to create a string_view we should use such string that will not die after outing out
        // of scope (so we use "word_s" below)
        auto it = word_to_document_freqs_.find(std::string(word));
        const auto& word_s = it->first;
        const auto& doc_freqs = it->second;

        int count_doc_id = count_if(
            policy,
            doc_freqs.begin(), doc_freqs.end(),
            [&document_id](const auto& kv) {
                return kv.first == document_id;
            });

        if (count_doc_id) {
            match_words.push_back(std::string_view(word_s));
        }
    }

    for (std::string_view word : query.minus_words) {
        const std::map<int, double>& doc_freqs = word_to_document_freqs_.at(std::string(word));

        int count_doc_id = count_if(
            policy,
            doc_freqs.begin(), doc_freqs.end(),
            [&document_id](const auto& kv) {
                return kv.first == document_id;
            });

        if (count_doc_id) {
            match_words.clear();
        }
    }

    return std::make_tuple(match_words, document_ratings_status_.at(document_id).status);
}

template <typename ExecutionPolicy, typename Comparator>
std::vector<Document> SearchServer::FindAllDocuments(ExecutionPolicy&& policy, const Query& query, Comparator comparator) const {
    std::map<int, double> document_to_relevance;
    std::mutex m;  // for writing to document_to_relevance

    std::for_each(
        policy,
        query.plus_words.begin(), query.plus_words.end(),
        [&](std::string_view word_v) {
            std::string word(word_v);
            if (word_to_document_freqs_.count(word)) {
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

                    std::lock_guard guard(m);
                    document_to_relevance[document_id] += term_freq * inverse_document_freq;
                }
            }
        });

    for (std::string_view word_v : query.minus_words) {
        std::string word(word_v);
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }

        for (const auto [document_id, _] : word_to_document_freqs_.at(word)) {
            document_to_relevance.erase(document_id);
        }
    }

    std::vector<Document> matched_documents(document_to_relevance.size());
    std::transform(
        policy,
        document_to_relevance.begin(), document_to_relevance.end(),
        matched_documents.begin(),
        [&](const auto& kv) {
            Document document_data = document_ratings_status_.at(kv.first);

            return Document(kv.first, kv.second, document_data.rating, document_data.status);
        });

    return matched_documents;
}
