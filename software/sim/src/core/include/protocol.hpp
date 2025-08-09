#pragma once

#include <cstdint>
#include <vector>
#include <array>

// ===================================================================
// UART-LIKE PROTOCOL IMPLEMENTATION
// ===================================================================

namespace Protocol {
    
    // Protocol constants
    static constexpr uint8_t START_BYTE = 0xAA;
    static constexpr uint8_t STOP_BYTE = 0x55;
    static constexpr uint32_t MAX_DATA_BLOCKS = 32;
    static constexpr uint32_t RESPONSE_TIMEOUT_MS = 15;
    static constexpr uint32_t ERROR_TIMEOUT_MS = 30;
    
    // Module IDs (5 bits, max 31 modules)
    enum class ModuleID : uint8_t {
        CVS = 0x01,
        SES = 0x02, 
        DS  = 0x03,
        OS  = 0x04,
        VIS = 0x05,
        BROADCAST = 0x1F  // All modules
    };
    
    // Command codes (3 bits, max 8 commands)
    enum class CommandCode : uint8_t {
        REQUEST_DATA = 0x00,        // Request data from module
        EXECUTE_COMMAND = 0x01,     // Execute command
        DATA_RESPONSE = 0x02,       // Data response
        COMMAND_ACK = 0x03,         // Command acknowledgment
        ERROR_RESPONSE = 0x04,      // Error response
        PING = 0x05,                // Ping request
        PONG = 0x06,                // Ping response
        RESERVED = 0x07             // Reserved for future use
    };
    
    // Message header structure (16 bits)
    union MessageHeader {
        uint16_t raw;
        struct {
            uint16_t device_id : 5;      // Device ID (0-31)
            uint16_t command : 3;        // Command code (0-7)
            uint16_t data_blocks : 5;    // Number of data blocks (0-31)
            uint16_t flags : 3;          // Reserved flags
        } fields;
        
        MessageHeader() : raw(0) {}
        MessageHeader(ModuleID id, CommandCode cmd, uint8_t blocks = 0, uint8_t flag = 0) {
            fields.device_id = static_cast<uint8_t>(id);
            fields.command = static_cast<uint8_t>(cmd);
            fields.data_blocks = blocks;
            fields.flags = flag;
        }
    };
    
    // Data block (32 bits)
    union DataBlock {
        uint32_t raw;
        
        // Different interpretations of the same 32 bits
        struct {
            uint8_t byte0, byte1, byte2, byte3;
        } bytes;
        
        struct {
            uint16_t word0, word1;
        } words;
        
        struct {
            float value;
        } float_val;
        
        // Discrete bits (32 individual flags)
        struct {
            uint32_t bit0  : 1, bit1  : 1, bit2  : 1, bit3  : 1,
                     bit4  : 1, bit5  : 1, bit6  : 1, bit7  : 1,
                     bit8  : 1, bit9  : 1, bit10 : 1, bit11 : 1,
                     bit12 : 1, bit13 : 1, bit14 : 1, bit15 : 1,
                     bit16 : 1, bit17 : 1, bit18 : 1, bit19 : 1,
                     bit20 : 1, bit21 : 1, bit22 : 1, bit23 : 1,
                     bit24 : 1, bit25 : 1, bit26 : 1, bit27 : 1,
                     bit28 : 1, bit29 : 1, bit30 : 1, bit31 : 1;
        } bits;
        
        DataBlock() : raw(0) {}
        DataBlock(uint32_t value) : raw(value) {}
        DataBlock(float value) : float_val{value} {}
    };
    
    // Complete message structure
    struct Message {
        uint8_t start_byte;
        MessageHeader header;
        std::vector<DataBlock> data_blocks;
        uint8_t stop_byte;
        
        Message() : start_byte(START_BYTE), stop_byte(STOP_BYTE) {}
        
        // Create header-only message
        Message(ModuleID target_id, CommandCode cmd, uint8_t flags = 0) 
            : start_byte(START_BYTE), header(target_id, cmd, 0, flags), stop_byte(STOP_BYTE) {}
        
        // Create message with data
        Message(ModuleID target_id, CommandCode cmd, const std::vector<DataBlock>& data, uint8_t flags = 0)
            : start_byte(START_BYTE), header(target_id, cmd, data.size(), flags), 
              data_blocks(data), stop_byte(STOP_BYTE) {
            // Ensure we don't exceed maximum blocks
            if (data_blocks.size() > MAX_DATA_BLOCKS) {
                data_blocks.resize(MAX_DATA_BLOCKS);
            }
            header.fields.data_blocks = data_blocks.size();
        }
        
        // Serialize to byte array
        std::vector<uint8_t> serialize() const {
            std::vector<uint8_t> result;
            result.reserve(4 + data_blocks.size() * 4); // start + header + data + stop
            
            // Start byte
            result.push_back(start_byte);
            
            // Header (16 bits = 2 bytes, little endian)
            result.push_back(header.raw & 0xFF);
            result.push_back((header.raw >> 8) & 0xFF);
            
            // Data blocks (each 32 bits = 4 bytes, little endian)
            for (const auto& block : data_blocks) {
                result.push_back(block.raw & 0xFF);
                result.push_back((block.raw >> 8) & 0xFF);
                result.push_back((block.raw >> 16) & 0xFF);
                result.push_back((block.raw >> 24) & 0xFF);
            }
            
            // Stop byte
            result.push_back(stop_byte);
            
            return result;
        }
        
        // Deserialize from byte array
        static bool deserialize(const std::vector<uint8_t>& data, Message& result) {
            if (data.size() < 4) return false; // Minimum: start + header + stop
            
            size_t pos = 0;
            
            // Check start byte
            if (data[pos++] != START_BYTE) return false;
            
            // Parse header
            if (pos + 1 >= data.size()) return false;
            result.header.raw = data[pos] | (data[pos + 1] << 8);
            pos += 2;
            
            // Parse data blocks
            uint8_t expected_blocks = result.header.fields.data_blocks;
            if (pos + expected_blocks * 4 + 1 != data.size()) return false; // Check total size
            
            result.data_blocks.clear();
            result.data_blocks.reserve(expected_blocks);
            
            for (uint8_t i = 0; i < expected_blocks; ++i) {
                if (pos + 3 >= data.size()) return false;
                
                uint32_t block_value = data[pos] | 
                                     (data[pos + 1] << 8) | 
                                     (data[pos + 2] << 16) | 
                                     (data[pos + 3] << 24);
                result.data_blocks.emplace_back(block_value);
                pos += 4;
            }
            
            // Check stop byte
            if (pos >= data.size() || data[pos] != STOP_BYTE) return false;
            
            result.start_byte = START_BYTE;
            result.stop_byte = STOP_BYTE;
            
            return true;
        }
        
        // Utility methods
        bool isValid() const {
            return start_byte == START_BYTE && 
                   stop_byte == STOP_BYTE && 
                   data_blocks.size() == header.fields.data_blocks &&
                   data_blocks.size() <= MAX_DATA_BLOCKS;
        }
        
        ModuleID getTargetModule() const {
            return static_cast<ModuleID>(header.fields.device_id);
        }
        
        CommandCode getCommand() const {
            return static_cast<CommandCode>(header.fields.command);
        }
        
        uint8_t getDataBlockCount() const {
            return header.fields.data_blocks;
        }
        
        uint8_t getFlags() const {
            return header.fields.flags;
        }
    };
    
    // Helper functions for common operations
    
    // Create PING message
    inline Message createPingMessage(ModuleID target) {
        return Message(target, CommandCode::PING);
    }
    
    // Create PONG response
    inline Message createPongMessage(ModuleID target) {
        return Message(target, CommandCode::PONG);
    }
    
    // Create data request
    inline Message createDataRequest(ModuleID target) {
        return Message(target, CommandCode::REQUEST_DATA);
    }
    
    // Create command execution request
    inline Message createCommandRequest(ModuleID target, const std::vector<DataBlock>& command_data) {
        return Message(target, CommandCode::EXECUTE_COMMAND, command_data);
    }
    
    // Create data response
    inline Message createDataResponse(ModuleID target, const std::vector<DataBlock>& response_data) {
        return Message(target, CommandCode::DATA_RESPONSE, response_data);
    }
    
    // Create command acknowledgment
    inline Message createCommandAck(ModuleID target, bool success = true) {
        DataBlock ack_data;
        ack_data.bits.bit0 = success ? 1 : 0;
        return Message(target, CommandCode::COMMAND_ACK, {ack_data});
    }
    
    // Create error response
    inline Message createErrorResponse(ModuleID target, uint8_t error_code) {
        DataBlock error_data;
        error_data.bytes.byte0 = error_code;
        return Message(target, CommandCode::ERROR_RESPONSE, {error_data});
    }
    
} // namespace Protocol