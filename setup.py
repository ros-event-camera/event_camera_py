# ! DO NOT MANUALLY INVOKE THIS setup.py, USE CATKIN INSTEAD
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

from setuptools import setup
import os

package_name = 'event_camera_py'
ROS_VERSION = int(os.environ['ROS_VERSION'])

if ROS_VERSION == 1:
    from catkin_pkg.python_setup import generate_distutils_setup
    # fetch values from package.xml
    setup_args = generate_distutils_setup(
        packages=[package_name],
        package_dir={'': ''}  # location of python source is at top
    )
    setup(**setup_args)
elif ROS_VERSION == 2:
    setup(
        name=package_name,
        version='1.0.0',
        packages=[package_name],
        data_files=[
            ('share/ament_index/resource_index/packages',
             ['resource/' + package_name]),
            ('share/' + package_name, ['package.xml']),
        ],
        install_requires=['setuptools'],
        zip_safe=True,
        maintainer='Bernd Pfrommer',
        maintainer_email='bernd.pfrommer@gmail.com',
        description='Python access for event_camera_msgs',
        license='Apache License 2.0',
        tests_require=['pytest'],
        entry_points={
            'console_scripts': [
            ],
        })
