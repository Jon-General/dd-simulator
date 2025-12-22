/*
 * Copyright (c) 2023 - 2025 Chair for Design Automation, TUM
 * Copyright (c) 2025 Munich Quantum Software Company GmbH
 * All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#ifdef _OPENMP
#include <omp.h>
#endif

namespace dd {
inline void configureOpenMPThreads(const int desiredThreads) {
#ifdef _OPENMP
  if (desiredThreads > 0) {
    omp_set_num_threads(desiredThreads);
  }
#else
  (void)desiredThreads;
#endif
}
} // namespace dd
