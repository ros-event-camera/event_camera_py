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

import rosbag


class BagReader:
    """Convenience class for reading ROS1 bags."""

    def __init__(self, bag_name, verbose=False):
        self._bag = rosbag.Bag(bag_name + '.bag')
        if verbose:
            print('Opening bag :: ' + bag_name + '.bag')

    def read_messages(self, topics):
        return self._bag.read_messages(topics=topics)
