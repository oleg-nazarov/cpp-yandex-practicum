#include "document.h"

#include <iostream>
#include <string>

Document::Document() : id(0), relevance(0.0), rating(0), status(DocumentStatus::ACTUAL) {}
Document::Document(int id, double relevance, int rating) : id(id), relevance(relevance), rating(rating), status(DocumentStatus::ACTUAL) {}
Document::Document(int id, double relevance, int rating, DocumentStatus status) : id(id), relevance(relevance), rating(rating), status(status) {}
Document::Document(int rating, DocumentStatus status) : rating(rating), status(status) {}

std::ostream& operator<<(std::ostream& os, const Document& document) {
    using namespace std::string_literals;

    os << "{ "s
       << "document_id = "s << document.id << ", "s
       << "relevance = "s << document.relevance << ", "s
       << "rating = "s << document.rating << " }"s;

    return os;
}
