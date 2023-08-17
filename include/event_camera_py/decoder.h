// -*-c++-*--------------------------------------------------------------------
// Copyright 2022 Bernd Pfrommer <bernd.pfrommer@gmail.com>
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef EVENT_CAMERA_PY__DECODER_H_
#define EVENT_CAMERA_PY__DECODER_H_

#include <event_camera_codecs/decoder.h>
#include <event_camera_codecs/decoder_factory.h>
#include <event_camera_codecs/event_packet.h>
#include <event_camera_py/event_cd.h>
#include <event_camera_py/event_ext_trig.h>
#include <pybind11/numpy.h>
#include <pybind11/pybind11.h>

#include <string>
#include <tuple>
#include <vector>

class Decoder : public event_camera_codecs::EventProcessor
{
public:
  Decoder();
  // inherited from EventProcessor
  void eventCD(uint64_t sensor_time, uint16_t ex, uint16_t ey, uint8_t polarity) override;
  void eventExtTrigger(uint64_t sensor_time, uint8_t edge, uint8_t id) override;
  void finished() override {}
  void rawData(const char *, size_t) override {}
  // own methods
  void decode(pybind11::object msg);
  void decode_bytes(
    const std::string & encoding, uint16_t width, uint16_t height, uint64_t timeBase,
    pybind11::bytes events);
  void decode_array(
    const std::string & encoding, uint16_t width, uint16_t height, uint64_t timeBase,
    pybind11::array_t<uint8_t>);
  std::tuple<bool, uint64_t> decode_until(pybind11::object eventPacket, uint64_t untilTime);

  pybind11::array_t<EventCD> get_cd_events();
  pybind11::array_t<EventExtTrig> get_ext_trig_events();

  size_t get_num_cd_off() const { return (numCDEvents_[0]); }
  size_t get_num_cd_on() const { return (numCDEvents_[1]); }
  size_t get_num_trigger_rising() const { return (numExtTrigEvents_[0]); }
  size_t get_num_trigger_falling() const { return (numExtTrigEvents_[1]); }

private:
  void do_full_decode(
    const std::string & encoding, uint16_t width, uint16_t height, uint64_t timeBase,
    const uint8_t * buf, size_t bufSize);
  event_camera_codecs::Decoder<event_camera_codecs::EventPacket, Decoder> * initialize_decoder(
    const std::string & encoding, uint32_t width, uint32_t height);
  void reset_stored_events();
  // ------------ variables
  event_camera_codecs::DecoderFactory<event_camera_codecs::EventPacket, Decoder> decoderFactory_;
  size_t numCDEvents_[2] = {0, 0};
  size_t numExtTrigEvents_[2] = {0, 0};
  std::vector<EventCD> * cdEvents_{0};
  std::vector<EventExtTrig> * extTrigEvents_{0};
  size_t maxSizeCD_{0};
  size_t maxSizeExtTrig_{0};
};

#endif  // EVENT_CAMERA_PY__DECODER_H_
