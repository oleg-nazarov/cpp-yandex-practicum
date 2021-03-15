#include "remove_duplicates.h"

#include <algorithm>
#include <iterator>
#include <map>
#include <set>
#include <string>

#include "search_server.h"

void RemoveDuplicates(SearchServer& search_server) {
    using namespace std;

    set<int> ids_to_remove;
    map<set<string>, int> document_words_to_id;

    // gathering ids
    for (const int document_id : search_server) {
        const map<string, double>& word_freqs = search_server.GetWordFrequencies(document_id);

        set<string> document_words;
        transform(
            word_freqs.begin(), word_freqs.end(),
            inserter(document_words, document_words.begin()),
            [](auto& key_value) {
                return key_value.first;
            });

        if (document_words_to_id.count(document_words)) {
            int checked_id = document_words_to_id[document_words];

            ids_to_remove.insert(checked_id > document_id ? checked_id : document_id);

            // "if" is slower than "ternary operator"
            document_words_to_id[document_words] = checked_id > document_id ? document_id : checked_id;
        } else {
            document_words_to_id[document_words] = document_id;
        }
    }

    // removing
    for (int id : ids_to_remove) {
        std::cout << DUPLICATE_ID_INFO_TEXT << ' ' << id << std::endl;
        search_server.RemoveDocument(id);
    }
}
