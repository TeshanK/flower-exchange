#include "io/csv_reader.h"

#include <string>

void CSVRow::readNextRow(std::istream &stream) {
  if (line_.capacity() < 256) {
    line_.reserve(256);
  }
  std::getline(stream, line_);

  if (!line_.empty() && line_.back() == '\r') {
    line_.pop_back();
  }

  field_positions_.clear();
  if (field_positions_.capacity() < 16) {
    field_positions_.reserve(16);
  }
  field_positions_.push_back(-1);

  for (std::size_t pos = 0; pos < line_.size(); ++pos) {
    if (line_[pos] == ',') {
      field_positions_.push_back(static_cast<int>(pos));
    }
  }
  field_positions_.push_back(static_cast<int>(line_.size()));
}

std::istream &operator>>(std::istream &stream, CSVRow &row) {
  row.readNextRow(stream);
  return stream;
}

CSVIterator::CSVIterator(std::istream &stream)
    : stream_(stream.good() ? &stream : nullptr) {
  ++(*this);
}

CSVIterator::CSVIterator() : stream_(nullptr) {}

CSVIterator &CSVIterator::operator++() {
  if (stream_) {
    if (!((*stream_) >> row_)) {
      stream_ = nullptr;
    }
  }
  return *this;
}

CSVIterator CSVIterator::operator++(int) {
  CSVIterator tmp(*this);
  ++(*this);
  return tmp;
}

const CSVRow &CSVIterator::operator*() const { return row_; }

const CSVRow *CSVIterator::operator->() const { return &row_; }

bool CSVIterator::operator==(const CSVIterator &rhs) const {
  return ((this == &rhs) || ((stream_ == nullptr) && (rhs.stream_ == nullptr)));
}

bool CSVIterator::operator!=(const CSVIterator &rhs) const {
  return !(*this == rhs);
}

CSVRange::CSVRange(std::istream &stream) : stream_(stream) {}

CSVIterator CSVRange::begin() const { return CSVIterator(stream_); }

CSVIterator CSVRange::end() { return {}; }
