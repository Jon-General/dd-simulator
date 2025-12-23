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

#include "dd/ThreadConfig.hpp"

#include <algorithm>

#ifdef _OPENMP
#include <omp.h>
#endif

namespace dd {

int resolveOpenMPThreadCount(const int requested) noexcept {
#ifdef _OPENMP
  if (requested <= 0) {
    return std::max(1, omp_get_num_procs());
  }
  return std::max(1, requested);
#else
  (void)requested;
  return 1;
#endif
}

void configureOpenMPThreads(const int requested) noexcept {
#ifdef _OPENMP
  omp_set_dynamic(0);
  omp_set_num_threads(resolveOpenMPThreadCount(requested));
#else
  (void)requested;
#endif
}

} // namespace dd
