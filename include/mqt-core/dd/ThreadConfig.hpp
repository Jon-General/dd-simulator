/*
 * Copyright (c) 2023 - 2025 Chair for Design Automation, TUM
 * Copyright (c) 2025 Munich Quantum Software Company GmbH
 * All rights reserved.
 *
 * SPDX-License-Identifier: MIT
<<<<<<< HEAD
=======
 *
 * Licensed under the MIT License
>>>>>>> 413c2c13 (test)
 */

#pragma once

<<<<<<< HEAD
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
=======
namespace dd {

/**
 * @brief Resolve the OpenMP thread count requested by the user.
 *
 * @param requested Number of threads requested via configuration. A value of
 *        zero or a negative number indicates that all available processing
 *        units should be used.
 * @return The clamped number of threads that should be used at runtime.
 */
int resolveOpenMPThreadCount(int requested) noexcept;

/**
 * @brief Configure the OpenMP runtime for multi-core execution.
 *
 * @param requested Number of threads requested via configuration. A value of
 *        zero or a negative number indicates that all available processing
 *        units should be used.
 */
void configureOpenMPThreads(int requested) noexcept;

>>>>>>> 413c2c13 (test)
} // namespace dd
