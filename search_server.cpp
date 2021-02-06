#include "search_server.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <string>
#include <tuple>

#include "document.h"
#include "string_processing.h"

using namespace std::string_literals;

SearchServer::SearchServer() = default;

SearchServer::SearchServer(const std::string& text) : SearchServer(SplitIntoWords(text)) {}

void SearchServer::AddDocument(int document_id, const std::string& document, DocumentStatus status, const std::vector<int>& ratings) {
    if (document_id < 0) {
        throw std::invalid_argument("Document id mustn't be negative"s);
    } else if (SearchServer::document_ratings_status_.count(document_id)) {
        throw std::invalid_argument("Document with such id has already added"s);
    }

    const std::vector<std::string> words = SearchServer::SplitIntoWordsNoStopAndValid(document);
    const double inv_word_count = 1.0 / words.size();

    for (const std::string& word : words) {
        SearchServer::word_to_document_freqs_[word][document_id] += inv_word_count;
    }

    SearchServer::document_ratings_status_[document_id] = Document(SearchServer::ComputeAverageRating(ratings), status);
}

std::vector<Document> SearchServer::FindTopDocuments(const std::string& raw_query) const {
    return SearchServer::FindTopDocuments(raw_query, [](int document_id, DocumentStatus stat, int rating) {
        return stat == DocumentStatus::ACTUAL;
    });
}

std::vector<Document> SearchServer::FindTopDocuments(const std::string& raw_query, DocumentStatus status = DocumentStatus::ACTUAL) const {
    return SearchServer::FindTopDocuments(raw_query, [status](int document_id, DocumentStatus stat, int rating) {
        return stat == status;
    });
}

std::tuple<std::vector<std::string>, DocumentStatus> SearchServer::MatchDocument(const std::string& raw_query, int document_id) const {
    const Query query = SearchServer::ParseQuery(raw_query);
    std::vector<std::string> match_words;

    for (const std::string& word : query.plus_words) {
        if (SearchServer::word_to_document_freqs_.at(word).count(document_id)) {
            match_words.push_back(word);
        }
    }

    for (const std::string& word : query.minus_words) {
        if (SearchServer::word_to_document_freqs_.at(word).count(document_id)) {
            match_words.clear();
        }
    }

    return std::make_tuple(match_words, SearchServer::document_ratings_status_.at(document_id).status);
}

int SearchServer::GetDocumentCount() const {
    return SearchServer::document_ratings_status_.size();
}

int SearchServer::GetDocumentId(int order) const {
    if (order < 0 || order > static_cast<int>(SearchServer::document_id_by_order_.size()) - 1) {
        throw std::out_of_range("Index of document is out of range"s);
    }

    return SearchServer::document_id_by_order_[order];
}

bool SearchServer::HasSpecialCharacters(const std::string& word) {
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

bool SearchServer::IsStopWord(const std::string& word) const {
    return SearchServer::stop_words_.count(word) > 0;
}

std::vector<std::string> SearchServer::SplitIntoWordsNoStopAndValid(const std::string& text) const {
    std::vector<std::string> words;

    for (const std::string& word : SplitIntoWords(text)) {
        if (SearchServer::HasSpecialCharacters(word)) {
            throw std::invalid_argument("Document mustn't include special characters: \""s + word + "\""s);
        }

        if (!SearchServer::IsStopWord(word)) {
            words.push_back(word);
        }
    }

    return words;
}

SearchServer::QueryWord SearchServer::ParseQueryWord(std::string text) const {
    bool is_minus = false;

    // Word shouldn't be empty
    if (text[0] == '-') {
        if (text.size() == 1u || text[1] == '-' || text[1] == ' ') {
            throw std::invalid_argument("There must by another word after \"minus\" sign"s);
        }

        is_minus = true;
        text = text.substr(1);
    }

    if (SearchServer::HasSpecialCharacters(text)) {
        throw std::invalid_argument("Text mustn't include special characters"s);
    }

    return {
        text,
        is_minus,
        SearchServer::IsStopWord(text),
    };
}

SearchServer::Query SearchServer::ParseQuery(const std::string& text) const {
    SearchServer::Query query;

    for (const std::string& word : SplitIntoWords(text)) {
        const SearchServer::QueryWord query_word = SearchServer::ParseQueryWord(word);

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
    return std::log(SearchServer::document_ratings_status_.size() * 1.0 / SearchServer::word_to_document_freqs_.at(word).size());
}
