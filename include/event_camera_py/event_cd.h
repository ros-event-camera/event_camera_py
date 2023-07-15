// -*-c++-*--------------------------------------------------------------------
// Copyright 2022 Bernd Pfrommer <bernd.pfrommer@gmail.com>
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef EVENT_CAMERA_PY__EVENT_CD_H_
#define EVENT_CAMERA_PY__EVENT_CD_H_

#include <cstdint>

struct EventCD
{
  explicit EventCD(uint16_t xa = 0, uint16_t ya = 0, int8_t pa = 0, int32_t ta = 0)
  : x(xa), y(ya), p(pa), t(ta)
  {
  }
  uint16_t x;
  uint16_t y;
  int8_t p;
  int32_t t;
};
#endif  // EVENT_CAMERA_PY__EVENT_CD_H_
