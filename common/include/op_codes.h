#ifndef OP_CODES_H_
#define OP_CODES_H_
/*
 * op_codes.h
 * List of commands for the communication.
 * Since it does not make sense to have all pneumatic output on, the commands only allow one at a time
 *
 * Created: 06/05/2024 10:32:34
 *  Author: micro
 */ 

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Command values
 * Each command is selected such that there are a even mix of 1 and 0s whilst
 *  using the highest hamming distance
 */
typedef enum {
   opcodes_cmd_error            = 0,
   opcodes_cmd_idle             = 0b01001111, // 4f
   opcodes_cmd_push_door        = 0b01110001, // 71
   opcodes_cmd_pull_door        = 0b00001010, // 09
   opcodes_cmd_blast_toolsetter = 0b01111010, // 75
   opcodes_cmd_blast_spindle    = 0b01001000, // 48
   opcodes_cmd_unclamp_chuck    = 0b10000110, // 86
   opcodes_cmd_reserved0        = 0b11001001, // C9
   opcodes_cmd_reserved1        = 0b10110000, // B0
} opcodes_cmd_t;

/**
 * Possible types of reply
 */
typedef enum {
   opcodes_reply_off,
   opcodes_reply_on,
   opcodes_reply_error,
} opcodes_reply_t;


/** Mask applied to the input to indicate the input is ON */
#define OPCODE_INPUT_IS_ON_MASK 0xA5
#define OPCODE_INPUT_IS_OFF_MASK 0x5A

/** 
 * @return The opcode if the opcode is a valid command or opcodes_cmd_error (0)
 */
static inline opcodes_cmd_t opcodes_check_cmd_valid( uint8_t value )
{
   if ( 
      value == opcodes_cmd_idle || 
      value == opcodes_cmd_push_door ||
      value == opcodes_cmd_pull_door ||
      value == opcodes_cmd_blast_toolsetter ||
      value == opcodes_cmd_blast_spindle ||
      value == opcodes_cmd_unclamp_chuck ||
      value == opcodes_cmd_reserved0 ||
      value == opcodes_cmd_reserved1
      )
   {
      return (opcodes_cmd_t)value;
   }
   
   return opcodes_cmd_error;
}

/**
 * Extract the value
 * @return A reply with the value. The value may indicate a communication error
 */
static inline opcodes_reply_t opcodes_decode_reply(opcodes_cmd_t cmd, uint8_t value_read)
{
   uint8_t value_writen = (uint8_t)cmd;
   
   if ( (value_writen ^ OPCODE_INPUT_IS_ON_MASK) == value_read )
   {
      return opcodes_reply_on;
   }
   else if ( (value_writen ^ OPCODE_INPUT_IS_OFF_MASK) == value_read )
   {
      return opcodes_reply_off;
   }
   
   return opcodes_reply_error;
}

/** Create the reply */
static inline uint8_t opcodes_encode_reply(opcodes_reply_t reply, opcodes_cmd_t cmd_received)
{
   switch (reply)
   {
   case opcodes_reply_off:
      return cmd_received ^ OPCODE_INPUT_IS_OFF_MASK;
   case opcodes_reply_on:
      return cmd_received ^ OPCODE_INPUT_IS_ON_MASK;
   default:
      break;
   }

   return (uint8_t)opcodes_cmd_error;
}


#ifdef __cplusplus
}
#endif

#endif /* OP_CODES_H_ */