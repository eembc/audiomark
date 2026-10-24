/**
 * Copyright 2026 Harshit Kumar Shivhare
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef RVV_SUPPORT_GUARD_H
#define RVV_SUPPORT_GUARD_H

#if defined(__riscv) && defined(__riscv_vector)
#include <riscv_vector.h>
#else
#error "RISCV VECTOR EXTENSION NOT SUPPORTED"
#endif

#endif // RVV_SUPPORT_GUARD_H
