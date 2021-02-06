#pragma once
#include <iostream>

enum class DocumentStatus {
    ACTUAL,
    IRRELEVANT,
    BANNED,
    REMOVED
};

struct Document {
    int id;
    double relevance;
    int rating;
    DocumentStatus status;

    Document();
    Document(int id, double relevance, int rating);
    Document(int id, double relevance, int rating, DocumentStatus status);
    Document(int rating, DocumentStatus status);
};

std::ostream& operator<<(std::ostream& os, const Document& document);
