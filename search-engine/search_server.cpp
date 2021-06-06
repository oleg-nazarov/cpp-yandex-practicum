#include "search_server.h"

#include <algorithm>
#include <cmath>
#include <execution>
#include <set>
#include <stdexcept>
#include <string>
#include <string_view>
#include <tuple>
#include <vector>

#include "document.h"
#include "log_duration.h"
#include "string_processing.h"

using namespace std::string_literals;

SearchServer::SearchServer() = default;

SearchServer::SearchServer(const std::string& text) : SearchServer(std::string_view(text)) {}

SearchServer::SearchServer(std::string_view text) : SearchServer(SplitIntoWords(text)) {}

void SearchServer::AddDocument(int document_id, std::string_view document, DocumentStatus status, const std::vector<int>& ratings) {
    if (document_id < 0) {
        throw std::invalid_argument("Document id mustn't be negative"s);
    } else if (document_ratings_status_.count(document_id)) {
        throw std::invalid_argument("Document with such id has already added"s);
    }

    const std::vector<std::string_view> words = SplitIntoWordsNoStopAndValid(document);
    const double inv_word_count = 1.0 / words.size();

    for (const std::string_view word_v : words) {
        std::string word(word_v);

        word_to_document_freqs_[word][document_id] += inv_word_count;
        document_to_word_freqs_[document_id][word] += inv_word_count;
    }

    document_ratings_status_[document_id] = Document(ComputeAverageRating(ratings), status);
    document_ids_.insert(document_id);
}

void SearchServer::RemoveDocument(int document_id) {
    RemoveDocument(std::execution::seq, document_id);
}

std::vector<Document> SearchServer::FindTopDocuments(std::string_view raw_query) const {
    return FindTopDocuments(std::execution::seq, raw_query);
}

std::vector<Document> SearchServer::FindTopDocuments(std::string_view raw_query, DocumentStatus status) const {
    return FindTopDocuments(std::execution::seq, raw_query, status);
}

std::tuple<std::vector<std::string_view>, DocumentStatus> SearchServer::MatchDocument(
    std::string_view raw_query, int document_id) const {
    return MatchDocument(std::execution::seq, raw_query, document_id);
}

int SearchServer::GetDocumentCount() const {
    return document_ratings_status_.size();
}

const std::map<std::string, double>& SearchServer::GetWordFrequencies(int document_id) const {
    const static std::map<std::string, double> EMPTY_MAP = {};

    if (document_to_word_freqs_.count(document_id)) {
        return document_to_word_freqs_.at(document_id);
    }

    return EMPTY_MAP;
}

std::set<int>::const_iterator SearchServer::begin() const {
    return document_ids_.begin();
}

std::set<int>::const_iterator SearchServer::end() const {
    return document_ids_.end();
}

bool SearchServer::HasSpecialCharacters(std::string_view word) {
    return std::any_of(word.begin(), word.end(), [](const char c) {
        return c >= '\0' && c < ' ';
    });
}

int SearchServer::ComputeAverageRating(const std::vector<int>& ratings) {
    if (ratings.empty()) {
        return 0;
    }

    int rating_sum = 0;

    for (const int rating : ratings) {
        rating_sum += rating;
    }

    return rating_sum / static_cast<int>(ratings.size());
}

bool SearchServer::IsStopWord(std::string_view word) const {
    return count_if(stop_words_.begin(), stop_words_.end(), [&word](std::string_view word_v) {
               return word_v == word;
           }) > 0;
}

std::vector<std::string_view> SearchServer::SplitIntoWordsNoStopAndValid(std::string_view text) const {
    std::vector<std::string_view> words;

    for (std::string_view word : SplitIntoWords(text)) {
        if (HasSpecialCharacters(word)) {
            throw std::invalid_argument("Document mustn't include special characters: \""s + std::string(word) + "\""s);
        }

        if (!IsStopWord(word)) {
            words.push_back(word);
        }
    }

    return words;
}

SearchServer::QueryWord SearchServer::ParseQueryWord(std::string_view text) const {
    bool is_minus = false;

    // Word shouldn't be empty
    if (text[0] == '-') {
        if (text.size() == 1u || text[1] == '-' || text[1] == ' ') {
            throw std::invalid_argument("There must by another word after \"minus\" sign"s);
        }

        is_minus = true;
        text.remove_prefix(1);
    }

    if (HasSpecialCharacters(text)) {
        throw std::invalid_argument("Text mustn't include special characters"s);
    }

    return {
        text,
        is_minus,
        IsStopWord(text),
    };
}

SearchServer::Query SearchServer::ParseQuery(std::string_view text) const {
    Query query;

    for (std::string_view word : SplitIntoWords(text)) {
        const QueryWord query_word = ParseQueryWord(word);

        if (!query_word.is_stop) {
            if (query_word.is_minus) {
                query.minus_words.insert(query_word.data);
            } else {
                query.plus_words.insert(query_word.data);
            }
        }
    }
    return query;
}

// Existence required
double SearchServer::ComputeWordInverseDocumentFreq(const std::string& word) const {
    return std::log(document_ratings_status_.size() * 1.0 / word_to_document_freqs_.at(word).size());
}
