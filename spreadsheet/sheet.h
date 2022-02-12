#pragma once

#include <functional>
#include <memory>
#include <vector>

#include "cell.h"
#include "common.h"

namespace detail {
enum class TextOrValue {
    TEXT,
    VALUE,
};
}

class Sheet : public SheetInterface {
   public:
    Sheet();
    ~Sheet();

    void SetCell(Position pos, std::string text) override;

    const CellInterface* GetCell(Position pos) const override;
    CellInterface* GetCell(Position pos) override;

    void ClearCell(Position pos) override;

    Size GetPrintableSize() const override;

    void PrintTexts(std::ostream& output) const override;
    void PrintValues(std::ostream& output) const override;

   private:
    std::vector<std::vector<std::unique_ptr<CellInterface>>> cells_;
    Size size_;

    void ExtendCellsIfNeed(const Position& pos);

    void PrintTextOrValue(std::ostream& output, detail::TextOrValue type) const;

    void ThrowInvalidPosition(const Position& pos) const;

    void UpdateSize();
};
