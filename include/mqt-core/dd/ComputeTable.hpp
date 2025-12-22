/*
 * Copyright (c) 2023 - 2025 Chair for Design Automation, TUM
 * Copyright (c) 2025 Munich Quantum Software Company GmbH
 * All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 *
 * Licensed under the MIT License
 */

/**
 * @file ComputeTable.hpp
 * @brief Data structure for caching computed results of binary operations
 */

#pragma once

#include "dd/Node.hpp"
#include "dd/statistics/TableStatistics.hpp"
#include "ir/Definitions.hpp"

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <iostream>
#include <mutex>
#include <stdexcept>
#include <thread>
#include <vector>

namespace dd {

/**
 * @brief Data structure for caching computed results of binary operations
 * @tparam LeftOperandType type of the operation's left operand
 * @tparam RightOperandType type of the operation's right operand
 * @tparam ResultType type of the operation's result
 */
template <class LeftOperandType, class RightOperandType, class ResultType>
class ComputeTable {
public:
  /// Default number of buckets for the compute table
  static constexpr std::size_t DEFAULT_NUM_BUCKETS = 16384U;

  enum class BucketState : std::uint8_t { Empty = 0, Writing, Ready };

  /**
   * Default constructor
   * @param numBuckets Number of hash table buckets. Must be a power of two.
   */
  explicit ComputeTable(const size_t numBuckets = DEFAULT_NUM_BUCKETS) {
    // numBuckets must be a power of two
    if ((numBuckets & (numBuckets - 1)) != 0) {
      throw std::invalid_argument("Number of buckets must be a power of two.");
    }
    stats.entrySize = sizeof(Entry);
    stats.numBuckets = numBuckets;
    table = std::vector<Entry>(numBuckets);
    states = std::vector<std::atomic<BucketState>>(numBuckets);
    for (auto& state : states) {
      state.store(BucketState::Empty, std::memory_order_relaxed);
    }
  }

  /**
   * @brief An entry in the compute table
   * @details A triple consisting of the left operand, the right operand, and
   * the result of a binary operation.
   */
  struct Entry {
    LeftOperandType leftOperand;
    RightOperandType rightOperand;
    ResultType result;
  };

  /**
   * @brief Compute the hash value for a given pair of operands
   * @param leftOperand The left operand
   * @param rightOperand The right operand
   * @return The hash value
   */
  [[nodiscard]] std::size_t hash(const LeftOperandType& leftOperand,
                                 const RightOperandType& rightOperand) const {
    auto h1 = std::hash<LeftOperandType>{}(leftOperand);
    if constexpr (std::is_same_v<LeftOperandType, dNode*>) {
      if (!dNode::isTerminal(leftOperand)) {
        h1 = qc::combineHash(
            h1, dd::dNode::getDensityMatrixTempFlags(leftOperand->flags));
      }
    }
    auto h2 = std::hash<RightOperandType>{}(rightOperand);
    if constexpr (std::is_same_v<RightOperandType, dNode*>) {
      if (!dNode::isTerminal(rightOperand)) {
        h2 = qc::combineHash(
            h2, dd::dNode::getDensityMatrixTempFlags(rightOperand->flags));
      }
    }
    const auto hash = qc::combineHash(h1, h2);
    const auto mask = stats.numBuckets - 1;
    return hash & mask;
  }

  /// Get a reference to the underlying table
  [[nodiscard]] const auto& getTable() const { return table; }

  /// Get a reference to the statistics
  [[nodiscard]] const auto& getStats() const noexcept { return stats; }

  /**
   * @brief Insert a new entry into the compute table
   * @details Any existing entry for the resulting hash value will be replaced.
   * @param leftOperand The left operand
   * @param rightOperand The right operand
   * @param result The result of the operation
   */
  void insert(const LeftOperandType& leftOperand,
              const RightOperandType& rightOperand, const ResultType& result) {
    const auto key = hash(leftOperand, rightOperand);

    bool collision = false;

    while (true) {
      auto expected = BucketState::Empty;
      if (states[key].compare_exchange_strong(expected, BucketState::Writing,
                                               std::memory_order_acq_rel)) {
        table[key] = {leftOperand, rightOperand, result};
        states[key].store(BucketState::Ready, std::memory_order_release);
        break;
      }

      expected = BucketState::Ready;
      if (states[key].compare_exchange_strong(expected, BucketState::Writing,
                                               std::memory_order_acq_rel)) {
        collision = true;
        table[key] = {leftOperand, rightOperand, result};
        states[key].store(BucketState::Ready, std::memory_order_release);
        break;
      }

      std::this_thread::yield();
    }

    std::scoped_lock statsLock(statsMutex);
    if (collision) {
      ++stats.collisions;
    } else {
      stats.trackInsert();
    }
  }

  /**
   * @brief Look up a result in the compute table
   * @param leftOperand The left operand
   * @param rightOperand The right operand
   * @param useDensityMatrix Whether a density matrix is expected
   * @return A pointer to the result if it is found, otherwise nullptr.
   */
  ResultType* lookup(const LeftOperandType& leftOperand,
                     const RightOperandType& rightOperand,
                     [[maybe_unused]] const bool useDensityMatrix = false) {
    ResultType* result = nullptr;
    {
      std::scoped_lock statsLock(statsMutex);
      ++stats.lookups;
    }
    const auto key = hash(leftOperand, rightOperand);
    if (states[key].load(std::memory_order_acquire) != BucketState::Ready) {
      return result;
    }

    auto& entry = table[key];
    if (states[key].load(std::memory_order_acquire) != BucketState::Ready) {
      return result;
    }
    if (entry.leftOperand != leftOperand) {
      return result;
    }
    if (entry.rightOperand != rightOperand) {
      return result;
    }

    if constexpr (std::is_same_v<RightOperandType, dNode*> ||
                  std::is_same_v<RightOperandType, dCachedEdge>) {
      // Since density matrices are reduced representations of matrices, a
      // density matrix may not be returned when a matrix is required and vice
      // versa
      if (!dNode::isTerminal(entry.result.p) &&
          dNode::isDensityMatrixNode(entry.result.p->flags) !=
              useDensityMatrix) {
        return result;
      }
    }
    {
      std::scoped_lock statsLock(statsMutex);
      ++stats.hits;
    }
    return &entry.result;
  }

  /**
   * @brief Clear the compute table
   * @details Sets all entries to invalid.
   */
  void clear() {
    for (auto& state : states) {
      state.store(BucketState::Empty, std::memory_order_relaxed);
    }
  }

  /**
   * @brief Print the statistics of the compute table
   * @param os The output stream to print to
   * @return The output stream
   */
  std::ostream& printStatistics(std::ostream& os = std::cout) const {
    return os << stats;
  }

private:
  /// The actual table storing the entries
  std::vector<Entry> table;
  /// Per-bucket state to allow lock-free access
  std::vector<std::atomic<BucketState>> states;
  /// Protects statistics updates
  mutable std::mutex statsMutex;
  /// Statistics of the compute table
  TableStatistics stats{};
};
} // namespace dd
