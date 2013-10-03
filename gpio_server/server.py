# -*- coding: utf-8 -*-

import re
import sys
#===============================================
import time
import datetime
import dateutil.parser
import pytz
utc = pytz.utc
ber = pytz.timezone('Europe/Berlin')
#===============================================

from twisted.python import log
from twisted.internet import reactor
from twisted.python import usage, log
from twisted.internet.task import LoopingCall
from twisted.web.server import Site
from twisted.web.resource import Resource
from twisted.web.static import File

##########################################
#---------- Website

import server_web_handler
class Website(Resource):
  
  isLeaf = False
  addSlash = True
  
  def __init__(self):
    self.storage = {}
    Resource.__init__(self)
  
  def getChild(self, name, request):
    try:
      reload(server_web_handler)
      return server_web_handler.get_child(self, name, request)
    except:
      import traceback
      traceback.print_exc()
      return self
  
  def render(self, request):
    try:
      return server_web_handler.render(self, request)
    except Exception, e:
      import traceback
      traceback.print_exc()
      raise

#---------- Website
##########################################

print 'starting site'

if __name__ == '__main__':
  
  web = Site(Website())
  reactor.listenTCP(8080, web)
  reactor.run()
