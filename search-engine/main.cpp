#include <cmath>
#include <iomanip>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

#include "document.h"
#include "paginator.h"
#include "process_queries.h"
#include "remove_duplicates.h"
#include "request_queue.h"
#include "search_server.h"
#include "test_example_functions.h"

using namespace std::string_literals;
using namespace std::string_view_literals;

// макросы ASSERT, ASSERT_EQUAL, ASSERT_EQUAL_HINT, ASSERT_HINT и RUN_TEST
#define RUN_TEST(func) \
    func();            \
    std::cerr << #func << ": OK." << std::endl;

template <typename A, typename B>
void AssertEqualImpl(const A a, const B b, const std::string& a_str, const std::string& b_str,
                     const std::string& file_name, const int line_number, const std::string& func_name,
                     const std::string& hint) {
    if (a != b) {
        std::cerr << std::boolalpha;
        std::cerr << file_name << "("s << line_number << "): "s << func_name << ": "s;
        std::cerr << "ASSERT_EQUAL("s << a_str << ", "s << b_str << ") failed: "s;
        std::cerr << a << " != "s << b << "."s;

        if (!hint.empty()) {
            std::cerr << " Hint: "s << hint;
        }

        std::abort();
    }
}
#define ASSERT_EQUAL(a, b) AssertEqualImpl(a, b, #a, #b, __FILE__, __LINE__, __FUNCTION__, "")
#define ASSERT_EQUAL_HINT(a, b, hint) AssertEqualImpl(a, b, #a, #b, __FILE__, __LINE__, __FUNCTION__, hint)

void AssertImpl(const bool value, const std::string& value_str, const std::string& file_name,
                const int line_number, const std::string& func_name, const std::string& hint) {
    if (!value) {
        std::cerr << file_name << "("s << line_number << "): "s << func_name << ": "s;
        std::cerr << "ASSERT("s << value_str << ") failed."s;

        if (!hint.empty()) {
            std::cerr << " Hint: "s << hint;
        }

        std::abort();
    }
}
#define ASSERT(value) AssertImpl(value, #value, __FILE__, __LINE__, __FUNCTION__, "")
#define ASSERT_HINT(value, hint) AssertImpl(value, #value, __FILE__, __LINE__, __FUNCTION__, hint)

std::ostream& operator<<(std::ostream& os, DocumentStatus doc) {
    const char* status = 0;
#define PROCESS_DOC(p) \
    case (p):          \
        status = #p;   \
        break;
    switch (doc) {
        PROCESS_DOC(DocumentStatus::ACTUAL);
        PROCESS_DOC(DocumentStatus::IRRELEVANT);
        PROCESS_DOC(DocumentStatus::BANNED);
        PROCESS_DOC(DocumentStatus::REMOVED);
    }
#undef PROCESS_DOC

    os << status;

    return os;
}

// -------- the start of unit tests for a search engine ----------
void TestExcludeStopWordsFromAddedDocumentContent() {
    const int doc_id = 42;
    const std::string content = "cat in the city";
    const std::vector<int> ratings = {1, 2, 3};

    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments("in"s);
        ASSERT_EQUAL(found_docs.size(), 1u);
        const Document& doc0 = found_docs[0];
        ASSERT_EQUAL(doc0.id, doc_id);
    }

    {
        SearchServer server("in the"sv);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        ASSERT_HINT(server.FindTopDocuments("in"s).empty(), "Stop words must be excluded from documents"s);
    }
}

void TestDocumentsCount() {
    const int doc_id_1 = 42;
    const int doc_id_2 = 43;
    const std::string content = "cat in the city";
    const std::vector<int> ratings = {1, 2, 3};

    {
        SearchServer server;
        server.AddDocument(doc_id_1, content, DocumentStatus::IRRELEVANT, ratings);
        const size_t doc_count_1 = server.GetDocumentCount();
        ASSERT_EQUAL(doc_count_1, 1u);

        server.AddDocument(doc_id_2, content, DocumentStatus::BANNED, ratings);
        const size_t doc_count_2 = server.GetDocumentCount();
        ASSERT_EQUAL(doc_count_2, 2u);
    }
}

void TestAddingDocument() {
    const int doc_id = 42;
    const int doc_id_negative = -42;
    const std::string content = "cat in the city";
    const std::vector<int> ratings = {1, 2, 3};

    // document with existing id has not been added
    {
        bool got_exception = false;

        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);

        try {
            server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        } catch (const std::invalid_argument& error) {
            // TODO: kludge
            got_exception = true;
        }

        if (!got_exception) {
            ASSERT_HINT(false, "should throw an expetion on adding document with existing id"s);
        } else {
            ASSERT(true);
        }
    }

    // document with negative id has not been added
    {
        SearchServer server;
        const size_t doc_count_1 = server.GetDocumentCount();

        try {
            server.AddDocument(doc_id_negative, content, DocumentStatus::ACTUAL, ratings);
        } catch (const std::invalid_argument& error) {
        }
        const size_t doc_count_2 = server.GetDocumentCount();
        ASSERT_EQUAL(doc_count_1, doc_count_2);
    }

    // document with special character has not been added
    {
        SearchServer server;
        const size_t doc_count_1 = server.GetDocumentCount();

        try {
            server.AddDocument(doc_id, content + "\x12"s, DocumentStatus::ACTUAL, ratings);
        } catch (const std::invalid_argument& error) {
        }
        const size_t doc_count_2 = server.GetDocumentCount();
        ASSERT_EQUAL(doc_count_1, doc_count_2);
    }
}

void TestExcludeDocumentsWithMinusWordsFromSearchResult() {
    const int doc_id = 42;
    const std::string content = "cat in the city";
    const std::vector<int> ratings = {1, 2, 3};

    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments("cat -city"s);
        ASSERT_EQUAL(found_docs.size(), 0u);
    }
}

void TestMatchingDocument() {
    const int doc_id = 42;
    const std::string content = "cat in the city";
    const std::vector<int> ratings = {1, 2, 3};

    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto [words, statuses] = server.MatchDocument("city"s, doc_id);
        ASSERT_EQUAL(words[0], "city"s);
        ASSERT_EQUAL(statuses, DocumentStatus::ACTUAL);
    }

    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto matched_words = server.MatchDocument("-cat city"s, doc_id);
        ASSERT(std::get<0>(matched_words).empty());
    }
}

void TestDocumentRelevance() {
    const int doc_id_city = 42;
    const std::string content_city = "cat in the city";
    const std::vector<int> ratings = {1, 2, 3};
    const int doc_id_village = 43;
    const std::string content_village = "cat in the village";

    {
        SearchServer server;
        server.AddDocument(doc_id_village, content_village, DocumentStatus::ACTUAL, ratings);
        server.AddDocument(doc_id_city, content_city, DocumentStatus::ACTUAL, ratings);

        const auto found_docs = server.FindTopDocuments("cat in the city"s);
        const Document& doc_0 = found_docs[0];
        ASSERT_EQUAL(doc_0.id, doc_id_city);
        const Document& doc_1 = found_docs[1];
        ASSERT_EQUAL(doc_1.id, doc_id_village);
    }
}

void TestDocumentRating() {
    const int doc_id = 42;
    const std::string content = "cat in the city";
    const std::vector<int> ratings = {3, 30, 300};

    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, {});
        const auto found_docs = server.FindTopDocuments("cat"s);
        ASSERT_EQUAL(found_docs[0].rating, 0);
    }

    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments("cat"s);
        ASSERT_EQUAL(found_docs[0].rating, (3 + 30 + 300) / 3);
    }
}

void TestSearchResultWithComparator() {
    const int doc_id = 42;
    const std::string content = "cat in the city";
    const std::vector<int> ratings = {1, 2, 3};

    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::IRRELEVANT, {});
        const auto found_docs = server.FindTopDocuments("cat"s);
        ASSERT_EQUAL(found_docs.size(), 0u);

        const auto found_docs_irrelevant = server.FindTopDocuments(
            "cat"s,
            [](int document_id, DocumentStatus status, int rating) {
                return status == DocumentStatus::IRRELEVANT;
            });
        ASSERT_EQUAL(found_docs_irrelevant[0].id, doc_id);
    }
}

void TestSearchResultToDocumentStatus() {
    const std::string content = "cat in the city";
    const int doc_id_actual = 42;
    const int doc_id_banned = 43;
    const int doc_id_irrelevant = 44;
    const int doc_id_removed = 45;

    {
        SearchServer server;
        server.AddDocument(doc_id_actual, content, DocumentStatus::ACTUAL, {});
        server.AddDocument(doc_id_banned, content, DocumentStatus::BANNED, {});
        server.AddDocument(doc_id_irrelevant, content, DocumentStatus::IRRELEVANT, {});
        server.AddDocument(doc_id_removed, content, DocumentStatus::REMOVED, {});

        const auto found_docs_actual = server.FindTopDocuments("cat"s, DocumentStatus::ACTUAL);
        ASSERT_EQUAL(found_docs_actual[0].id, doc_id_actual);

        const auto found_docs_banned = server.FindTopDocuments("cat"s, DocumentStatus::BANNED);
        ASSERT_EQUAL(found_docs_banned[0].id, doc_id_banned);

        const auto found_docs_irrelevant = server.FindTopDocuments("cat"s, DocumentStatus::IRRELEVANT);
        ASSERT_EQUAL(found_docs_irrelevant[0].id, doc_id_irrelevant);

        const auto found_docs_removed = server.FindTopDocuments("cat"s, DocumentStatus::REMOVED);
        ASSERT_EQUAL(found_docs_removed[0].id, doc_id_removed);
    }
}

void TestRelevanceCalculating() {
    // tf calculating
    std::cout << std::fixed << std::setprecision(6);

    {
        const int doc_id_1 = 42;
        const std::string content_1 = "cat other other";
        const int doc_id_2 = 43;
        const std::string content_2 = "other";
        const int doc_id_3 = 44;
        const std::string content_3 = "cat";
        const std::vector<int> ratings = {1, 2, 3};

        SearchServer server;
        server.AddDocument(doc_id_1, content_1, DocumentStatus::ACTUAL, ratings);
        server.AddDocument(doc_id_2, content_2, DocumentStatus::ACTUAL, ratings);
        server.AddDocument(doc_id_3, content_3, DocumentStatus::ACTUAL, ratings);

        // relevance = tf * idf;
        // founded two docs with "cat" below have the same idf, so we can use it to check
        // and compare tf (found_docs_cat[0] has tf "1" and found_docs_cat[1] has "1/3"):
        const auto found_docs_cat = server.FindTopDocuments("cat"s);
        const double is_zero_1 = abs(found_docs_cat[0].relevance / found_docs_cat[1].relevance - 3.0) < EPS;
        ASSERT(is_zero_1);

        const auto found_docs_other = server.FindTopDocuments("other"s);
        const double is_zero_2 = abs(found_docs_other[0].relevance / found_docs_other[1].relevance - 3.0 / 2) < EPS;
        ASSERT(is_zero_2);
    }

    // idf calculating
    {
        const int doc_id_1 = 42;
        const std::string content_1 = "cat";
        const int doc_id_2 = 43;
        const std::string content_2 = "other";
        const int doc_id_3 = 44;
        const std::string content_3 = "other";
        const std::vector<int> ratings = {1, 2, 3};

        SearchServer server;
        server.AddDocument(doc_id_1, content_1, DocumentStatus::ACTUAL, ratings);
        server.AddDocument(doc_id_2, content_2, DocumentStatus::ACTUAL, ratings);
        const auto found_docs_cat = server.FindTopDocuments("cat"s);
        const double cat_relevance = 1 * log(2.0 / 1);
        const double is_zero_1 = abs(found_docs_cat[0].relevance - cat_relevance) < EPS;
        ASSERT(is_zero_1);

        server.AddDocument(doc_id_3, content_3, DocumentStatus::ACTUAL, ratings);
        const auto found_docs_other = server.FindTopDocuments("cat"s);
        const double other_relevance = 1 * log(3.0 / 1);
        const double is_zero_2 = abs(found_docs_other[0].relevance - other_relevance) < EPS;
        ASSERT(is_zero_2);
    }
}

void TestProcessQueries() {
    const std::vector<std::string> texts = {
        "funny pet and nasty rat"s,
        "funny pet with curly hair"s,
        "funny pet and not very nasty rat"s,
        "pet with rat and rat and rat"s,
        "nasty rat with curly hair"s,
    };
    const std::vector<std::string> queries = {
        "nasty rat -not"s,
        "not very funny nasty pet"s,
        "curly hair"s,
    };

    SearchServer search_server("and with"sv);
    int id = 0;
    for (const std::string& text : texts) {
        search_server.AddDocument(++id, text, DocumentStatus::ACTUAL, {1, 2});
    }

    // ProcessQueries
    {
        const auto documents = ProcessQueries(search_server, queries);
        ASSERT_EQUAL(documents.size(), queries.size());
    }

    // ProcessQueriesJoined
    {
        const auto documents = ProcessQueriesJoined(search_server, queries);
        ASSERT_EQUAL(documents.size(), 10u);
    }
}

// Функция TestSearchServer является точкой входа для запуска тестов
void TestSearchServer() {
    RUN_TEST(TestExcludeStopWordsFromAddedDocumentContent);
    RUN_TEST(TestDocumentsCount);
    RUN_TEST(TestAddingDocument);
    RUN_TEST(TestExcludeDocumentsWithMinusWordsFromSearchResult);
    RUN_TEST(TestMatchingDocument);
    RUN_TEST(TestDocumentRelevance);
    RUN_TEST(TestDocumentRating);
    RUN_TEST(TestSearchResultWithComparator);
    RUN_TEST(TestSearchResultToDocumentStatus);
    RUN_TEST(TestRelevanceCalculating);
    RUN_TEST(TestProcessQueries);
}

int main() {
    TestSearchServer();

    return 0;
}
