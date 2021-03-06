#include "set_manipulators.hpp"

#include <stdexcept>
#include <limits>
#include <algorithm>
#include <iterator>
#include <cmath>


void split( const Work& src, 
            std::vector<Work>& parts, 
            const unsigned int num_parts)
{
  parts.clear();
  // ensure that there are actually as many parts as specified. We want empty parts to enable null messages
  parts.resize(num_parts);

  const size_t ext_size = (src.size() - (src.size() % num_parts)) + num_parts;
  const size_t chunk_size = ext_size / num_parts;

  size_t i = 0;
  size_t bucket = 0;
  for (auto it : src) {
    parts[bucket].add(it);
    if(++i % chunk_size == 0) bucket++;
  }
}


void merge(Work& dest, const Work& src)
{
  auto prev_branch_id = (*src.begin()).branch_id + 1;
  for (auto it : src)  {
    const auto branch_id = it.branch_id;
    if (prev_branch_id != branch_id) {
      dest[branch_id];
      dest[branch_id].insert( dest.at(branch_id).end(),
                              src.at(branch_id).begin(), 
                              src.at(branch_id).end()
                            );
    }
    prev_branch_id = branch_id;
  }
}

void merge(Timer<>& dest, const Timer<>& src)
{
  dest.insert(dest.end(), src.begin(), src.end());
}

void compute_and_set_lwr(Sample<Placement>& sample)
{
  #ifdef __OMP
  #pragma omp parallel for schedule(dynamic)
  #endif
  for (size_t i = 0; i < sample.size(); ++i) {
    auto &pq = sample[i];
    double total = 0.0;
    double max=-std::numeric_limits<double>::infinity();

    double entropy = 0.0;

    // find the maximum
    for (auto &p : pq) {
      if (p.likelihood() > max) {
        max = p.likelihood();
      }
    }

    // sum up the distances to the max
    for (auto &p : pq) {
      total += std::exp(p.likelihood() - max);
    }

    // normalize the distances
    for (auto &p : pq) {
      double lwr = std::exp(p.likelihood() - max) / total;
      p.lwr(lwr);
      // compute the shannon entropy of the query (in nats)
      entropy -= (lwr * std::log(lwr));
    }

    pq.entropy(entropy);
  }
}

static void sort_by_lwr(PQuery<Placement>& pq)
{
  sort(pq.begin(), pq.end(),
    [](const Placement &p_a, const Placement &p_b) -> bool {
      return p_a.lwr() > p_b.lwr();
    }
  );
}

void discard_bottom_x_percent(Sample<Placement>& sample, const double x)
{
  if (x < 0.0 || x > 1.0) {
    throw std::range_error{"x is not a percentage (outside of [0,1])"};
  }

  #ifdef __OMP
  #pragma omp parallel for schedule(dynamic)
  #endif
  for (size_t i = 0; i < sample.size(); ++i) {
    auto &pq = sample[i];
    auto num_keep = static_cast<int>(ceil((1.0 - x) * static_cast<double>(pq.size())));
    sort_by_lwr(pq);
    auto erase_iter = pq.begin();
    advance(erase_iter, num_keep);
    pq.erase(erase_iter, pq.end());
  }
}

void discard_by_support_threshold(Sample<Placement>& sample, 
                                  const double thresh, 
                                  const size_t min,
                                  const size_t max)
{
  if (thresh < 0.0 or thresh > 1.0){
    throw std::range_error{"thresh is not a valid likelihood weight ratio (outside of [0,1])"};
  }

  if (min < 1) {
    throw std::range_error{"Filter min cannot be smaller than 1!"};
  }

  #ifdef __OMP
  #pragma omp parallel for schedule(dynamic)
  #endif
  for (size_t i = 0; i < sample.size(); ++i) {
    auto &pq = sample[i];
    auto erase_iter = partition(
      pq.begin(), 
      pq.end(),
      [thresh](Placement &p) -> bool {
        return (p.lwr() > thresh);
      }
    );

    const auto num_kept = static_cast<size_t>(distance(pq.begin(), erase_iter));

    if ( num_kept < min ) {
      erase_iter = pq.begin() + min - 1;
    }

    if ( num_kept > max ) {
      erase_iter = pq.begin() + max - 1;
    }

    pq.erase(erase_iter, pq.end());
  }
}

void discard_by_accumulated_threshold(Sample<Placement>& sample, 
                                      const double thresh,
                                      const size_t min, 
                                      const size_t max)
{
  if (thresh < 0.0 || thresh > 1.0) {
    throw std::range_error{"thresh is not a valid likelihood weight ratio (outside of [0,1])"};
  }

  if (min < 1) {
    throw std::range_error{"Filter min cannot be smaller than 1!"};
  }
  
  if (min > max) {
    throw std::range_error{"Filter min cannot be smaller than max!"};
  }

  // accumulation and erasure phase
  #ifdef __OMP
  #pragma omp parallel for schedule(dynamic)
  #endif
  for (size_t i = 0; i < sample.size(); ++i) {
    auto &pq = sample[i];
    sort_by_lwr(pq);
    double sum = 0.0;

    size_t num_summed = 0;

    auto pq_iter = pq.begin();
    for (pq_iter = pq.begin(); num_summed < max and sum < thresh; ++pq_iter, ++num_summed) {
      // sum up until threshold is passed. if we abort before it is passed, we would have the possibility of
      // empty lists
      sum += pq_iter->lwr();
    }

    auto to_add = distance(pq_iter, pq.begin() + min - 1);
    if (to_add > 0) {
      advance(pq_iter, to_add);
    }

    pq.erase(pq_iter, pq.end());
  }
}

/* Find duplicate sequences in a MSA and collapse them into one entry that
  holds all respective headers */
void find_collapse_equal_sequences(MSA& msa)
{
  auto end_merge_range = msa.end();
  auto begin_merge_range = end_merge_range;
  for (auto target_iter = msa.begin(); target_iter != end_merge_range; target_iter++) {
    auto target = *target_iter;
    begin_merge_range = partition(target_iter + 1, end_merge_range,
    [&target](const Sequence& query)
    {
      return !(target == query);
    });
    // now all sequences in the msa that are equal to the "target" are at the end of the msa
    // whose first element is *begin_merge_range
    for (auto merge_iter = begin_merge_range; merge_iter != end_merge_range; merge_iter++) {
      (*target_iter).merge((*merge_iter));
    }

    end_merge_range = begin_merge_range;

  }
  // merging done: all redundant sequences are at the back of msa, starting at end_merge_range
  // cleanup:
  msa.erase(end_merge_range, msa.end());
}
