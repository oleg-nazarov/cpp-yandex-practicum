#include <string>
#include <vector>

std::vector<std::string> SplitIntoWords(const std::string& text) {
    using namespace std::string_literals;

    std::vector<std::string> words;
    std::string word;

    for (const char c : text) {
        if (c == ' ' && !word.empty()) {
            words.push_back(word);
            word = ""s;
        } else {
            word += c;
        }
    }

    words.push_back(word);

    return words;
}
