# event_camera_py

This repository holds ROS/ROS2 tools for processing
[event_camera_msgs](https://github.com/ros-event-camera/event_camera_msgs)
under ROS and ROS2 with python. These messages are produced by the
[metavision_ros_driver](https://github.com/ros-event-camera/metavision_ros_driver). For decoding, the
[event_camera_codecs](https://github.com/ros-event-camera/event_camera_codecs)
package is used.

With this repository you can quickly load events from a ROS/ROS2 bag
into your python code. The decoder will return a structured numpy array of the same format that the Metavision SDK uses:
```python
dtype={'names':['x','y','p','t'], 'formats':['<u2','<u2','i1','<i4'], 'offsets':[0,2,4,8], 'itemsize':12})]
```
To access e.g. the timestamps (in microseconds) you would use ``foo['t']``, where ``foo`` is the numpy array returned by the decoder. See sample code below.

## Supported platforms

Currently tested on:

 - Ubuntu 20.04: ROS Noetic and ROS2 Galactic
 - Ubuntu 22.04: ROS2 Humble and ROS2 Rolling

## How to build
Create a workspace, clone this repo, and use ``vcs``
to pull in the remaining dependencies:

```bash
pkg=event_camera_py
mkdir -p ~/${pkg}_ws/src
cd ~/${pkg}_ws
git clone https://github.com/ros-event-camera/${pkg}.git src/${pkg}
cd src
vcs import < ${pkg}/${pkg}.repos
cd ..
```
### Install system dependencies
You will probably be missing the ``pybind11_catkin package``:
```bash
sudo apt-get install ros-${ROS_DISTRO}-pybind11-catkin
```

### configure and build on ROS1:

```bash
catkin config -DCMAKE_BUILD_TYPE=RelWithDebInfo  # (optionally add -DCMAKE_EXPORT_COMPILE_COMMANDS=1)
catkin build
```

### configure and build on ROS2:

```bash
cd ~/${pkg}_ws
colcon build --symlink-install --cmake-args -DCMAKE_BUILD_TYPE=RelWithDebInfo  # (optionally add -DCMAKE_EXPORT_COMPILE_COMMANDS=1)
```

## Decoding event array messages

The following sample code shows how to decode event array messages under ROS1.
```python
import rosbag
from event_camera_py import Decoder

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
```python
from bag_reader_ros2 import BagReader
from event_camera_py import Decoder

topic = '/event_camera/events'
bag = BagReader('foo', topic)
decoder = Decoder()

while bag.has_next():
        topic, msg, t_rec = bag.read_next()
        decoder.decode(msg)
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
the encoding. 
  - For 'evt3' (metavision) encoding the raw packet needs to be decoded
    to obtain the sensor time. The encoded sensor time has two quirks: it
    wraps around every 2^24 usec (16.77 sec) and it has bit noise errors.
    The decoder used by the ``event_camera_py`` packet keeps track of the
    wrap around and tries to correct the bit errors. But if you start
    decoding from the middle of the event stream your sensor time stamps
    will start at somewhere between 0 and 16.77s due to the wrap
    around, i.e. sensor time depends on where you start decoding in the
    message stream.
  - For 'libcaer_cmp' (libcaer) encoding, the time stamps in the event
    stream are in nanoseconds since epoch, which makes them unsuitable for
    32 bit representation. For this reason the decoder sets the time stamp
    of the first event to zero, and all subsequent event times are relative
    to the first event time. The time since epoch (in usec) of the first
    event can be obtained from the decoder via ``get_start_time()``.

The time 't' column in the python array returned by ``get_cd_events()``
is the sensor time 3), in micro seconds. The host time can be
obtained by suitably combining the sensor time 3) with the ROS header
stamp 2). The most naive way is to compute the time difference between
sensor time and header stamp for the first packet and subsequently
use that difference to obtain host time from sensor time. Obviously
this will not account for drift between sensor and host clocks.

Note that the event time stamps in the structured python array are represented
by a 32bit signed integer and thus will roll over after about 35mins!


## License

This software is issued under the Apache License Version 2.0.
