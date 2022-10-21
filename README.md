# event_array_py

This repository holds ROS/ROS2 tools for processing
[event_array_msgs](https://github.com/berndpfrommer/event_array_msgs)
under ROS and ROS2 with python. These messages are produced by the
[metavision_ros_driver](https://github.com/berndpfrommer/metavision_ros_driver).

## Supported platforms

Currently tested on Ubuntu 20.04 under ROS Noetic and ROS2 Galactic.

## How to build
Create a workspace (``event_array_py_ws``), clone this repo, and use ``wstool``
to pull in the remaining dependencies:

```
pkg=event_array_py
mkdir -p ~/${pkg}_ws/src
cd ~/${pkg}_ws
git clone https://github.com/berndpfrommer/${pkg}.git src/${pkg}
wstool init src src/${pkg}/${pkg}.rosinstall
# to update an existing space:
# wstool merge -t src src/${pkg}/${pkg}.rosinstall
# wstool update -t src
```

### configure and build on ROS1:

```
catkin config -DCMAKE_BUILD_TYPE=RelWithDebInfo  # (optionally add -DCMAKE_EXPORT_COMPILE_COMMANDS=1)
catkin build
```

### configure and build on ROS2:

```
cd ~/${pkg}_ws
colcon build --symlink-install --cmake-args -DCMAKE_BUILD_TYPE=RelWithDebInfo  # (optionally add -DCMAKE_EXPORT_COMPILE_COMMANDS=1)
```

## Decoding event array messages

The following sample code shows how to decode event array messages
under ROS1. The usage of the decoder is identical under ROS2.
```
import rosbag
from event_array_py import Decoder

bag = rosbag.Bag('foo.bag')
decoder = Decoder()

for topic, msg, t in bag.read_messages(topics=topic):
    decoder.decode(msg.encoding, msg.time_base, msg.events)
    cd_events = decoder.get_cd_events()
    print(cd_events)
    trig_events = decoder.get_ext_trig_events()
    print(trig_events)
```
The returned event arrays are structured numpy ndarrays that are
compatible with Prophesee's Metavision SDK.

## License

This software is issued under the Apache License Version 2.0.
