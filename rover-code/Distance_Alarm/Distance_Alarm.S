.section .text

.global set_output_enable, trigger_pin, reset_pin #basically extern to c++

.type set_output_enable, @function
.type trigger_pin, @function
.type reset_pin, @function

set_output_enable: #setting the pin 1 - trigger as output
  lui t0, 0x60004 #base address for GPIO registers
  addi t0, t0, 0x020 #offset for the GPIO_ENABLE_REG to enable output, input is by default

  li t1, 0x2 # the specific pin numero 1
  sw t1, 0(t0) #set the pin, but unfortunately now it's overwritting sorta the rest.

  ret

trigger_pin: #sending a high for 10us then low to trigger the signal
  lui t0, 0x60004
  addi t0, t0, 0x004 #GPIO_OUT_REG address
  
  li t1, 0x2 #pin number 1
  sw t1, 0(t0)

  li t2, 0 #starting the delay loop, 16MHz clock
  li t3, 160 # around 10us?

delay_loop:
  addi t2, t2, 1
  blt t2, t3, delay_loop

reset_pin: #sending a low signal
  lui t0, 0x60004
  addi t0, t0, 0x004 #GPIO_OUT_REG address

  li t1, 0x00 #kinda overwritting all...
  sw t1, 0(t0)

  ret