#include <cctype>
#include <cmath>
#include <iterator>
#include <regex>
#include <sstream>
#include <string>
#include <string_view>
#include <tuple>

#include "common.h"

namespace {

const int LETTERS = 26;

const std::regex COL_ROW_SYMBOLS_RE(R"(^([A-Z]+)([1-9]\d*)$)");

}  // namespace

bool Position::operator==(const Position rhs) const {
    return std::tie(this->row, this->col) == std::tie(rhs.row, rhs.col);
}

bool Position::operator<(const Position rhs) const {
    return std::tie(this->row, this->col) < std::tie(rhs.row, rhs.col);
}

bool Position::IsValid() const {
    return (row >= 0 && row < MAX_ROWS) && (col >= 0 && col < MAX_COLS);
}

std::string Position::ToString() const {
    using namespace std::literals;
    if (!IsValid()) {
        return ""s;
    }

    std::string col_row_s = "";

    // col
    int temp = col;
    while (true) {
        char ch = temp % LETTERS + 'A';
        col_row_s.insert(0, 1, ch);

        if (temp / LETTERS == 0) {
            break;
        }

        temp = temp / LETTERS - 1;
    }

    // row
    col_row_s.append(std::to_string(row + 1));

    return col_row_s;
}

Position Position::FromString(std::string_view str_sv) {
    std::string str(str_sv);
    std::smatch sm;
    std::regex_search(str, sm, COL_ROW_SYMBOLS_RE);

    if (sm.size() == 0u) {
        return Position::NONE;
    }

    std::string col_s = std::move(sm[1].str());
    std::string row_s = std::move(sm[2].str());

    int col = 0;
    for (size_t i = 0; i < col_s.size(); ++i) {
        col += (col_s[i] - 'A' + 1) * std::pow(LETTERS, col_s.size() - 1 - i);
    }

    if (row_s.size() > MAX_ROWS_LENGTH) {
        return Position::NONE;
    }

    Position pos{std::stoi(row_s) - 1, col - 1};

    if (!pos.IsValid()) {
        return Position::NONE;
    }

    return pos;
}

const Position Position::NONE = {-1, -1};

const size_t Position::MAX_ROWS_LENGTH = std::to_string(Position::MAX_ROWS).size();

bool Size::operator==(Size rhs) const {
    return cols == rhs.cols && rows == rhs.rows;
}
