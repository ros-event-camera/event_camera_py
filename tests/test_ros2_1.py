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
from rosidl_runtime_py.utilities import get_message
from event_camera_py import Decoder
import numpy as np
import rosbag2_py


class BagReader():
    """Convenience class for reading ROS2 bags."""

    def __init__(self, bag_name, topics):
        bag_path = str(bag_name)
        storage_options, converter_options = self.get_rosbag_options(bag_path)
        self.reader = rosbag2_py.SequentialReader()
        self.reader.open(storage_options, converter_options)
        topic_types = self.reader.get_all_topics_and_types()
        self.type_map = {topic_types[i].name: topic_types[i].type
                         for i in range(len(topic_types))}
        storage_filter = rosbag2_py.StorageFilter(topics=[topics])
        self.reader.set_filter(storage_filter)

    def has_next(self):
        return self.reader.has_next()

    def read_next(self):
        (topic, data, t_rec) = self.reader.read_next()
        msg_type = get_message(self.type_map[topic])
        msg = deserialize_message(data, msg_type)
        return (topic, msg, t_rec)

    def get_rosbag_options(self, path, serialization_format='cdr'):
        storage_options = rosbag2_py.StorageOptions(uri=path,
                                                    storage_id='sqlite3')
        converter_options = rosbag2_py.ConverterOptions(
            input_serialization_format=serialization_format,
            output_serialization_format=serialization_format)
        return storage_options, converter_options


def test_readbag():
    topic = '/event_camera/events'
    bag = BagReader('tests/test_events_ros2_1', topic)
    decoder = Decoder()
    num_on_events = 0
    num_off_events = 0
    num_up_trig = 0
    num_down_trig = 0
    sum_time = 0
    while bag.has_next():
        topic, msg, t_rec = bag.read_next()
        decoder.decode_bytes(msg.encoding, msg.width, msg.height,
                             msg.time_base, msg.events.tobytes())
        cd_events = decoder.get_cd_events()
        trig_events = decoder.get_ext_trig_events()
        if cd_events.shape[0] > 0:
            num_off_events += np.count_nonzero(cd_events['p'] == 0)
            num_on_events += np.count_nonzero(cd_events['p'] == 1)
            sum_time += np.sum(cd_events['t'])
        if trig_events.shape[0] > 0:
            num_up_trig += np.count_nonzero(trig_events['p'] == 0)
            num_down_trig += np.count_nonzero(trig_events['p'] == 1)
            sum_time += np.sum(trig_events['t'])

    assert(num_off_events == 238682)
    assert(num_on_events == 139584)
    assert(num_up_trig == 18)
    assert(num_down_trig == 18)
    assert(sum_time == 6024891770230)
