/*
 * Copyright 2022 CAS—Atlantic (University of New Brunswick, CASA)
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef _ODIN_II_H_
#define _ODIN_II_H_

#include "odin_types.h"
/* Odin-II exit status enumerator */
enum ODIN_ERROR_CODE { ERROR_INITIALIZATION, ERROR_PARSE_CONFIG, ERROR_PARSE_ARCH, ERROR_ELABORATION, ERROR_OPTIMIZATION, ERROR_TECHMAP };

void set_default_config();

#endif // _ODIN_II_H_
