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

#include "dd/UniqueTable.hpp"

#include "dd/MemoryManager.hpp"
#include "dd/Node.hpp"

#include <algorithm>
#include <cstddef>
#include <nlohmann/json.hpp>
#include <numeric>
#include <string>
#include <vector>

#ifdef _OPENMP
#include <omp.h>
#endif

namespace dd {

UniqueTable::UniqueTable(MemoryManager& manager,
                         const UniqueTableConfig& config)
    : cfg(config), gcLimit(config.initialGCLimit), memoryManager(&manager),
      tables(config.nVars), stats(config.nVars) {
  for (auto& stat : stats) {
    stat.entrySize = sizeof(Bucket);
    stat.numBuckets = cfg.nBuckets;
  }
  initializeLocks();
}

void UniqueTable::resize(const std::size_t nVars) {
  std::unique_lock lock(tableMutex);
  cfg.nVars = nVars;
  tables.resize(nVars, Table(cfg.nBuckets));
  // TODO: if the new size is smaller than the old one we might have to
  // release the unique table entries for the superfluous variables
  stats.resize(nVars);
  for (auto& stat : stats) {
    stat.entrySize = sizeof(Bucket);
    stat.numBuckets = cfg.nBuckets;
  }
  initializeLocks();
}

bool UniqueTable::possiblyNeedsCollection() const {
  return getNumEntries() >= gcLimit;
}

std::size_t UniqueTable::garbageCollect(const bool force) {
  std::unique_lock lock(tableMutex);

  const std::size_t numEntriesBefore = getNumEntriesLocked();
  if ((!force && numEntriesBefore < gcLimit) || numEntriesBefore == 0U) {
    return 0U;
  }

  std::size_t totalCollected = 0U;

#ifdef _OPENMP
#pragma omp parallel for reduction(+:totalCollected) default(none)          \
    shared(tables, stats, memoryManager)
#endif
  for (std::size_t v = 0; v < tables.size(); ++v) {
    auto& table = tables[v];
    auto& stat = stats[v];
    ++stat.gcRuns;
    std::size_t removed = 0U;
    for (auto& bucket : table) {
      NodeBase* current = bucket;
      NodeBase* previous = nullptr;
      while (current != nullptr) {
        if (!current->isMarked()) {
          NodeBase* next = current->next();
          if (previous == nullptr) {
            bucket = next;
          } else {
            previous->setNext(next);
          }
          memoryManager->returnEntry(*current);
          current = next;
          ++removed;
        } else {
          previous = current;
          current = current->next();
        }
      }
    }
    stat.numEntries -= removed;
    totalCollected += removed;
  }

  const auto numEntries = numEntriesBefore - totalCollected;
  if (numEntries > gcLimit / 10 * 9) {
    gcLimit = numEntries + cfg.initialGCLimit;
  }
  return totalCollected;
}

void UniqueTable::clear() {
  std::unique_lock lock(tableMutex);
  for (auto& table : tables) {
    std::fill(table.begin(), table.end(), nullptr);
  }
  gcLimit = cfg.initialGCLimit;
  for (std::size_t v = 0; v < stats.size(); ++v) {
    if (v < statsLocks.size() && statsLocks[v] != nullptr) {
      std::lock_guard<std::mutex> statsGuard(*statsLocks[v]);
      stats[v].reset();
      stats[v].gcRuns = 0U;
    } else {
      stats[v].reset();
      stats[v].gcRuns = 0U;
    }
  }
};

UniqueTableStatistics
UniqueTable::getStats(const std::size_t idx) const noexcept {
  std::shared_lock lock(tableMutex);
  if (idx < statsLocks.size() && statsLocks[idx] != nullptr) {
    std::lock_guard<std::mutex> guard(*statsLocks[idx]);
    return stats.at(idx);
  }
  return stats.at(idx);
}

nlohmann::basic_json<>
UniqueTable::getStatsJson(const bool includeIndividualTables) const {
  std::shared_lock lock(tableMutex);
  if (std::ranges::all_of(stats, [](const UniqueTableStatistics& stat) {
        return stat.peakNumEntries == 0U;
      })) {
    return "unused";
  }

  UniqueTableStatistics totalStats;
  std::vector<UniqueTableStatistics> snapshots;
  snapshots.reserve(stats.size());
  for (std::size_t idx = 0; idx < stats.size(); ++idx) {
    if (idx < statsLocks.size() && statsLocks[idx] != nullptr) {
      std::lock_guard<std::mutex> guard(*statsLocks[idx]);
      snapshots.push_back(stats[idx]);
    } else {
      snapshots.push_back(stats[idx]);
    }
  }

  for (const auto& snapshot : snapshots) {
    totalStats.entrySize = std::max(totalStats.entrySize, snapshot.entrySize);
    totalStats.numBuckets += snapshot.numBuckets;
    totalStats.numEntries += snapshot.numEntries;
    totalStats.peakNumEntries += snapshot.peakNumEntries;
    totalStats.collisions += snapshot.collisions;
    totalStats.hits += snapshot.hits;
    totalStats.lookups += snapshot.lookups;
    totalStats.inserts += snapshot.inserts;
    totalStats.gcRuns = std::max(totalStats.gcRuns, snapshot.gcRuns);
  }

  nlohmann::basic_json<> j;
  j["total"] = totalStats.json();
  if (includeIndividualTables) {
    std::size_t v = 0U;
    for (const auto& snapshot : snapshots) {
      j[std::to_string(v)] = snapshot.json();
      ++v;
    }
  }
  return j;
}

std::size_t UniqueTable::getNumEntries() const noexcept {
  std::shared_lock lock(tableMutex);
  return getNumEntriesLocked();
}

std::size_t UniqueTable::countMarkedEntries() const noexcept {
  std::shared_lock lock(tableMutex);
  std::size_t count = 0U;
#ifdef _OPENMP
#pragma omp parallel for reduction(+:count) default(none) shared(tables)
#endif
  for (std::size_t idx = 0; idx < tables.size(); ++idx) {
    const auto& table = tables[idx];
    std::size_t local = 0U;
    for (const auto* bucket : table) {
      auto* p = bucket;
      while (p != nullptr) {
        if (p->isMarked()) {
          ++local;
        }
        p = p->next();
      }
    }
    count += local;
  }
  return count;
}

} // namespace dd

void dd::UniqueTable::initializeLocks() {
  bucketLocks.clear();
  bucketLocks.resize(cfg.nVars);
  for (auto& locks : bucketLocks) {
    locks.clear();
    locks.reserve(cfg.nBuckets);
    for (std::size_t key = 0; key < cfg.nBuckets; ++key) {
      locks.emplace_back(std::make_unique<std::mutex>());
    }
  }

  statsLocks.clear();
  statsLocks.reserve(cfg.nVars);
  for (std::size_t v = 0; v < cfg.nVars; ++v) {
    statsLocks.emplace_back(std::make_unique<std::mutex>());
  }
}

std::size_t dd::UniqueTable::getNumEntriesLocked() const noexcept {
  std::size_t total = 0U;
  for (std::size_t v = 0; v < stats.size(); ++v) {
    if (v < statsLocks.size() && statsLocks[v] != nullptr) {
      std::lock_guard<std::mutex> guard(*statsLocks[v]);
      total += stats[v].numEntries;
    } else {
      total += stats[v].numEntries;
    }
  }
  return total;
}
