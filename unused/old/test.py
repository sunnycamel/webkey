#!/usr/bin/python
import socket
import time
import struct
#import random
from threading import Thread
from threading import Condition

f = None


class phone:
    def __init__(self,u,r,c=None,a=None,p=None):
        self.lock = Condition()
        self.connlist = []
            #print "new phone, length: ",len(self.connlist)
        self.username = u
        self.random = r
        self.lastknown = None
        self.redirect = None
        if c != None:
            self.appendconn(c,a,p)
    def appendconn(self,c,a,p):
        self.lock.acquire()
        try:
            c.setsockopt( socket.SOL_SOCKET, socket.SO_KEEPALIVE, 1)
            c.setsockopt( socket.SOL_TCP, socket.TCP_KEEPCNT, 6)
            c.setsockopt( socket.SOL_TCP, socket.TCP_KEEPIDLE, 180)
            c.setsockopt( socket.SOL_TCP, socket.TCP_KEEPINTVL, 10)
        except:
            print "error in setting up keepalive"
        self.connlist.append(c)
        if len(self.connlist) > 3:
            self.connlist.pop(0).close()
        if self.lastknown != a[0] + ":" + p:
            #print "not lastknownip"
            #print "lastknown:", self.lastknown, "new: ",a[0] + ":" + p
            while len(self.connlist) > 1:
                self.connlist.pop(0).close()
            self.lastknown = a[0] + ":" + p
            self.redirect = None
            testthread(a[0],p,self).start()
        if self.redirect:
            c.sendall("stop")
            while len(self.connlist):
                self.connlist.pop().close()
        self.lock.notifyAll()
        self.lock.release()
    def getconn(self):
        exptime = time.time()+10
        self.lock.acquire()
        while len(self.connlist) == 0:
            #print "waiting"
            try:
                self.lock.wait(10)
            except RuntimeError:
                self.lock.release()
                return None
            if exptime < time.time():
                break
        if len(self.connlist) == 0:
#            print "Got signal, but no connection"
            log("Got signal, but no connection\n",self.username)
            self.lock.release()
            return None
        c = self.connlist.pop(0)
#        print "removing c, length: ",len(self.connlist)
        self.lock.release()
        return c
    def close(self):
        self.lock.acquire()
        while len(self.connlist):
            self.connlist.pop().close()
        self.lock.notifyAll()
        self.lock.release()


def t():
    return str(time.localtime()[0:6])

def log(s,username=None):
    global f
    if s[-1] != "\n":
        s += "\n"
    if username:
        try:
            tf = open("webkeyusername_"+username+".txt",'a')
            tf.write(s)
            tf.close(s)
        except:
            pass
    else:
        try:
            f.write(s)
            f.flush()
        except:
            try:
                f.close()
            except:
                pass
            try:
                f = open("webkey_log.txt",'a');
            except:
                pass


class testthread(Thread):
    def __init__(self,addr,port,phone):
        Thread.__init__(self)
        self.addr = addr
        self.port = port
        self.phone = phone
    def run(self):
        try:
            sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            sock.connect((self.addr,int(self.port)))
            sock.sendall("GET /test HTTP/1.1\r\nHOST: anything\r\n\r\n")
            d = sock.recv(4096)
            sock.close()
            if d=="Webkey":
                if self.port == "80":
                    self.phone.redirect = self.addr
                else:
                    self.phone.redirect = self.addr + ":" + self.port
                log("it can be redirected to "+self.phone.redirect,self.phone.username)
        except:
            pass

class connectionthread(Thread):
    def __init__(self,c,a,s):
        Thread.__init__(self)
        self.conn = c
        self.phones = s
        self.address = a
    def run(self):
        #print "started phonethread "
        try:
            firstline = self.conn.recv(4096)
        except:
            self.conn.close()
            return
        if firstline.startswith("GET /register_") or firstline.startswith("WEBKEY"):
            self.phoneclient(firstline)
        elif firstline.startswith("GET / ") or firstline.startswith("GET /index.html") \
                or firstline.startswith("GET /html/") or firstline.startswith("GET /robots.txt")\
                or firstline.startswith("GET /favicon.ico"):
            self.lighttpdclient(firstline)

        else:
            self.browserclient(firstline)

    def trysendall(self,data):
        try:
            self.conn.sendall(data)
        except:
            return False
        return True

    def phoneclient(self,firstline):
        l = t() + " "
        p = firstline.find("/")
        if p == -1: self.conn.close(); return
        #print firstline
        if firstline.startswith("GET /register_"):
            p = firstline[14:].find("/")+14
            q = firstline[p+1:].find("/")+p+1
            e = firstline[q+1:].find(" ") + q+1

            username = firstline[14:p]
            random = firstline[p+1:q]
            version = firstline[q+1:e]
            l+="register, username = "+username+", random = "+random+", version = "+version+ " " ;
            if len(username) == 0:
                self.trysendall("HTTP/1.1 200 OK\r\n\r\nEmpty username is not allowed.")
                l+="Emtpy username is not allowed "
            elif username in self.phones and self.phones[username].random != random:
                self.trysendall("HTTP/1.1 200 OK\r\n\r\nUsername is already used.")
                l+="Username is already used. "
            else:
                if not username in self.phones:
                    self.phones[username] = phone(username,random)
                    l+="really new user, "
                self.trysendall("HTTP/1.1 200 OK\r\n\r\nOK")
                l+="OK"
            self.conn.close()
            log(l,username)
            return


        username = firstline[7:p]
        q = firstline[p+1:].find("/")+p+1
        g = firstline[q+1:].find("/")+q+1
        e = firstline[g+1:].find("\r") + g+1
        random = firstline[p+1:q]
        version = firstline[q+1:g]
        port = firstline[g+1:e]

        print "username from phone =",username
        l+="username = "+username+", random = "+random+", version = "+version+", port = "+port+" ";
        if not username or not random: 
            l+="error, empty username or random\n"
            self.trysendall("stop")
            self.conn.close(); 
            log(l,username)
            return
        if username in self.phones and self.phones[username].random == random:
            l+="all ok"
            self.phones[username].appendconn(self.conn,self.address,port)
        elif not username in self.phones:
            l+="new phone"
            self.phones[username] = phone(username,random,self.conn,self.address,port)
        else:
            l+="wrong username"
            self.trysendall("stop")
            log(l,username)
            self.conn.close(); 
            return
        l+="\n"
        log(l,username)

    def browserclient(self,firstline):
        #print "started clientthread "
        data = firstline
        l = t() + " "
        data2 = ""
        while 1:
            e = data.find("\r\n\r\n")
            if e != -1:
                data2 = data[e+4:]
                data = data[:e+4]
                break
            dl = len(data)
            try:
                data += self.conn.recv(4096)
            except:
                self.conn.close()
                return
            if dl == len(data):
                #print data
                print "no endline, exiting"
                l += "no endline, exiting"
                log(l)
                self.conn.close()
                return;
            #print [ord(s) for s in data[-4:]]
        p = data.find("Content-Length:")
        if p != -1:
            i = p+16
            while i < len(data) and '0' <= data[i] <= '9':
                i+=1;
            cl = int(data[p+16:i])
            cl = min(cl,32*1024*1024) #There is a limit
            while 1:
                dl = len(data2)
                if len(data2) >= cl:
                    break
                try:
                    data2 += self.conn.recv(4096)
                except:
                    return
                if dl == len(data2):
                    l+= "connection freezed reading data2"
                    self.conn.close()
                    log(l)
                    return
        data += data2
        if not data:
            self.conn.close()
            return
        l += data[:data.find('\r')]+" "
        #l+="datalength = "+str(len(data))+" "
        username = ""
        prefix = 0
        if data[:5]=="GET /":
            prefix = 5
        elif data[:6]=="POST /":
            prefix = 6
        else:
            self.conn.close()
            return
        pos = len(data)
        if data[prefix:].find('/') != -1:
            pos = data[prefix:].find('/')
        if data[prefix:].find(' ') != -1:
            pos = min(pos,data[prefix:].find(' '))
        if pos==len(data):
            self.conn.close()
            return
        username = data[prefix:pos+prefix]
        l+="username = "+username+" "
        if not username in self.phones:
            self.trysendall("HTTP/1.1 404 Not Found\r\n\r\nPhone is not in the database. Is it online?\r\n")
            self.conn.close()
            l+="Phone is not in the database\n"
            log(l,username)
            return
        if data[pos+prefix] != '/':
            self.trysendall("HTTP/1.1 200 OK\r\n\r\n<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Strict//EN\"\
                    \"http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd\"><html xmlns=\"http://www.w3.org/1999/xhtml\" xml:lang=\"hu\" lang=\"hu\">\
                            <head> <title>Please put a slash at the end of the link. Redirecting...</title>\
                                                            <meta http-equiv=\"REFRESH\" content=\"0;url="+username+"/\"></head>\
                                                                    <body></body></html>\r\n")
            l+="not a phone address, redirected.\n"
            log(l,username)
            self.conn.close()
            return
        data = data[:prefix]+data[pos+prefix+1:]
        if self.phones[username].redirect:
            self.trysendall("HTTP/1.1 200 OK\r\n\r\n<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Strict//EN\"\
                    \"http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd\"><html xmlns=\"http://www.w3.org/1999/xhtml\" xml:lang=\"hu\" lang=\"hu\">\
                            <head> <title>Your phone has a public IP, redirecting...</title>\
                            <meta http-equiv=\"REFRESH\" content=\"1;url=http://"+self.phones[username].redirect+"/\"></head>\
                            <body></body>Your phone has a public IP, redirecing to <a href=\"http://"+self.phones[username].redirect+"/>http://"+self.phones[username].redirect+"/</a></html>\r\n")
            l+="it has a public ip, redirected.\n"
            log(l,username)
            self.conn.close()
            return
        c=None
        while 1:
            c = self.phones[username].getconn()
            if c == None:
                self.trysendall("HTTP/1.1 404 Not Found\r\n\r\nPhone is not online\r\n")
                self.conn.close()
                l+="Phone is not in the database.\n"
                log(l,username)
                return
            try:
                c.sendall(data)
                break
            except:
                print "sendall error"
                c.close()
                l+="Using other connection, "
        s = 0
        while 1:
            data = None
            try:
                data = c.recv(4096)
                s += len(data)
                if not data: break
                self.trysendall(data)
            except:
                break
        self.conn.close()
        c.close()
        l+="sent "+str(s)+" bytes\n"
        log(l,username)

    def lighttpdclient(self,firstline):
        #print "started clientthread "
        data = firstline
        l = t() + " lighttpd "
        data2 = ""
        while 1:
            e = data.find("\r\n\r\n")
            if e != -1:
                data2 = data[e+4:]
                data = data[:e+4]
                break
            dl = len(data)
            try:
                data += self.conn.recv(4096)
            except:
                self.conn.close()
                return
            if dl == len(data):
                #print data
                print "no endline, exiting"
                l += "no endline, exiting"
                log(l)
                self.conn.close()
                return;
            print [ord(s) for s in data[-4:]]
        p = data.find("Content-Length:")
        if p != -1:
            i = p+16
            while i < len(data) and '0' <= data[i] <= '9':
                i+=1;
            cl = int(data[p+16:i])
            cl = min(cl,32*1024*1024) #There is a limit
            while 1:
                dl = len(data2)
                if len(data2) >= cl:
                    break
                try:
                    data2 += self.conn.recv(4096)
                except:
                    return
                if dl == len(data2):
                    l+= "connection freezed reading data2"
                    self.conn.close()
                    log(l)
                    return
        data += data2
        if not data:
            self.conn.close()
            return
        l += data[:data.find('\r')]+" "
        c = None
        try:
            c = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            c.connect(("127.0.0.1",81))
            c.sendall(data)
        except:
            self.conn.close()
            c.close()
            l += "connection to lighttp refused"
            log(l)
            return
        s = 0
        while 1:
            data = None
            try:
                data = c.recv(4096)
                s += len(data)
                if not data: break
                self.trysendall(data)
            except:
                break
        self.conn.close()
        c.close()
        l+="sent "+str(s)+" bytes\n"
        log(l)


HOST = ''
PORT = 8080

#client
clientsock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
clientsock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
#clientsock.setblocking(0)
clientsock.bind((HOST, PORT))
clientsock.listen(2500)
print "server started"

phones = dict()
try:
    db = open("webkeyusers.txt","r");
    for x in db.readlines():
        l = x[:-1].split(' ')
        if len(l) == 2:
            phones[l[0]] = phone(l[0],l[1])
            print "loaded user: "+l[0]
    db.close()
except:
    pass

i = 0
f = open("webkey_log.txt",'a');
log(t()+" Server started");
while 1:
    try:
        conn, addr = clientsock.accept()
#        print 'Connected by', addr
#        conn.setsockopt( socket.SOL_SOCKET, socket.SO_SNDTIMEO, struct.pack('ii',30,0))
        log(str(i) + ". " + t() + " client connected from " + str(addr) + "\n")
        connectionthread(conn,addr,phones).start()
        i+=1
        #print i
    except KeyboardInterrupt:
        try:
            f.close()
        except:
            pass
        break

print "stopping"
db = open("webkeyusers.txt","w");
for username in phones:
    db.write(username+" "+phones[username].random+"\n")
db.close();

clientsock.shutdown(socket.SHUT_RDWR)
clientsock.close()
[phones[p].close() for p in phones]
