#pragma once

#include <vector>
#include <limits>

#include "Sample.hpp"
#include "Timer.hpp"
#include "Range.hpp"
#include "MSA.hpp"
#include "Work.hpp"

void split( const Sample& src, 
            std::vector<Sample>& parts, 
            const unsigned int num_parts);
void split( const Work& source, 
            std::vector<Work>& parts, 
            const unsigned int num_parts);
void merge(Sample& dest, const Sample &src);
void merge(Work& dest, const Work& src);
void merge(Timer<>& dest, const Timer<>& src);
void compute_and_set_lwr(Sample& sample);
void discard_bottom_x_percent(Sample& sample, const double x);
void discard_by_support_threshold(Sample& sample, 
                                  const double thresh, 
                                  const unsigned int min=1, 
                                  const unsigned int max=std::numeric_limits<unsigned int>::max());
void discard_by_accumulated_threshold(Sample& sample, 
                                      const double thresh,
                                      const unsigned int min=1, 
                                      const unsigned int max=std::numeric_limits<unsigned int>::max());
Range superset(Range a, Range b);
Range get_valid_range(std::string sequence);
void find_collapse_equal_sequences(MSA& msa);

/**
 * Splits a <src> into <num_parts> number of equally sized <parts>.
 * <parts> is expected to be empty.
 *
 * REQUIRES T TO BE:
 *   Iterable
 *   has size()
 *   has insert()
 * 
 * @param src    Container to split
 * @param parts     resulting parts vector
 * @param num_parts number of parts
 */
template <class T>
void split( const T& src, 
            std::vector<T>& parts, 
            const unsigned int num_parts)
{
  parts.clear();
  unsigned int chunk_size = ceil(src.size() / static_cast<double>(num_parts));
  auto move_begin = src.begin();

  while (move_begin < src.end()) {
    auto move_end = std::min(move_begin + chunk_size, src.end());
    parts.emplace_back();
    parts.back().insert(move_begin, move_end);
    move_begin = move_end;
  }
}

template <class T>
void merge(T& dest, std::vector<T>& parts)
{
  for (const auto& p : parts) {
    merge(dest, p);
  }
}

template <class T>
void merge(std::vector<T>& dest, const std::vector<T>& parts)
{
  dest.insert( std::end(dest), std::begin(parts), std::end(parts) );
}
