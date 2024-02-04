# -----------------------------------------------------------------------------
# Copyright 2022 Bernd Pfrommer <bernd.pfrommer@gmail.com>
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
#
try:
    # under ROS2 need to add dll dir for windows
    from rpyutils import add_dll_directories_from_env

    with add_dll_directories_from_env('PATH'):
        from event_camera_py._event_camera_py import Decoder
        from event_camera_py._event_camera_py import UniqueDecoder

except ImportError:
    try:
        # if rpyutils does not insist, try regular import under ROS2
        from event_camera_py._event_camera_py import Decoder
        from event_camera_py._event_camera_py import UniqueDecoder
    except ImportError:
        # import under ROS1
        from _event_camera_py import Decoder
        from _event_camera_py import UniqueDecoder
__all__ = ['Decoder', 'UniqueDecoder']
