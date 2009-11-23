import urllib2
from urllib2 import urlopen

class VertexDb:
    
    def __init__(self, host="http//localhost:8080"):
        self.host = host
    
    def __str__(self):
        return "vertexdb at %s" % self.host
    
    def write(self, path, key, val):
        return urlopen("%s%s%s%s" % (self.host, path, "/?action=write&key=", key), val).read()
        
    def read(self, path, key):
        return urlopen("%s%s%s%s" % (self.host, path, "/?action=read&key=", key)).read()
        
    def mkdir(self, path):
      return urlopen("%s%s%s" % (self.host, path, "/?action=mkdir")).read()

    def size(self, path):
        """docstring for size"""
        return urlopen("%s%s%s" % (self.host, path, "/?action=size")).read()

vdb = VertexDb("http://localhost:8080")
print vdb.size("/")
vdb.mkdir("/test/")
vdb.write("/test/", "akey", "avalue")
print vdb.read("/test/", "akey")

# Implement protocols so that nodes can be used as dictionaries etc?
# Look at fs APIs.