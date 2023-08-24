#
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

set(CMAKE_CXX_STANDARD 17)

add_compile_options(-Wall -Wextra -Wpedantic -Werror)

add_definitions(-DUSING_ROS_1)

find_package(catkin REQUIRED COMPONENTS
  pybind11_catkin
  event_camera_msgs
  event_camera_codecs)

catkin_package(
    LIBRARIES
    CATKIN_DEPENDS pybind11_catkin)

include_directories(
  include
  ${catkin_INCLUDE_DIRS})

pybind_add_module(_event_camera_py MODULE src/decoder.cpp)

install(TARGETS
  _event_camera_py
  LIBRARY DESTINATION ${CATKIN_PACKAGE_BIN_DESTINATION})

install(FILES
  tests/test_events_1.bag
  DESTINATION ${CATKIN_PACKAGE_SHARE_DESTINATION}/data)

if(CATKIN_ENABLE_TESTING)
  catkin_add_nosetests(
    tests/test_1.py
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
    DEPENDENCIES
    ${catkin_EXPORTED_TARGETS} ${${PROJECT_NAME}_EXPORTED_TARGETS})
endif()

catkin_python_setup()
