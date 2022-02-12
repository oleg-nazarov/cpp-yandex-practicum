#pragma once

#include <functional>
#include <iosfwd>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

// Позиция ячейки. Индексация с нуля.
struct Position {
    struct Hash {
        size_t operator()(const Position& pos) const {
            return std::hash<std::string>{}(pos.ToString());
        }
    };

    int row = 0;
    int col = 0;

    bool operator==(Position rhs) const;
    bool operator<(Position rhs) const;

    bool IsValid() const;
    std::string ToString() const;

    static Position FromString(std::string_view str);

    static const int MAX_ROWS = 16384;
    static const int MAX_COLS = 16384;
    static const Position NONE;

   private:
    static const size_t MAX_ROWS_LENGTH;
};

struct Size {
    int rows = 0;
    int cols = 0;

    bool operator==(Size rhs) const;
};

// Описывает ошибки, которые могут возникнуть при вычислении формулы.
class FormulaError {
   public:
    enum class Category {
        Ref,    // ссылка на ячейку с некорректной позицией
        Value,  // ячейка не может быть трактована как число
        Div0,   // в результате вычисления возникло деление на ноль
    };

    FormulaError(Category category) : category_(category) {}

    Category GetCategory() const {
        return category_;
    }

    bool operator==(FormulaError rhs) const {
        return category_ == rhs.category_;
    }

    std::string_view ToString() const {
        using namespace std::literals::string_view_literals;

        switch (category_) {
            case Category::Ref:
                return "#REF!"sv;
            case Category::Value:
                return "#VALUE!"sv;
            case Category::Div0:
                return "#DIV/0!"sv;
            default:
                return "#UNKNOWN ERROR!"sv;
        }
    }

   private:
    Category category_;
};

std::ostream& operator<<(std::ostream& output, FormulaError fe);

// Исключение, выбрасываемое при попытке задать синтаксически некорректную формулу
class FormulaException : public std::runtime_error {
   public:
    using std::runtime_error::runtime_error;
};

// Исключение, выбрасываемое при попытке передать в метод некорректную позицию
class InvalidPositionException : public std::out_of_range {
   public:
    using std::out_of_range::out_of_range;
};

// Исключение, выбрасываемое при попытке задать формулу, которая приводит к
// циклической зависимости между ячейками
class CircularDependencyException : public std::runtime_error {
   public:
    using std::runtime_error::runtime_error;
};

inline constexpr char FORMULA_SIGN = '=';
inline constexpr char ESCAPE_SIGN = '\'';

class CellInterface {
   public:
    // Либо текст ячейки, либо значение формулы, либо сообщение об ошибке из формулы
    using Value = std::variant<std::string, double, FormulaError>;

    virtual ~CellInterface() = default;

    // Задаёт содержимое ячейки. Если текст начинается со знака "=", то он  интерпретируется как формула.
    // Уточнения по записи формулы:
    // * Если текст содержит только символ "=" и больше ничего, то он не считается формулой
    // * Если текст начинается с символа "'" (апостроф), то при выводе значения ячейки методом
    // GetValue() он опускается. Можно использовать, если нужно начать текст со знака "=", но
    // чтобы он не интерпретировался как формула.
    virtual void Set(std::string text) = 0;

    // Возвращает внутренний текст ячейки, как если бы мы начали её
    // редактирование. В случае текстовой ячейки это её текст (возможно,
    // содержащий экранирующие символы). В случае формулы - её выражение.
    virtual std::string GetText() const = 0;

    // Возвращает видимое значение ячейки.
    // В случае текстовой ячейки это её текст (без экранирующих символов). В
    // случае формулы - числовое значение формулы или сообщение об ошибке.
    virtual Value GetValue() const = 0;

    // Возвращает список ячеек, которые непосредственно задействованы в данной
    // формуле. Список отсортирован по возрастанию и не содержит повторяющихся
    // ячеек. В случае текстовой ячейки список пуст.
    virtual std::vector<Position> GetReferencedCells() const = 0;
};

inline std::ostream& operator<<(std::ostream& output, const CellInterface::Value& value) {
    std::visit([&](const auto& x) { output << x; }, value);
    return output;
}

// Интерфейс таблицы
class SheetInterface {
   public:
    virtual ~SheetInterface() = default;

    // Задаёт содержимое ячейки. Если текст начинается со знака "=", то он  интерпретируется как
    // формула. Если задаётся синтаксически некорректная формула, то бросается исключение
    // FormulaException и значение ячейки не изменяется. Если задаётся формула, которая приводит к
    // циклической зависимости (в частности, если формула использует текущую ячейку), то бросается
    // исключение CircularDependencyException и значение ячейки не изменяется.
    // Уточнения по записи формулы:
    // * Если текст содержит только символ "=" и больше ничего, то он не считается формулой
    // * Если текст начинается с символа "'" (апостроф), то при выводе значения ячейки методом
    // GetValue() он опускается. Можно использовать, если нужно начать текст со знака "=", но чтобы
    // он не интерпретировался как формула.
    virtual void SetCell(Position pos, std::string text) = 0;

    // Возвращает значение ячейки. Если ячейка пуста, может вернуть nullptr.
    virtual const CellInterface* GetCell(Position pos) const = 0;
    virtual CellInterface* GetCell(Position pos) = 0;

    // Очищает ячейку.
    // Последующий вызов GetCell() для этой ячейки вернёт либо nullptr, либо объект с пустым текстом.
    virtual void ClearCell(Position pos) = 0;

    // Вычисляет размер области, которая участвует в печати.
    // Определяется как ограничивающий прямоугольник всех ячеек с непустым текстом.
    virtual Size GetPrintableSize() const = 0;

    // Выводит всю таблицу в переданный поток. Столбцы разделяются знаком табуляции. После каждой
    // строки выводится символ перевода строки. Для преобразования ячеек в строку используются методы
    // GetValue() или GetText() соответственно. Пустая ячейка представляется пустой строкой в любом случае.
    virtual void PrintValues(std::ostream& output) const = 0;
    virtual void PrintTexts(std::ostream& output) const = 0;
};

// Создаёт готовую к работе пустую таблицу.
std::unique_ptr<SheetInterface> CreateSheet();
