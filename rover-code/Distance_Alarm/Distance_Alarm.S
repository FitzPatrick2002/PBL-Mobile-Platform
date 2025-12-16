.section .text

.global set_output_enable, trigger_pin, reset_pin, trigger_pin_dos, reset_pin_dos, led_high, led_low #basically extern to c++

.type set_output_enable, @function
.type trigger_pin, @function
.type reset_pin, @function
.type trigger_pin_dos, @function
.type reset_pin_dos, @function
.type led_high, @function
.type led_low, @function

set_output_enable: #setting the pin 1 - trigger as output
  lui t0, 0x60004 #base address for GPIO registers
  addi t0, t0, 0x024 #offset for the GPIO_ENABLE_REG to enable output, input is by default

  lw t1, 0(t0) # in order not to overwrite, read the current value
  ori t1, t1, 0x2A #pin number 1
  #li t1, 0x2 #pin number 1
  sw t1, 0(t0)

  #li t1, 0x2A # pins 1,3,5 (5 does not work, unsure as to why)
  #sw t1, 0(t0) #set the pin, but unfortunately now it's overwritting sorta the rest.

  ret

trigger_pin: #sending a high for 10us then low to trigger the signal
  lui t0, 0x60004
  addi t0, t0, 0x004 #GPIO_OUT_REG address
  
  lw t1, 0(t0) # in order not to overwrite, read the current value
  ori t1, t1, 0x02 #pin number 1
  #li t1, 0x2 #pin number 1
  sw t1, 0(t0)

  li t2, 0 #starting the delay loop, 16MHz clock
  li t3, 160 # around 10us?

delay_loop:
  addi t2, t2, 1
  blt t2, t3, delay_loop

reset_pin: #sending a low signal
  lui t0, 0x60004
  addi t0, t0, 0x004 #GPIO_OUT_REG address

  lw t1, 0(t0) # in order not to overwrite, read the current value
  li t2, 0x02 #mask for pin 1
  not t2, t2 #invert 
  and t1, t1, t2 #reset only pin 1
  sw t1, 0(t0)

  ret

trigger_pin_dos: #sending a high for 10us then low to trigger the signal
  lui t0, 0x60004
  addi t0, t0, 0x004 #GPIO_OUT_REG address
  
  lw t1, 0(t0) # in order not to overwrite, read the current value
  ori t1, t1, 0x08 #pin number 3
  sw t1, 0(t0)

  li t2, 0 #starting the delay loop, 16MHz clock
  li t3, 160 # around 10us?

delay_loop_dos:
  addi t2, t2, 1
  blt t2, t3, delay_loop_dos #fallback later

reset_pin_dos: #sending a low signal
  lui t0, 0x60004
  addi t0, t0, 0x004 #GPIO_OUT_REG address

  lw t1, 0(t0) # in order not to overwrite, read the current value
  li t2, 0x08 #mask for pin 3
  not t2, t2 #invert 
  and t1, t1, t2 #reset only pin 3
  sw t1, 0(t0)

  ret

led_high: #works
  lui t0, 0x60004
  addi t0, t0, 0x004 #GPIO_OUT_REG address
  
  lw t1, 0(t0) # in order not to overwrite, read the current value
  ori t1, t1, 0x20 #pin number 5
  sw t1, 0(t0)
  ret

led_low: #works
  lui t0, 0x60004
  addi t0, t0, 0x004 #GPIO_OUT_REG address

  lw t1, 0(t0) # in order not to overwrite, read the current value
  li t2, 0x20 #mask for pin 5
  not t2, t2 #invert 
  and t1, t1, t2 #reset only pin 5
  sw t1, 0(t0)

  ret