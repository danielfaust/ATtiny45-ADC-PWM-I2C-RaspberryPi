

if 'counter' not in self.storage:
  self.storage['counter'] = 0

import smbus
bus = smbus.SMBus(1)
addr = 0x2F

try:
  
  responses = []
  
  print 'writing to', addr
  
  increment = 1
  #self.storage['counter'] = 0
  if self.storage['counter'] > 255:
    self.storage['counter'] = 32
  
  for i in range(1):
    time.sleep(0.01)
    val1 = self.storage['counter']
    self.storage['counter'] += increment
    ###################################################################################
    bus.write_byte(addr, 0x01)
    bus.write_byte(addr, val1)
    bus.write_byte(addr, val1 >> 8)
    ###################################################################################
    bus.write_byte(addr, 0x02)
    bus.write_byte(addr, val1)
    bus.write_byte(addr, val1 >> 8)
    ###################################################################################
    time.sleep(0.01)
    adc0 = bus.read_word_data(addr, 0x03)
    responses.append(str(adc0))
    ###################################################################################
    time.sleep(0.01)
    adc1 = bus.read_word_data(addr, 0x04)
    responses.append(str(adc1))
    ###################################################################################
  
  return 'ok<br>' + ' | '.join(responses)
  
except:
  
  traceback.print_exc()
  return 'err: ' + str('')
