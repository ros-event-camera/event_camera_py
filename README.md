# event_array_py

This repository holds ROS/ROS2 tools for processing
[event_array_msgs](https://github.com/berndpfrommer/event_array_msgs)
under ROS and ROS2 with python. These messages are produced by the
[metavision_ros_driver](https://github.com/berndpfrommer/metavision_ros_driver). For decoding, the
[event_array_codecs](https://github.com/berndpfrommer/event_array_codecs)
package is used.

With this repository you can quickly load events from a ROS/ROS2 bag
into your python code. The decoder will return a structured numpy array of the same format that the Metavision SDK uses, i.e.
``dtype={'names':['x','y','p','t'], 'formats':['<u2','<u2','i1','<i4']``. To access e.g. the timestamps (in microseconds) you would use ``foo['t']``, where ``foo`` is the numpy array returned by the decoder. See sample code below.

## Supported platforms

Currently tested on Ubuntu 20.04 under ROS Noetic and ROS2 Galactic
and on Ubuntu 22.04 under ROS2 Humble.

## How to build
Create a workspace (``event_array_py_ws``), clone this repo, and use ``vcs``
to pull in the remaining dependencies:

```
pkg=event_array_py
mkdir -p ~/${pkg}_ws/src
cd ~/${pkg}_ws
git clone https://github.com/berndpfrommer/${pkg}.git src/${pkg}
cd src
vcs import < ${pkg}/${pkg}.repos
cd ..
```
### Install system dependencies
You will probably be missing the ``pybind11_catkin package``:
```
sudo apt-get install ros-${ROS_DISTRO}-pybind11-catkin
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

The following sample code shows how to decode event array messages under ROS1.
```
import rosbag
from event_array_py import Decoder

topic = '/event_camera/events'
bag = rosbag.Bag('foo.bag')
decoder = Decoder()

for topic, msg, t in bag.read_messages(topics=topic):
    decoder.decode_bytes(msg.encoding, msg.width, msg.height,
	                     msg.time_base, msg.events)
    cd_events = decoder.get_cd_events()
    print(cd_events)
    trig_events = decoder.get_ext_trig_events()
    print(trig_events)
```
Here is a sample code for ROS2. It uses a helper class "BagReader"
that you can find in the ``src`` folder. Note the conversion to numpy array:
```
from bag_reader_ros2 import BagReader
from event_array_py import Decoder

topic = '/event_camera/events'
bag = BagReader('foo', topic)
decoder = Decoder()

while bag.has_next():
        topic, msg, t_rec = bag.read_next()
        decoder.decode_bytes(msg.encoding, msg.width, msg.height,
                             msg.time_base, msg.events.tobytes())
        cd_events = decoder.get_cd_events()
        print(cd_events)
        trig_events = decoder.get_ext_trig_events()
        print(trig_events)
```

The returned event arrays are structured numpy ndarrays that are
compatible with Prophesee's Metavision SDK.

## About timestamps

A message in a recorded rosbag has three sources of time information:

1) The recording timestamp. This is when the message was written into
the bag by the rosbag recorder. It is the least precise of all time
stamps and therefore usually not used.
2) The message time stamp in the header (header.stamp). This is the
time when the ROS driver host received the first event packet from the SDK
for that ROS message. Remember that a ROS message can contain multiple
SDK packets, but the header.stamp refers to the first SDK packet
received.
3) The sensor time encoded in the packets. This time stamp depends on
the encoding. For standard 'evt3' encoding the raw packet needs to be decoded
to obtain the sensor time. The encoded sensor time has two quirks: it
wraps around every 2^24 usec (16.77 sec) and it has bit noise errors.
The decoder used by the ``event_array_py`` packet keeps track of the
wrap around and tries to correct the bit errors. But if you start
decoding from the middle of the event stream your sensor time stamps
will start at somewhere between 0 and 16.77s due to the wrap
around, i.e. sensor time depends on where you start decoding in the
message stream.

The time 't' column in the python array returned by ``get_cd_events()``
is the sensor time 3), in micro seconds. The host time can be
obtained by suitably combining the sensor time 3) with the ROS header
stamp 2). The most naive way is to compute the time difference between
sensor time and header stamp for the first packet and subsequently
use that difference to obtain host time from sensor time. Obviously
this will not account for drift between sensor and host clocks.


## License

This software is issued under the Apache License Version 2.0.
