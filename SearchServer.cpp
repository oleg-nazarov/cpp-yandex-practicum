#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <map>
#include <set>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

using namespace std;

const int MAX_RESULT_DOCUMENT_COUNT = 5;
const double EPS = 1e-6;

vector<string> SplitIntoWords(const string& text) {
    vector<string> words;
    string word;

    for (const char c : text) {
        if (c == ' ' && !word.empty()) {
            words.push_back(word);
            word = "";
        } else {
            word += c;
        }
    }

    words.push_back(word);

    return words;
}

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

    Document() : id(0), relevance(0.0), rating(0), status(DocumentStatus::ACTUAL){};
    Document(int id, double relevance, int rating) : id(id), relevance(relevance), rating(rating), status(DocumentStatus::ACTUAL){};
    Document(int id, double relevance, int rating, DocumentStatus status) : id(id), relevance(relevance), rating(rating), status(status){};
    Document(int rating, DocumentStatus status) : rating(rating), status(status){};
};

class SearchServer {
   public:
    SearchServer() = default;

    SearchServer(const string& text) : SearchServer(SplitIntoWords(text)) {}

    template <typename Container>
    SearchServer(const Container& stop_words) {
        for (const string& word : stop_words) {
            if (word.empty()) {
                continue;
            }

            if (HasSpecialCharacters(word)) {
                throw invalid_argument("Step words mustn't include special characters"s);
            }

            stop_words_.insert(word);
        }
    }

    void AddDocument(int document_id, const string& document, DocumentStatus status, const vector<int>& ratings) {
        if (document_id < 0) {
            throw invalid_argument("Document id mustn't be negative"s);
        } else if (document_ratings_status_.count(document_id)) {
            throw invalid_argument("Document with such id has already added"s);
        } else if (HasSpecialCharacters(document)) {
            throw invalid_argument("Document mustn't include special characters"s);
        }

        const vector<string> words = SplitIntoWordsNoStop(document);
        const double inv_word_count = 1.0 / words.size();
        for (const string& word : words) {
            word_to_document_freqs_[word][document_id] += inv_word_count;
        }
        document_ratings_status_[document_id] = Document(ComputeAverageRating(ratings), status);
    }

    vector<Document> FindTopDocuments(const string& raw_query, DocumentStatus status = DocumentStatus::ACTUAL) const {
        return FindTopDocuments(raw_query, [status](int document_id, DocumentStatus stat, int rating) {
            return stat == status;
        });
    }

    template <typename Comparator>
    vector<Document> FindTopDocuments(const string& raw_query, Comparator comparator) const {
        const Query query = ParseQuery(raw_query);
        auto matched_documents = FindAllDocuments(query, comparator);

        sort(
            matched_documents.begin(),
            matched_documents.end(),
            [](const Document& lhs, const Document& rhs) {
                if (abs(lhs.relevance - rhs.relevance) < EPS) {
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

    tuple<vector<string>, DocumentStatus> MatchDocument(const string& raw_query, int document_id) const {
        const Query query = ParseQuery(raw_query);
        vector<string> match_words;

        for (const string& word : query.plus_words) {
            if (word_to_document_freqs_.at(word).count(document_id)) {
                match_words.push_back(word);
            }
        }

        for (const string& word : query.minus_words) {
            if (word_to_document_freqs_.at(word).count(document_id)) {
                match_words.clear();
            }
        }

        return tuple(match_words, document_ratings_status_.at(document_id).status);
    }

    int GetDocumentCount() const {
        return document_ratings_status_.size();
    }

    int GetDocumentId(int order) const {
        if (order < 0 || order > static_cast<int>(document_id_by_order_.size()) - 1) {
            throw out_of_range("Index of document is out of range"s);
        }

        return document_id_by_order_[order];
    }

   private:
    set<string> stop_words_;
    map<string, map<int, double>> word_to_document_freqs_;
    map<int, Document> document_ratings_status_;
    vector<int> document_id_by_order_;

    static bool HasSpecialCharacters(const string& word) {
        return any_of(word.begin(), word.end(), [](const char c) {
            return c >= '\0' && c < ' ';
        });
    }

    bool IsStopWord(const string& word) const {
        return stop_words_.count(word) > 0;
    }

    vector<string> SplitIntoWordsNoStop(const string& text) const {
        vector<string> words;

        for (const string& word : SplitIntoWords(text)) {
            if (!IsStopWord(word)) {
                words.push_back(word);
            }
        }

        return words;
    }

    static int ComputeAverageRating(const vector<int>& ratings) {
        if (ratings.empty()) {
            return 0;
        }

        int rating_sum = 0;

        for (const int rating : ratings) {
            rating_sum += rating;
        }

        return rating_sum / static_cast<int>(ratings.size());
    }

    struct QueryWord {
        string data;
        bool is_minus;
        bool is_stop;
    };

    QueryWord ParseQueryWord(string text) const {
        bool is_minus = false;

        // Word shouldn't be empty
        if (text[0] == '-') {
            if (text.size() == 1u || text[1] == '-' || text[1] == ' ') {
                throw invalid_argument("There must by another word after \"minus\" sign"s);
            }

            is_minus = true;
            text = text.substr(1);
        }

        if (HasSpecialCharacters(text)) {
            throw invalid_argument("Text mustn't include special characters"s);
        }

        return {
            text,
            is_minus,
            IsStopWord(text),
        };
    }

    struct Query {
        set<string> plus_words;
        set<string> minus_words;
    };

    Query ParseQuery(const string& text) const {
        Query query;

        for (const string& word : SplitIntoWords(text)) {
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
    double ComputeWordInverseDocumentFreq(const string& word) const {
        return log(document_ratings_status_.size() * 1.0 / word_to_document_freqs_.at(word).size());
    }

    template <typename Comparator>
    vector<Document> FindAllDocuments(const Query& query, Comparator comparator) const {
        map<int, double> document_to_relevance;

        for (const string& word : query.plus_words) {
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

        for (const string& word : query.minus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }

            for (const auto [document_id, _] : word_to_document_freqs_.at(word)) {
                document_to_relevance.erase(document_id);
            }
        }

        vector<Document> matched_documents;
        for (const auto [document_id, relevance] : document_to_relevance) {
            Document document_data = document_ratings_status_.at(document_id);

            matched_documents.push_back({ document_id,
                                          relevance,
                                          document_data.rating,
                                          document_data.status });
        }
        return matched_documents;
    }
};

void PrintMatchDocumentResult(int document_id, const vector<string>& words, DocumentStatus status) {
    cout << "{ "s
         << "document_id = "s << document_id << ", "s
         << "status = "s << static_cast<int>(status) << ", "s
         << "words ="s;
    for (const string& word : words) {
        cout << ' ' << word;
    }
    cout << "}"s << endl;
}

void PrintDocument(const Document& document) {
    cout << "{ "s
         << "document_id = "s << document.id << ", "s
         << "relevance = "s << document.relevance << ", "s
         << "rating = "s << document.rating << " }"s << endl;
}

// макросы ASSERT, ASSERT_EQUAL, ASSERT_EQUAL_HINT, ASSERT_HINT и RUN_TEST
#define RUN_TEST(func) func(); cerr << #func << ": OK." << endl;

template <typename A, typename B>
void AssertEqualImpl(const A a, const B b, const string& a_str, const string& b_str,
                     const string& file_name, const int line_number, const string& func_name,
                     const string& hint) {
    if (a != b) {
        cerr << boolalpha;
        cerr << file_name << "(" << line_number << "): " << func_name << ": ";
        cerr << "ASSERT_EQUAL(" << a_str << ", " << b_str << ") failed: ";
        cerr << a << " != " << b << ".";

        if (!hint.empty()) {
            cerr << " Hint: " << hint;
        }

        abort();
    }
}
#define ASSERT_EQUAL(a, b) AssertEqualImpl(a, b, #a, #b, __FILE__, __LINE__, __FUNCTION__, "")
#define ASSERT_EQUAL_HINT(a, b, hint) AssertEqualImpl(a, b, #a, #b, __FILE__, __LINE__, __FUNCTION__, hint)

void AssertImpl(const bool value, const string& value_str, const string& file_name,
                const int line_number, const string& func_name, const string& hint) {
    if (!value) {
        cerr << file_name << "(" << line_number << "): " << func_name << ": ";
        cerr << "ASSERT(" << value_str << ") failed."s;

        if (!hint.empty()) {
            cerr << " Hint: " << hint;
        }

        abort();
    }
}
#define ASSERT(value) AssertImpl(value, #value, __FILE__, __LINE__, __FUNCTION__, "")
#define ASSERT_HINT(value, hint) AssertImpl(value, #value, __FILE__, __LINE__, __FUNCTION__, hint)

ostream& operator<<(ostream& os, DocumentStatus doc) {
    const char* status = 0;
#define PROCESS_DOC(p) case (p): status = #p; break;
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

// -------- Начало модульных тестов поисковой системы ----------
void TestExcludeStopWordsFromAddedDocumentContent() {
    const int doc_id = 42;
    const string content = "cat in the city";
    const vector<int> ratings = {1, 2, 3};

    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments("in"s);
        ASSERT_EQUAL(found_docs.size(), 1u);
        const Document& doc0 = found_docs[0];
        ASSERT_EQUAL(doc0.id, doc_id);
    }

    {
        SearchServer server("in the"s);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        ASSERT_HINT(server.FindTopDocuments("in"s).empty(), "Stop words must be excluded from documents"s);
    }
}

void TestDocumentsCount() {
    const int doc_id_1 = 42;
    const int doc_id_2 = 43;
    const string content = "cat in the city";
    const vector<int> ratings = {1, 2, 3};

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
    const string content = "cat in the city";
    const vector<int> ratings = {1, 2, 3};

    // document with existing id has not been added
    {
        bool got_exception = false;

        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);

        try {
            server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        } catch (const invalid_argument& error) {
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
        } catch (const invalid_argument& error) {
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
        } catch (const invalid_argument& error) {
        }
        const size_t doc_count_2 = server.GetDocumentCount();
        ASSERT_EQUAL(doc_count_1, doc_count_2);
    }
}

void TestExcludeDocumentsWithMinusWordsFromSearchResult() {
    const int doc_id = 42;
    const string content = "cat in the city";
    const vector<int> ratings = {1, 2, 3};

    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments("cat -city"s);
        ASSERT_EQUAL(found_docs.size(), 0u);
    }
}

void TestMatchDocument() {
    const int doc_id = 42;
    const string content = "cat in the city";
    const vector<int> ratings = {1, 2, 3};

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
        ASSERT(get<0>(matched_words).empty());
    }
}

void TestDocumentRelevance() {
    const int doc_id_city = 42;
    const string content_city = "cat in the city";
    const vector<int> ratings = {1, 2, 3};
    const int doc_id_village = 43;
    const string content_village = "cat in the village";

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
    const string content = "cat in the city";
    const vector<int> ratings = {3, 30, 300};

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
    const string content = "cat in the city";
    const vector<int> ratings = {1, 2, 3};

    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::IRRELEVANT, {});
        const auto found_docs = server.FindTopDocuments("cat"s);
        ASSERT_EQUAL(found_docs.size(), 0u);

        const auto found_docs_irrelevant = server.FindTopDocuments(
            "cat",
            [](int document_id, DocumentStatus status, int rating) {
                return status == DocumentStatus::IRRELEVANT;
            });
        ASSERT_EQUAL(found_docs_irrelevant[0].id, doc_id);
    }
}

void TestSearchResultToDocumentStatus() {
    const string content = "cat in the city";
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
    cout << fixed << setprecision(6);

    {
        const int doc_id_1 = 42;
        const string content_1 = "cat other other";
        const int doc_id_2 = 43;
        const string content_2 = "other";
        const int doc_id_3 = 44;
        const string content_3 = "cat";
        const vector<int> ratings = {1, 2, 3};

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
        const string content_1 = "cat";
        const int doc_id_2 = 43;
        const string content_2 = "other";
        const int doc_id_3 = 44;
        const string content_3 = "other";
        const vector<int> ratings = {1, 2, 3};

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

// Функция TestSearchServer является точкой входа для запуска тестов
void TestSearchServer() {
    RUN_TEST(TestExcludeStopWordsFromAddedDocumentContent);
    RUN_TEST(TestDocumentsCount);
    RUN_TEST(TestAddingDocument);
    RUN_TEST(TestExcludeDocumentsWithMinusWordsFromSearchResult);
    RUN_TEST(TestMatchDocument);
    RUN_TEST(TestDocumentRelevance);
    RUN_TEST(TestDocumentRating);
    RUN_TEST(TestSearchResultWithComparator);
    RUN_TEST(TestSearchResultToDocumentStatus);
    RUN_TEST(TestRelevanceCalculating);
}

// --------- Окончание модульных тестов поисковой системы -----------

int main() {
    TestSearchServer();

    SearchServer search_server("и в на"s);

    search_server.AddDocument(0, "белый кот и модный ошейник"s, DocumentStatus::ACTUAL, { 8, -3 });
    search_server.AddDocument(1, "пушистый кот пушистый хвост"s, DocumentStatus::ACTUAL, { 7, 2, 7 });
    search_server.AddDocument(2, "ухоженный пёс выразительные глаза"s, DocumentStatus::ACTUAL, { 5, -12, 2, 1 });
    search_server.AddDocument(3, "ухоженный скворец евгений"s, DocumentStatus::BANNED, { 9 });

    cout << "ACTUAL by default:"s << endl;

    for (const Document& document : search_server.FindTopDocuments("пушистый ухоженный кот"s)) {
        PrintDocument(document);
    }

    cout << "ACTUAL:"s << endl;
    for (const Document &document : search_server.FindTopDocuments(
             "пушистый ухоженный кот"s,
             [](int document_id, DocumentStatus status, int rating) {
                 return status == DocumentStatus::ACTUAL;
             })) {
        PrintDocument(document);
    }

    cout << "Even ids:"s << endl;
    for (const Document &document : search_server.FindTopDocuments(
             "пушистый ухоженный кот"s, [](int document_id, DocumentStatus status, int rating) {
                 return document_id % 2 == 0;
             })) {
        PrintDocument(document);
    }

    return 0;
}
