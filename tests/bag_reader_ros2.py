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

from rclpy.serialization import deserialize_message
import rosbag2_py
from rosidl_runtime_py.utilities import get_message


class BagReader:
    """Convenience class for reading ROS2 bags."""

    def __init__(self, bag_name, verbose=False):
        storage_options, converter_options = self.__get_rosbag_options(str(bag_name))
        self._reader = rosbag2_py.SequentialReader()
        self._reader.open(storage_options, converter_options)
        topic_types = self._reader.get_all_topics_and_types()
        self._type_map = {
            topic_types[i].name: topic_types[i].type for i in range(len(topic_types))
        }
        if verbose:
            print('Opening bag :: ' + bag_name)

    def read_messages(self, topics):
        self._reader.set_filter(rosbag2_py.StorageFilter(topics=topics))
        all_msgs = []
        while self._reader.has_next():
            all_msgs.append(self.__read_next())
        return all_msgs

    def __read_next(self):
        (topic, data, t_rec) = self._reader.read_next()
        msg_type = get_message(self._type_map[topic])
        msg = deserialize_message(data, msg_type)
        return (topic, msg, t_rec)

    def __get_rosbag_options(self, path, serialization_format='cdr'):
        storage_options = rosbag2_py.StorageOptions(uri=path)
        converter_options = rosbag2_py.ConverterOptions(
            input_serialization_format=serialization_format,
            output_serialization_format=serialization_format,
        )
        return storage_options, converter_options
