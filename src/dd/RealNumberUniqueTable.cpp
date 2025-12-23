/*
 * Copyright (c) 2023 - 2025 Chair for Design Automation, TUM
 * Copyright (c) 2025 Munich Quantum Software Company GmbH
 * All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 *
 * Licensed under the MIT License
 */

/* Additional modifications by Jul Jon General
 * UP Diliman - Department of Computer Science
 * Logic & Computability Laboratory
 * 2025
 */

#include "dd/RealNumberUniqueTable.hpp"

#include "dd/DDDefinitions.hpp"
#include "dd/MemoryManager.hpp"
#include "dd/RealNumber.hpp"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <limits>
#include <ostream>

#ifdef _OPENMP
#include <omp.h>
#endif

namespace dd {

RealNumberUniqueTable::RealNumberUniqueTable(MemoryManager& manager,
                                             const std::size_t initialGCLim)
    : memoryManager(&manager), initialGCLimit(initialGCLim) {
  stats.entrySize = sizeof(Bucket);
  stats.numBuckets = NBUCKET;
  for (const auto& ival : immortals::get()) {
    RealNumber::immortalize(lookupNonNegative(ival));
  }
}

std::int64_t RealNumberUniqueTable::hash(const fp val) noexcept {
  static constexpr std::int64_t MASK = NBUCKET - 1;
  assert(val >= 0);
  const auto key = static_cast<std::int64_t>(std::nearbyint(val * MASK));
  return std::min<std::int64_t>(key, MASK);
}

RealNumber* RealNumberUniqueTable::lookup(const fp val) {
  // if the value is close enough to zero, return the zero entry (avoiding -0.0)
  if (RealNumber::approximatelyZero(val)) {
    return &constants::zero;
  }
  if (const auto sign = std::signbit(val); sign) {
    return RealNumber::getNegativePointer(lookupNonNegative(std::abs(val)));
  }
  return lookupNonNegative(val);
}

RealNumber* RealNumberUniqueTable::lookupNonNegative(const fp val) {
  assert(!std::isnan(val));
  assert(val > 0);

  if (RealNumber::approximatelyEquals(val, 1.0)) {
    return &constants::one;
  }

  if (RealNumber::approximatelyEquals(val, SQRT2_2)) {
    return &constants::sqrt2over2;
  }

  const auto lowerKeyRaw = hash(val - RealNumber::eps);
  const auto upperKeyRaw = hash(val + RealNumber::eps);
  const auto keyRaw = hash(val);
  const auto lowerKey = static_cast<std::size_t>(lowerKeyRaw);
  const auto upperKey = static_cast<std::size_t>(upperKeyRaw);
  const auto key = static_cast<std::size_t>(keyRaw);

  std::array<std::size_t, 3> keyCandidates{lowerKey, upperKey, key};
  std::sort(keyCandidates.begin(), keyCandidates.end());
  const auto last = std::unique(keyCandidates.begin(), keyCandidates.end());
  std::array<std::unique_lock<std::mutex>, 3> bucketGuards;
  std::size_t guardIdx = 0U;
  for (auto it = keyCandidates.begin(); it != last; ++it) {
    bucketGuards[guardIdx++] = std::unique_lock<std::mutex>(bucketLocks[*it]);
  }

  {
    std::lock_guard<std::mutex> statsGuard(statsMutex);
    ++stats.lookups;
  }

  if (upperKey == lowerKey) {
    return findOrInsert(lowerKey, val);
  }

  // code below is to properly handle border cases |----(-|-)----|
  // in case a value close to a border is looked up,
  // only the last entry in the lower bucket and the first entry in the upper
  // bucket need to be checked

  RealNumber* pLower; // NOLINT(cppcoreguidelines-init-variables)
  RealNumber* pUpper; // NOLINT(cppcoreguidelines-init-variables)
  if (lowerKey != key) {
    pLower = tailTable[lowerKey];
    pUpper = table[key];
  } else {
    pLower = tailTable[key];
    pUpper = table[upperKey];
  }

  const bool lowerMatchFound =
      (pLower != nullptr &&
       RealNumber::approximatelyEquals(val, pLower->value));
  const bool upperMatchFound =
      (pUpper != nullptr &&
       RealNumber::approximatelyEquals(val, pUpper->value));

  if (lowerMatchFound && upperMatchFound) {
    {
      std::lock_guard<std::mutex> statsGuard(statsMutex);
      ++stats.hits;
    }
    const auto diffToLower = std::abs(pLower->value - val);
    const auto diffToUpper = std::abs(pUpper->value - val);
    // val is actually closer to p_lower than to p_upper
    if (diffToLower < diffToUpper) {
      return pLower;
    }
    return pUpper;
  }

  if (lowerMatchFound) {
    {
      std::lock_guard<std::mutex> statsGuard(statsMutex);
      ++stats.hits;
    }
    return pLower;
  }

  if (upperMatchFound) {
    {
      std::lock_guard<std::mutex> statsGuard(statsMutex);
      ++stats.hits;
    }
    return pUpper;
  }

  // Since no match was found, a new value needs to be added
  // Depending on which border of the bucket the value lies, a value either
  // needs to be inserted in the front or the back of the bucket.
  if (key == lowerKey) {
    return insertFront(key, val);
  }
  return insertBack(key, val);
}

bool RealNumberUniqueTable::possiblyNeedsCollection() const noexcept {
  std::lock_guard<std::mutex> statsGuard(statsMutex);
  return stats.numEntries >= gcLimit;
}

std::size_t RealNumberUniqueTable::garbageCollect(const bool force) noexcept {
  {
    std::lock_guard<std::mutex> statsGuard(statsMutex);
    if ((!force && stats.numEntries < gcLimit) ||
        stats.numEntries <= immortals::size()) {
      return 0U;
    }
    ++stats.gcRuns;
  }

  std::size_t collected = 0U;

#ifdef _OPENMP
#pragma omp parallel for reduction(+:collected) default(none)              \
  shared(table, tailTable, bucketLocks, memoryManager)
#endif
  for (std::size_t key = 0; key < table.size(); ++key) {
    std::unique_lock<std::mutex> bucketGuard(bucketLocks[key]);
    RealNumber* curr = table[key];
    RealNumber* prev = nullptr;
    std::size_t removedLocal = 0U;
    while (curr != nullptr) {
      if (!RealNumber::isImmortal(curr) && !RealNumber::isMarked(curr)) {
        RealNumber* next = curr->next();
        if (prev == nullptr) {
          table[key] = next;
        } else {
          prev->setNext(next);
        }
        memoryManager->returnEntry(*curr);
        curr = next;
        ++removedLocal;
      } else {
        prev = curr;
        curr = curr->next();
      }
    }
    tailTable[key] = prev;
    collected += removedLocal;
  }

  {
    std::lock_guard<std::mutex> statsGuard(statsMutex);
    if (collected > 0U) {
      stats.numEntries -= collected;
    }
    if (stats.numEntries > gcLimit / 10 * 9) {
      gcLimit = stats.numEntries + initialGCLimit;
    } else if (stats.numEntries < gcLimit / 128) {
      gcLimit = std::max(initialGCLimit, gcLimit / 2);
    }
  }
  return collected;
}

void RealNumberUniqueTable::clear() noexcept {
  for (std::size_t key = 0; key < table.size(); ++key) {
    std::unique_lock<std::mutex> bucketGuard(bucketLocks[key]);
    table[key] = nullptr;
    tailTable[key] = nullptr;
  }
  {
    std::lock_guard<std::mutex> statsGuard(statsMutex);
    gcLimit = initialGCLimit;
    stats.reset();
  }
}

void RealNumberUniqueTable::print() const {
  const auto precision = std::cout.precision();
  std::cout.precision(std::numeric_limits<dd::fp>::max_digits10);
  for (std::size_t key = 0; key < table.size(); ++key) {
    auto* p = table[key];
    if (p != nullptr) {
      std::cout << key << ": \n";
    }

    while (p != nullptr) {
      std::cout << "\t\t" << p->value << " "
                << reinterpret_cast<std::uintptr_t>(p) << "\n";
      p = p->next();
    }

    if (table[key] != nullptr) {
      std::cout << "\n";
    }
  }
  std::cout.precision(precision);
}

std::ostream& RealNumberUniqueTable::printBucketDistribution(std::ostream& os) {
  for (auto* bucket : table) {
    if (bucket == nullptr) {
      os << "0\n";
      continue;
    }
    std::size_t bucketCount = 0;
    while (bucket != nullptr) {
      ++bucketCount;
      bucket = bucket->next();
    }
    os << bucketCount << "\n";
  }
  os << "\n";
  return os;
}

std::size_t RealNumberUniqueTable::countMarkedEntries() const noexcept {
  std::size_t count = 0U;
#ifdef _OPENMP
#pragma omp parallel for reduction(+:count) default(none) shared(table, bucketLocks)
#endif
  for (std::size_t key = 0; key < table.size(); ++key) {
    std::unique_lock<std::mutex> bucketGuard(bucketLocks[key]);
    const auto* curr = table[key];
    std::size_t local = 0U;
    while (curr != nullptr) {
      if (RealNumber::isMarked(curr)) {
        ++local;
      }
      curr = curr->next();
    }
    count += local;
  }
  return count;
}

RealNumber* RealNumberUniqueTable::findOrInsert(const std::int64_t key,
                                                const fp val) {
  const auto k = static_cast<std::size_t>(key);
  auto* curr = table[k];
  if (curr == nullptr) {
    auto* entry = memoryManager->get<RealNumber>();
    entry->value = val;
    entry->setNext(curr);
    table[k] = entry;
    tailTable[k] = entry;
    {
      std::lock_guard<std::mutex> statsGuard(statsMutex);
      stats.trackInsert();
    }
    return entry;
  }

  auto* back = tailTable[k];
  if (back != nullptr && back->value <= val) {
    if (RealNumber::approximatelyEquals(val, back->value)) {
      {
        std::lock_guard<std::mutex> statsGuard(statsMutex);
        ++stats.hits;
      }
      return back;
    }
    {
      std::lock_guard<std::mutex> statsGuard(statsMutex);
      ++stats.collisions;
    }
    auto* entry = memoryManager->get<RealNumber>();
    entry->value = val;
    entry->setNext(nullptr);
    back->setNext(entry);
    tailTable[k] = entry;
    {
      std::lock_guard<std::mutex> statsGuard(statsMutex);
      stats.trackInsert();
    }
    return entry;
  }

  RealNumber* prev = nullptr;
  const fp valTol = val + RealNumber::eps;
  while (curr != nullptr && curr->value <= valTol) {
    if (RealNumber::approximatelyEquals(curr->value, val)) {
      // check if val is actually closer to the next element in the list (if
      // there is one)
      if (curr->next() != nullptr) {
        const auto& next = curr->next();
        // potential candidate in range
        if (valTol >= next->value) {
          const auto diffToCurr = std::abs(curr->value - val);
          const auto diffToNext = std::abs(next->value - val);
          // val is actually closer to next than to curr
          if (diffToNext < diffToCurr) {
            {
              std::lock_guard<std::mutex> statsGuard(statsMutex);
              ++stats.hits;
            }
            return next;
          }
        }
      }
      {
        std::lock_guard<std::mutex> statsGuard(statsMutex);
        ++stats.hits;
      }
      return curr;
    }
    {
      std::lock_guard<std::mutex> statsGuard(statsMutex);
      ++stats.collisions;
    }
    prev = curr;
    curr = curr->next();
  }

  auto* entry = memoryManager->get<RealNumber>();
  entry->value = val;

  if (prev == nullptr) {
    // add to front of bucket
    table[k] = entry;
  } else {
    prev->setNext(entry);
  }
  entry->setNext(curr);
  if (curr == nullptr) {
    tailTable[k] = entry;
  }
  {
    std::lock_guard<std::mutex> statsGuard(statsMutex);
    stats.trackInsert();
  }
  return entry;
}

RealNumber* RealNumberUniqueTable::insertFront(const std::int64_t key,
                                               const fp val) {
  auto* entry = memoryManager->get<RealNumber>();
  entry->value = val;

  const auto k = static_cast<std::size_t>(key);
  auto* curr = table[k];
  table[k] = entry;
  entry->setNext(curr);
  if (curr == nullptr) {
    tailTable[k] = entry;
  }
  {
    std::lock_guard<std::mutex> statsGuard(statsMutex);
    stats.trackInsert();
  }
  return entry;
}

RealNumber* RealNumberUniqueTable::insertBack(const std::int64_t key,
                                              const fp val) {
  auto* entry = memoryManager->get<RealNumber>();
  entry->value = val;
  entry->setNext(nullptr);

  const auto k = static_cast<std::size_t>(key);
  auto* back = tailTable[k];
  tailTable[k] = entry;
  if (back == nullptr) {
    table[k] = entry;
  } else {
    back->setNext(entry);
  }
  {
    std::lock_guard<std::mutex> statsGuard(statsMutex);
    stats.trackInsert();
  }
  return entry;
}

} // namespace dd
