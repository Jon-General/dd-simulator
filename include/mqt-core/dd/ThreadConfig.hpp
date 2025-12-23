/*
 * Copyright (c) 2023 - 2025 Chair for Design Automation, TUM
 * Copyright (c) 2025 Munich Quantum Software Company GmbH
 * All rights reserved.
 *
 * SPDX-License-Identifier: MIT
<<<<<<< HEAD
 *
 * Licensed under the MIT License
=======
 */

/* Additional modifications by Jul Jon General
 * UP Diliman - Department of Computer Science
 * Logic & Computability Laboratory
 * 2025
>>>>>>> f8ae3e26 (added attributions to Jon General)
 */

#pragma once

<<<<<<< HEAD
=======
#ifdef _OPENMP
#include <omp.h>
#endif

>>>>>>> f8ae3e26 (added attributions to Jon General)
namespace dd {

// Determine how many threads to request from OpenMP. Fallback to 1 when
// OpenMP is unavailable or reports an invalid count.
int resolveOpenMPThreadCount(int requested) noexcept;

// Configure the OpenMP runtime using the resolved thread count.
void configureOpenMPThreads(int requested) noexcept;

} // namespace dd
