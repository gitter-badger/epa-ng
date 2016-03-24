#pragma once

#include <vector>

#include "Sequence.hpp"
#include "Placement.hpp"

class PQuery {
public:
  typedef Placement                                       value_type;
  typedef typename std::vector<value_type>::iterator        iterator;
  typedef typename std::vector<value_type>::const_iterator  const_iterator;

  PQuery() : sequence_id_(0) {};
  PQuery (const unsigned int seq_id, const unsigned int size)
    : placements_(size), sequence_id_(seq_id) {};
  PQuery (const unsigned int seq_id)
    : sequence_id_(seq_id) {};
  ~PQuery() = default;

  // needs to be in the header
  template<typename ...Args> void emplace_back(Args && ...args)
  {
    placements_.emplace_back(std::forward<Args>(args)...);
  }

  // member access
  Placement& back() {return placements_.back();};
  unsigned int sequence_id() const {return sequence_id_;};
  unsigned int size() const {return placements_.size();};
  void erase(iterator begin, iterator end) {placements_.erase(begin, end);};

  // Iterator Compatibility
  iterator begin() {return placements_.begin();};
  iterator end() {return placements_.end();};
  const_iterator begin() const {return placements_.cbegin();};
  const_iterator end() const {return placements_.cend();};
  const_iterator cbegin() {return placements_.cbegin();};
  const_iterator cend() {return placements_.cend();};

  // Operator overloads
  Placement& operator[] (const unsigned int index)
  {
    return placements_[index];
  }
private:
  std::vector<Placement> placements_;
  const unsigned int sequence_id_;
};
