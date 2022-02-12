#include "formula.h"

#include <algorithm>
#include <cassert>
#include <cctype>
#include <forward_list>
#include <iterator>
#include <optional>
#include <sstream>

#include "FormulaAST.h"

using namespace std::literals;

std::ostream& operator<<(std::ostream& output, FormulaError fe) {
    return output << fe.ToString();
}

namespace {
class Formula : public FormulaInterface {
   public:
    explicit Formula(std::string expression) : ast_(ParseFormulaAST(expression)) {
        std::forward_list<Position> cells_copy = ast_.GetCells();
        cells_copy.unique();
        cells_ = {std::move_iterator(cells_copy.begin()), std::move_iterator(cells_copy.end())};
    }

    Value Evaluate(const SheetInterface& sheet) const override {
        if (!cache_.has_value()) {
            try {
                cache_ = ast_.Execute(sheet);
            } catch (const FormulaError& e) {
                cache_ = e;
            }
        }

        return *cache_;
    }

    std::string GetExpression() const override {
        std::ostringstream oss;
        ast_.PrintFormula(oss);

        return oss.str();
    }

    std::vector<Position> GetReferencedCells() const {
        return cells_;
    }

    bool HasCache() const override {
        return cache_.has_value();
    }
    void ClearCache() override {
        cache_.reset();
    }

   private:
    FormulaAST ast_;
    std::vector<Position> cells_;
    mutable std::optional<Value> cache_;
};
}  // namespace

std::unique_ptr<FormulaInterface> ParseFormula(std::string expression) {
    return std::make_unique<Formula>(std::move(expression));
}
