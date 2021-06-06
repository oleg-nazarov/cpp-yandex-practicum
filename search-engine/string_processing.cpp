#include <string_view>
#include <utility>
#include <vector>

std::vector<std::string_view> SplitIntoWords(std::string_view text) {
    std::vector<std::string_view> words;

    while (true) {
        int64_t word_start = text.find_first_not_of(' ');

        if (word_start == text.npos) {
            break;
        }

        text.remove_prefix(word_start);

        int64_t word_end = text.find(' ');

        if (word_end == text.npos) {
            words.push_back(text.substr(0));
            break;
        }

        words.push_back(text.substr(0, word_end));
        text.remove_prefix(word_end + 1);
    }

    return words;
}
