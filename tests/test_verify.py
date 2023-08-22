# -----------------------------------------------------------------------------
# Copyright 2023 Bernd Pfrommer <bernd.pfrommer@gmail.com>
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

import numpy as np


def packet_is_unique(packets) -> bool:
    for p in (pp for pp in packets if pp.shape[0] > 0):
        idx = np.stack((p["x"], p["y"]), axis=1)
        _, c = np.unique(idx, return_counts=True, axis=0)
        num_with_duplicates = np.count_nonzero(c > 1)
        if num_with_duplicates > 0:
            print("bad packet:\n", p)
            print(f"has {num_with_duplicates} duplicates at:")
            return False
    return True
