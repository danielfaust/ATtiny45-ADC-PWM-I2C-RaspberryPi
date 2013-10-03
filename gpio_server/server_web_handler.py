# -*- coding: utf-8 -*-

# 好 <- my Unicode warning character

# beware, there's a lot of testing junk in this file

#--------------------------------------------------
import jinja2
env = jinja2.Environment(loader=jinja2.FileSystemLoader(''))
from twisted.web.static import File
#--------------------------------------------------
import json
import uuid
import urllib2
import glob

import os
import re
import subprocess
import sys
import traceback

#===============================================
import re
import time
import datetime
import dateutil.parser
#===============================================
import pytz
utc = pytz.utc
ber = pytz.timezone('Europe/Berlin')
#-----------------------------------------------
def to_iso8601(when=None, tz=ber):
  if not when:
    import datetime
    when = datetime.datetime.now(tz)
  _when = when.strftime("%Y-%m-%dT%H:%M:%S.%f%z")
  return _when[:-8] + _when[-5:]
#-----------------------------------------------
def from_iso8601(when=None, tz=ber):
  import dateutil.parser
  _when = dateutil.parser.parse(when)
  return _when
#===============================================

##################################################################

def get_child(self, name, request):
  
  if request.path[:8] == '/static/' or request.path[:7] == '/static':
    return File("./static")
  
  elif request.path[:9] == '/scripts/' or request.path[:8] == '/scripts':
    request.setHeader('Content-Type', 'text/javascript; charset=UTF-8')
    request.setHeader('Access-Control-Allow-Origin', '')
    return File(path="./scripts")
  
  return self

import smbus
import math
import RPi.GPIO as GPIO  
def InitGPIO():
  GPIO.setmode(GPIO.BCM)
  GPIO.setwarnings(False)
  GPIO.setup(25, GPIO.OUT)
  IOPort = GPIO.PWM(25, 150)
  IOPort.start(0)
  return IOPort

##################################################################

def render(self, request):
  
  #############################################################################
  if request.path == "/":
    #~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    
    _time = datetime.datetime.now(ber).strftime("%Y-%m-%dT%H:%M:%S.%f%z")
    _time = "".join(re.split('(\.[0-9]{3})[0-9]{3}?', _time))
    print "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~"
    print _time
    
    request.setHeader('Content-Type', 'text/html; charset=UTF-8')
    return env.get_template('index.html').render(time=_time).encode('utf-8')
  
  #############################################################################
  # you care about this
  #############################################################################
  elif request.path == "/i2c":
    #~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    
    if 'counter' not in self.storage:
      self.storage['counter'] = 0
    
    bus = smbus.SMBus(1)
    addr = 0x2F
    try:
      responses = []
      
      ###################################################################################
      
      print 'writing to', addr
      
      ###################################################################################
      
      increment = 1
      #self.storage['counter'] = 0
      if self.storage['counter'] > 255:
        self.storage['counter'] = 32
      
      for i in range(1):
        
        ###################################################################################
        val1 = self.storage['counter']
        self.storage['counter'] += increment
        ###################################################################################
        
        ###################################################################################
        bus.write_byte(addr, 0x06) # read adc on pin 3
        adc0l = bus.read_byte(addr)
        adc0h = bus.read_byte(addr)
        adc0 = (adc0h << 8) + adc0l
        #adc0 = bus.read_word_data(addr, 0x06)
        responses.append(str(adc0))
        ###################################################################################
        
        val1 = adc0/4
        
        ###################################################################################
        '''
        bus.write_byte(addr, 0x02)
        bus.write_byte(addr, val1)
        '''
        bus.write_byte_data(addr, 0x02, val1)
        time.sleep(0.02)
        ###################################################################################
        
        
        ###################################################################################
        '''
        bus.write_byte(addr, 0x04)
        bus.write_byte(addr, 16)
        bus.write_byte(addr, 0)
        time.sleep(0.2)
        bus.write_byte(addr, 0x04)
        bus.write_byte(addr, 4)
        bus.write_byte(addr, 0)
        '''
        bus.write_byte_data(addr, 0x04, 4)
        time.sleep(0.05)
        bus.write_byte_data(addr, 0x04, 0)
        time.sleep(0.05)
        bus.write_byte_data(addr, 0x04, 16)
        time.sleep(0.05)
        bus.write_byte_data(addr, 0x04, 0)
        time.sleep(0.05)
        bus.write_byte_data(addr, 0x04, 8)
        time.sleep(0.05)
        bus.write_byte_data(addr, 0x04, 0)
        '''
        bus.write_byte(addr, 0x04)
        bus.write_byte(addr, 0)
        bus.write_byte(addr, 0)
        '''
        ###################################################################################
        
      print 'done writing to', addr
      ###################################################################################
      return 'ok<br>' + ' | '.join(responses)
      ###################################################################################
      
    except:
      
      time.sleep(0.1)
      bus.write_byte(addr, 0x01)
      bus.write_byte(addr, 0)
      bus.write_byte(addr, 0)
      
      ###################################################################################
      traceback.print_exc()
      return 'err: ' + str('')
      ###################################################################################
      
    return 'ok'
  
  #############################################################################
  # and about this
  #############################################################################
  elif request.path == "/tiny":

    #~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    PWM_value = int(request.args['pwm'][0])
    PWM_address = int(request.args['address'][0])
    #~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    #PWM_value = PWM_value * 100.0
    #if 'gpio' not in self.storage:
    #  self.storage['gpio'] = InitGPIO()
    #self.storage['gpio'].ChangeDutyCycle(PWM_value)
    #~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    bus = smbus.SMBus(1)
    addr = 0x2F
    '''
    bus.write_byte(addr, PWM_address & 0xFF);
    bus.write_byte(addr, PWM_value)
    '''
    bus.write_byte_data(addr, PWM_address, PWM_value)
    #os.system('i2cset -y 1 0x2F ' + str(PWM_address) + ' ' + str(PWM_value) + ' b')
    #~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    return ''
    #~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  
  #############################################################################
  elif request.path == "/push":

    #~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    PWM_value = float(request.args['pwm'][0])
    PWM_value = PWM_value / 100.0
    #~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    #PWM_value = PWM_value * 100.0
    #if 'gpio' not in self.storage:
    #  self.storage['gpio'] = InitGPIO()
    #self.storage['gpio'].ChangeDutyCycle(PWM_value)
    #~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    PWM_value = math.pow(PWM_value,2)
    #~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    PWM_value = '{0:.4f}'.format(PWM_value)
    f = open('/dev/pi-blaster', 'w')
    f.write('1='+PWM_value+'\n')
    f.close()
    #print PWM_value
    return ''
    #~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  
  #############################################################################
  elif request.path == "/omx":
    p = subprocess.Popen(["omxplayer", "/home/pi/Desktop/rooster.mp3"], stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    return ''
    #~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  #############################################################################
  elif request.path == "/snd":
    #~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    PWM_pin = 23
    GPIO.cleanup()
    GPIO.setmode(GPIO.BCM)
    GPIO.setwarnings(False)
    GPIO.setup(PWM_pin, GPIO.OUT)
    IOPort = GPIO.PWM(PWM_pin, 100)
    #for i in range(1000,4000,100):
    #  print i
    #  IOPort.ChangeFrequency(i)
    #  time.sleep(0.25)
    
    PWM_duration = 0.025
    PWM_pause = 0.075
    
    tones = [(4000,1), (8000,1), (16000,1)]
    tones = [(4000,1), (8000,1), (32000,1)]
    tones = [(4000,10), (4000,1)]
    tones = [(32000,10), (8000,1), (4000,1)]
    for tone in tones:
      IOPort.ChangeFrequency(tone[0])
      IOPort.start(50.0)
      time.sleep(PWM_duration*tone[1])
      IOPort.stop()
      time.sleep(PWM_pause)
    
    
    GPIO.cleanup()
    print 'ok'
    
    #~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    '''
    PWM_value = '{0:.4f}'.format(PWM_value)
    f = open('/dev/pi-blaster', 'w')
    f.write('1='+PWM_value+'\n')
    f.close()
    print PWM_value
    '''
    return ''
    #~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  
  #############################################################################
  else:
    #~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    
    _time = datetime.datetime.now(ber).strftime("%Y-%m-%dT%H:%M:%S.%f%z")
    _time = "".join(re.split('(\.[0-9]{3})[0-9]{3}?', _time))
    print "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~"
    print _time
    
    request.setHeader('Content-Type', 'text/plain')
    request.setHeader('Server', 'Test')
    ret  = ""
    ret += str(request.method) + "\n"
    ret += str(request.host) + "\n"
    ret += str(request.client) + "\n"
    ret += str(request.client.host) + "\n"
    ret += str(request.path) + "\n"
    ret += str(request.args) + "\n"
    ret += str(request.received_headers) + "\n"
    print ret
    return ret + str(request.getAllHeaders())

##################################################################
