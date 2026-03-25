#pragma once

#include <istream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

class CSVRow {
public:
    // Returns a non-owning view of one parsed field.
    std::string_view operator[](std::size_t index) const {
        if (index >= size()) {
            throw std::out_of_range("CSVRow column index out of range");
        }
        return std::string_view(&line_[field_positions_[index] + 1],
                                field_positions_[index + 1] -
                                    (field_positions_[index] + 1));
    }

    // Returns number of fields parsed in the current row.
    std::size_t size() const {
        return field_positions_.empty() ? 0 : field_positions_.size() - 1;
    }

    // Reads next line from stream and updates delimiter index positions.
    void readNextRow(std::istream& stream);

private:
    std::string line_;
    std::vector<int> field_positions_;
};

// Stream extraction for CSVRow.
std::istream& operator>>(std::istream& stream, CSVRow& row);

class CSVIterator {
public:
    using iterator_category = std::input_iterator_tag;
    using value_type = CSVRow;
    using difference_type = std::ptrdiff_t;
    using pointer = CSVRow*;
    using reference = CSVRow&;

    // Constructs begin iterator and advances to first row.
    explicit CSVIterator(std::istream& stream);
    // Constructs end sentinel iterator.
    CSVIterator();

    // Advances iterator to next row.
    CSVIterator& operator++();
    CSVIterator operator++(int);

    // Access current row.
    const CSVRow& operator*() const;
    const CSVRow* operator->() const;

    bool operator==(const CSVIterator& rhs) const;
    bool operator!=(const CSVIterator& rhs) const;

private:
    std::istream* stream_;
    CSVRow row_;
};

class CSVRange {
public:
    // Creates range facade over stream.
    explicit CSVRange(std::istream& stream);
    // Begin and end iterators for range-for loops.
    CSVIterator begin() const;
    static CSVIterator end();

private:
    std::istream& stream_;
};
