#include "matching/matching_engine.h"

std::size_t MatchingEngine::instrument_index(InstrumentType instrument) {
  return static_cast<std::size_t>(instrument);
}
