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


def emsg(name, var, exp):
    return f'{name} is off: {var} vs expected: {exp}'


class EventCounter:
    # noqa: CNL100  (suppress flake8 error)
    def __init__(self):
        self._num_on_events = 0
        self._num_off_events = 0
        self._num_rise_trig = 0
        self._num_fall_trig = 0
        self._sum_time = 0

    def add_cd_events(self, cd_events):
        if cd_events.shape[0] > 0:
            self._num_off_events += np.count_nonzero(cd_events['p'] == 0)
            self._num_on_events += np.count_nonzero(cd_events['p'] == 1)
            self._sum_time += np.sum(cd_events['t'])

    def add_trig_events(self, trig_events):
        if trig_events.shape[0] > 0:
            self._num_rise_trig += np.count_nonzero(trig_events['p'] == 0)
            self._num_fall_trig += np.count_nonzero(trig_events['p'] == 1)
            self._sum_time += np.sum(trig_events['t'])

    def add_cd_event_packets(self, cd_event_packets):
        for p in cd_event_packets:
            self.add_cd_events(p)

    def add_trig_event_packets(self, trig_event_packets):
        for p in trig_event_packets:
            self.add_trig_events(p)

    def check_count(
        self,
        sum_time,
        num_off_events,
        num_on_events,
        num_rise_trig,
        num_fall_trig,
    ):
        assert self._sum_time == sum_time, emsg('sum_time', self._sum_time, sum_time)
        assert self._num_off_events == num_off_events, emsg(
            'num_off_events', self._num_off_events, num_off_events
        )
        assert self._num_on_events == num_on_events, emsg(
            'num_on_events', self._num_on_events, num_on_events
        )
        assert self._num_rise_trig == num_rise_trig, emsg(
            'num_rise_trig', self._num_rise_trig, num_rise_trig
        )
        assert self._num_rise_trig == num_rise_trig, emsg(
            'num_fall_trig', self._num_fall_trig, num_fall_trig
        )

    def get_result_dict(self):
        return {
            'num_off_events': self._num_off_events,
            'num_on_events': self._num_on_events,
            'num_rise_trig': self._num_rise_trig,
            'num_fall_trig': self._num_fall_trig,
            'sum_time': self._sum_time,
        }

    def print_results(self):
        print(self.get_result_dict())
