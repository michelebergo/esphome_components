/* 
One Datapackage consists of:
<start> <src> <dst> <cmd> <data: 8 bytes> <chksum> <end>

With:
Byte   Identifier   Comments
----------------------------
0      Start  : start of message (0x32)
1      Src    : Source address (00 01 02 03 = indoor units; C8 = outdoor unit; AD = ?; F0 = ?)
2      Dst    : Destination address
3      Cmd    : Command byte
4-11   Data   : Data is always 8 bytes in length, unused bytes will be zero
12     Chksum : Checksum of message which is the XOR of bytes 2-12
13     End    : end of message (0x34)

*/

#include "esphome/core/log.h"
#include "samsung_ac_f1f2com.h"
#include <vector>

namespace esphome {
namespace samsung_ac_f1f2com {

static const char *TAG = "samsung_ac_f1f2com";

static const uint8_t DATA_SRC = 1;
static const uint8_t DATA_DST = 2;
static const uint8_t DATA_CMD = 3;
static const uint8_t DATA_BYTE1 = 4;
static const uint8_t DATA_BYTE2 = 5;
static const uint8_t DATA_BYTE3 = 6;
static const uint8_t DATA_BYTE4 = 7;
static const uint8_t DATA_BYTE5 = 8;
static const uint8_t DATA_BYTE6 = 9;
static const uint8_t DATA_BYTE7 = 10;
static const uint8_t DATA_BYTE8 = 11;

static const uint8_t ADDR_INDOOR_UNIT_1 = 0x20;
static const uint8_t ADDR_INDOOR_UNIT_2 = 0x21;
static const uint8_t ADDR_INDOOR_UNIT_3 = 0x22;
static const uint8_t ADDR_INDOOR_UNIT_4 = 0x23;

static const uint8_t ADDR_OUTDOOR_UNIT_1 = 0xC8;

static const uint8_t ADDR_REMOTE_UNIT_1 = 0x84;

void Samsung_AC_F1F2comComponent::setup() {}

void Samsung_AC_F1F2comComponent::dump_config(){
  ESP_LOGCONFIG(TAG, "Samsung_AC_F1F2com:");
  ESP_LOGCONFIG(TAG, "dataline debug enabled?: %s", this->dataline_debug_ ? "true" : "false");
  this->check_uart_settings(2400, 1, esphome::uart::UART_CONFIG_PARITY_EVEN, 8);
}

void Samsung_AC_F1F2comComponent::update() {
  //publish values for indoor unit 1
  if (this->indoor1_set_temp_sensor_)
    this->indoor1_set_temp_sensor_->publish_state(indoor1_set_temp_);
  if (this->indoor1_room_temp_sensor_)
    this->indoor1_room_temp_sensor_->publish_state(indoor1_room_temp_);
  if (this->indoor1_pipe_in_temp_sensor_)
    this->indoor1_pipe_in_temp_sensor_->publish_state(indoor1_pipe_in_temp_);
  if (this->indoor1_pipe_out_temp_sensor_)
    this->indoor1_pipe_out_temp_sensor_->publish_state(indoor1_pipe_out_temp_);
  if (this->indoor1_fanspeed_sensor_)
    this->indoor1_fanspeed_sensor_->publish_state(indoor1_fanspeed_);
  if (this->indoor1_mode_sensor_)
    this->indoor1_mode_sensor_->publish_state(indoor1_mode_);
  if (this->indoor1_capacity_sensor_)
    this->indoor1_capacity_sensor_->publish_state(indoor1_capacity_);
  if (this->indoor1_delta_q_sensor_)
    this->indoor1_delta_q_sensor_->publish_state(indoor1_delta_q_);
  if (this->indoor1_bladeswing_binary_sensor_)
    this->indoor1_bladeswing_binary_sensor_->publish_state(indoor1_bladeswing_);
  if (this->indoor1_operating_binary_sensor_)
    this->indoor1_operating_binary_sensor_->publish_state(indoor1_operation_);
  
  //publish values for indoor unit 2
  if (this->indoor2_set_temp_sensor_)
    this->indoor2_set_temp_sensor_->publish_state(indoor2_set_temp_);
  if (this->indoor2_room_temp_sensor_)
    this->indoor2_room_temp_sensor_->publish_state(indoor2_room_temp_);
  if (this->indoor2_pipe_in_temp_sensor_)
    this->indoor2_pipe_in_temp_sensor_->publish_state(indoor2_pipe_in_temp_);
  if (this->indoor2_pipe_out_temp_sensor_)
    this->indoor2_pipe_out_temp_sensor_->publish_state(indoor2_pipe_out_temp_);
  if (this->indoor2_fanspeed_sensor_)
    this->indoor2_fanspeed_sensor_->publish_state(indoor2_fanspeed_);
  if (this->indoor2_mode_sensor_)
    this->indoor2_mode_sensor_->publish_state(indoor2_mode_);
  if (this->indoor2_capacity_sensor_)
    this->indoor2_capacity_sensor_->publish_state(indoor2_capacity_);
  if (this->indoor2_delta_q_sensor_)
    this->indoor2_delta_q_sensor_->publish_state(indoor2_delta_q_);
  if (this->indoor2_bladeswing_binary_sensor_)
    this->indoor2_bladeswing_binary_sensor_->publish_state(indoor2_bladeswing_);
  if (this->indoor2_operating_binary_sensor_)
    this->indoor2_operating_binary_sensor_->publish_state(indoor2_operation_);
}

void Samsung_AC_F1F2comComponent::loop() {
  const uint32_t now = millis();
  if (receiving_ && (now - last_transmission_ >= 500)) {
    // last transmission too long ago. Reset RX index.
    ESP_LOGW(TAG, "Last transmission too long ago. Reset RX index.");
    data_.clear();
    receiving_ = false;
  }
    
  if (!available())
    // nothing in uart-input-buffer, end here
    return;
  
  last_transmission_ = now;
  while (available()) {
    uint8_t c;
    read_byte(&c);
    if (c == 0x32 && !receiving_) {//start-byte found
      receiving_ = true;
      data_.clear();
    }
    if (receiving_) {//reading datablock of 14 bytes in progress
      data_.push_back(c);
      if (data_.size() == 14 && c == 0x34) {//endbyte found and lenght of datablock is correct
        receiving_ = false;
        if (check_data_())
        parse_data_();
      }
      else if (data_.size() >= 14) {//no endbyte found at 14th byte
        ESP_LOGW(TAG, "received datablock to long (> 14): %02X %02x %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X",
                 data_[0], data_[1], data_[2], data_[3], data_[4], data_[5], data_[6], data_[7], data_[8], data_[9], data_[10], data_[11], data_[12], data_[13]);
        receiving_ = false;
      }
    }
  }
}

float Samsung_AC_F1F2comComponent::get_setup_priority() const { return setup_priority::DATA; }

bool Samsung_AC_F1F2comComponent::check_data_() const {
  if (dataline_debug_ == true) {
    //print all communication packages when dataline debug is set true
    ESP_LOGD(TAG, "%d: %02X %02x %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X",
             millis(), data_[0], data_[1], data_[2], data_[3], data_[4], data_[5], data_[6], data_[7], data_[8], data_[9], data_[10], data_[11], data_[12], data_[13]);
  }
  if (data_[0] != 0x32) {
    ESP_LOGW(TAG, "unexpected start byte (not 0x32): %d", data_[0]);
    return false;
  }
  //crc berechnen: xor data_[1] bis data_[11]
  int i;
  uint8_t crc = data_[1];
  for (i = 2; i <= 11; i++) {
    crc = crc xor data_[i];
  }
  bool result = false;
  if (crc == data_[12]) result = true;
  if (!result)
    ESP_LOGW(TAG, "data checksum failed! calculated: %02x but received: %02x", crc, data_[12]);
  return result;
}
  
void Samsung_AC_F1F2comComponent::parse_data_() {
  //uncomment next 4 lines to see all packages from indoor1 to outdoor1
  if (data_[DATA_SRC] == ADDR_INDOOR_UNIT_1 && data_[DATA_DST] == ADDR_REMOTE_UNIT_1) { //data from indoor-unit 1 to outdoor-unit
    ESP_LOGD(TAG, "Raw: %02X %02x %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X",
             data_[0], data_[1], data_[2], data_[3], data_[4], data_[5], data_[6], data_[7], data_[8], data_[9], data_[10], data_[11], data_[12], data_[13]);
  }

  //data from indoor-unit 1 to outdoor-unit
  if (data_[DATA_SRC] == ADDR_INDOOR_UNIT_1 && data_[DATA_DST] == ADDR_REMOTE_UNIT_1) {
    //CMD 0x52
    if (data_[DATA_CMD] == 0x52) {
      indoor1_set_temp_ = byte_to_temperature_(data_[DATA_BYTE1] & 0x7F);   // Byte 1: set temp
      indoor1_room_temp_ = byte_to_temperature_(data_[DATA_BYTE2] & 0x7F);  // Byte 2: room temp
      indoor1_pipe_in_temp_ = byte_to_temperature_(data_[DATA_BYTE3] & 0x7F); // Byte 3: output air temp

      indoor1_fanspeed_ = data_[DATA_BYTE4] & 0b00000111; // fan speed bits (0=auto, 2=low, 4=medium, 5=high)
      indoor1_bladeswing_ = ((data_[DATA_BYTE4] & 0b11111000) == 0xF8); // swing mode (0x1A = swing on, 0x1F = off, here generic check)

      indoor1_operation_ = (data_[DATA_BYTE5] & 0b10000000) > 0; // power status (bit 7)
      indoor1_remote_controlled_ = (data_[DATA_BYTE5] & 0b00001111); // bitfield for control type
      ESP_LOGD(TAG, "Indoor Unit 1: Set Temp: %d, Room Temp: %d, Pipe In Temp: %d, Pipe Out Temp: %d, Fan Speed: %d, Mode: %02X, Capacity: %.1f, Delta Q: %d",
               indoor1_set_temp_, indoor1_room_temp_, indoor1_pipe_in_temp_, indoor1_pipe_out_temp_,
               indoor1_fanspeed_, indoor1_mode_, indoor1_capacity_, indoor1_delta_q_);
    }
  }
}

int8_t Samsung_AC_F1F2comComponent::byte_to_temperature_(uint8_t databyte) {
  int8_t temperature = databyte - 55;   //0�C = 55 (0x37)
  return temperature;
}

}  // namespace samsung_ac_f1f2com
}  // namespace esphome
