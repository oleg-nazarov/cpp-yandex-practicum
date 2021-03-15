#include "search_server.h"

#include <algorithm>
#include <cmath>
#include <set>
#include <stdexcept>
#include <string>
#include <tuple>
#include <vector>

#include "document.h"
#include "log_duration.h"
#include "string_processing.h"

using namespace std::string_literals;

SearchServer::SearchServer() = default;

SearchServer::SearchServer(const std::string& text) : SearchServer(SplitIntoWords(text)) {}

void SearchServer::AddDocument(int document_id, const std::string& document, DocumentStatus status, const std::vector<int>& ratings) {
    if (document_id < 0) {
        throw std::invalid_argument("Document id mustn't be negative"s);
    } else if (document_ratings_status_.count(document_id)) {
        throw std::invalid_argument("Document with such id has already added"s);
    }

    const std::vector<std::string> words = SplitIntoWordsNoStopAndValid(document);
    const double inv_word_count = 1.0 / words.size();

    for (const std::string& word : words) {
        word_to_document_freqs_[word][document_id] += inv_word_count;

        document_to_word_freqs_[document_id][word] += inv_word_count;
    }

    document_ratings_status_[document_id] = Document(ComputeAverageRating(ratings), status);
    document_ids_.insert(document_id);
}

void SearchServer::RemoveDocument(int document_id) {
    if (document_to_word_freqs_.count(document_id) == 0) {
        return;
    }

    for (auto& [word, freqs] : document_to_word_freqs_.at(document_id)) {
        word_to_document_freqs_.at(word).erase(document_id);
    }

    document_to_word_freqs_.erase(document_id);
    document_ratings_status_.erase(document_id);
    document_ids_.erase(document_id);
}

std::vector<Document> SearchServer::FindTopDocuments(const std::string& raw_query) const {
    return FindTopDocuments(raw_query, DocumentStatus::ACTUAL);
}

std::vector<Document> SearchServer::FindTopDocuments(const std::string& raw_query, DocumentStatus status) const {
    return FindTopDocuments(raw_query, [status](int document_id, DocumentStatus stat, int rating) {
        return stat == status;
    });
}

std::tuple<std::vector<std::string>, DocumentStatus> SearchServer::MatchDocument(const std::string& raw_query, int document_id) const {
    LOG_DURATION_STREAM(OPERATION_TIME_STRING, std::cerr);

    const Query query = ParseQuery(raw_query);
    std::vector<std::string> match_words;

    for (const std::string& word : query.plus_words) {
        if (word_to_document_freqs_.at(word).count(document_id)) {
            match_words.push_back(word);
        }
    }

    for (const std::string& word : query.minus_words) {
        if (word_to_document_freqs_.at(word).count(document_id)) {
            match_words.clear();
        }
    }

    return std::make_tuple(match_words, document_ratings_status_.at(document_id).status);
}

int SearchServer::GetDocumentCount() const {
    return document_ratings_status_.size();
}

const std::map<std::string, double>& SearchServer::GetWordFrequencies(int document_id) const {
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
    return stop_words_.count(word) > 0;
}

std::vector<std::string> SearchServer::SplitIntoWordsNoStopAndValid(const std::string& text) const {
    std::vector<std::string> words;

    for (const std::string& word : SplitIntoWords(text)) {
        if (HasSpecialCharacters(word)) {
            throw std::invalid_argument("Document mustn't include special characters: \""s + word + "\""s);
        }

        if (!IsStopWord(word)) {
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

    if (HasSpecialCharacters(text)) {
        throw std::invalid_argument("Text mustn't include special characters"s);
    }

    return {
        text,
        is_minus,
        IsStopWord(text),
    };
}

SearchServer::Query SearchServer::ParseQuery(const std::string& text) const {
    Query query;

    for (const std::string& word : SplitIntoWords(text)) {
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
