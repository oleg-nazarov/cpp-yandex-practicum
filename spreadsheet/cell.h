#pragma once

#include <functional>
#include <memory>
#include <string>
#include <unordered_set>
#include <variant>

#include "common.h"
#include "formula.h"

namespace detail {
class Impl;
}

class Sheet;

class Cell : public CellInterface {
   public:
    Cell(const SheetInterface& sheet, Position pos);
    ~Cell();

    void Set(std::string text) override;

    std::string GetText() const override;
    Value GetValue() const override;
    std::vector<Position> GetReferencedCells() const override;

   private:
    const SheetInterface& sheet_;
    Position pos_;
    std::unique_ptr<detail::Impl> impl_;
    std::unordered_set<Position, Position::Hash> dependent_cells_;
    std::unordered_set<Position, Position::Hash> referenced_cells_;

    void CheckCircularDependency(const std::vector<Position>& references) const;
    void CheckCircularDependencyHelper(const Position& this_cell_pos, const std::vector<Position>& references,
                                       std::unordered_set<Position, Position::Hash>& visited_cells) const;

    void ClearCaches(const std::unordered_set<Position, Position::Hash>& dependent);

    void EraseEdges();
    void AddEdges(const std::vector<Position>& references);
};
