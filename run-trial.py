#!/usr/bin/python

from mininet.topo import Topo
from mininet.net import Mininet
from mininet.node import Host
from mininet.link import Link
from mininet.cli import CLI
from mininet.util import dumpNodeConnections
from mininet.util import ensureRoot

from subprocess import Popen, PIPE
from time import sleep, time

import sys
import os
import math
import requests
from optparse import OptionParser

parser = OptionParser()
parser.add_option('--maxw', dest="maxw",
                    type=int,
                    default=1)
parser.add_option('--maxd', dest="maxd",
                    type=int,
                    default=1000)
(options, args)=parser.parse_args()


ensureRoot()

class ProtoTester(Topo):
    def __init__(self):
        
        # Initialise topology
        Topo.__init__(self)

        # Add hosts and switches
        sender = self.addHost('sender', ip='10.0.1.1', mac='00:00:00:00:00:01')
        LTE = self.addHost('LTE', ip='0.0.0.0')
        receiver = self.addHost('receiver', ip='10.0.1.2', mac='00:00:00:00:00:02')

        s1 = self.addSwitch('s1')
        s2 = self.addSwitch('s2')

        # Add links
        self.addLink(sender, s1)
        self.addLink(s1, LTE)
        self.addLink(LTE, s2)
        self.addLink(s2, receiver)

def set_all_IP(net, sender, LTE, receiver):
    sender.sendCmd('ifconfig sender-eth0 10.0.1.1 netmask 255.255.255.0')
    sender.waitOutput()
    LTE.sendCmd('ifconfig LTE-eth0 up')
    LTE.waitOutput()
    LTE.sendCmd('ifconfig LTE-eth1 up')
    LTE.waitOutput()
    receiver.sendCmd('ifconfig receiver-eth0 10.0.1.2 netmask 255.255.255.0')
    receiver.waitOutput()

    sender.sendCmd('echo 1 > /proc/sys/net/ipv6/conf/all/disable_ipv6')
    sender.waitOutput()
    LTE.sendCmd('echo 1 > /proc/sys/net/ipv6/conf/all/disable_ipv6')
    LTE.waitOutput()
    receiver.sendCmd('echo 1 > /proc/sys/net/ipv6/conf/all/disable_ipv6')
    receiver.waitOutput()

def display_routes(net, sender, LTE, receiver):
    print 'sender route...'
    sender.sendCmd('route -n')
    print sender.waitOutput()
    print 'LTE route...'
    LTE.sendCmd('route -n')
    print LTE.waitOutput()
    print 'receiver route...'
    receiver.sendCmd('route -n')
    print receiver.waitOutput()

def run_cellsim(LTE):
    LTE.sendCmd('/home/ubuntu/multisend/sender/cellsim-setup.sh LTE-eth0 LTE-eth1')
    LTE.waitOutput()
    print "Running cellsim (this will take a few minutes)..."
    LTE.sendCmd('/home/ubuntu/multisend/sender/cellsim-runner.sh')
    LTE.waitOutput()
    print "done."

def run_datagrump(sender, receiver, maxw):
    print "Running datagrump-receiver...",
    receiver.sendCmd('/home/ubuntu/datagrump/datagrump-receiver 9000 >/tmp/receiver-stdout 2>/tmp/receiver-stderr &')
    receiver.waitOutput()
    print "done."
    print "Running datagrump-sender...",
    sender.sendCmd('/home/ubuntu/datagrump/datagrump-sender 10.0.1.2 9000 debug -maxwindow ' + str(maxw) + ' >/tmp/sender-stdout 2>/tmp/sender-stderr &')
    sender.waitOutput()
    print "done."

def print_welcome_message():
    print "####################################################################"
    print "#                                                                  #"
    print "#               6.829 PS 2 Emulated Network Test                   #"
    print "#                                                                  #"
    print "#          running sender <=> cellsim <=> receiver                 #"
    print "#                                                                  #"
    print "#  Debug output in /tmp/{sender,receiver,cellsim}-{stdout,stderr}  #"
    print "#                                                                  #"
    print "####################################################################"
    print

def run_cellsim_topology(window):
    print_welcome_message()

    os.system( "killall -q controller" )
    os.system( "killall -q cellsim" )
    os.system( "killall -q datagrump-sender" )
    os.system( "killall -q datagrump-receiver" )

    topo = ProtoTester()
    net = Mininet(topo=topo, host=Host, link=Link)
    net.start()

    sender = net.getNodeByName('sender')
    LTE = net.getNodeByName('LTE')
    receiver = net.getNodeByName('receiver')

    set_all_IP(net, sender, LTE, receiver)
    
    #Dump connections
    #dumpNodeConnections(net.hosts)
    #display_routes(net, sender, LTE, receiver)
    print window
    run_datagrump(sender, receiver, window)

    run_cellsim(LTE)

#    CLI(net)

    net.stop()

def upload_data( username ):
    print "Uploading data to server...",
    os.system( 'gzip --stdout /tmp/cellsim-stdout > /tmp/to-upload.gz' )
    reply = requests.post( 'http://6829.keithw.org/cgi-bin/6829/upload-data',
                           files={'contents': (username, open( '/tmp/to-upload.gz',
                                                               'rb' ))} )
    print "done. Got reply:"
    print
    print reply.text

if __name__ == '__main__':
    if len(sys.argv) != 2:
        print "Usage: sudo %s [username]" % sys.argv[ 0 ]
    else:
#experiment with max_window_size
        windows=[10]
        for w in windows:
            run_cellsim_topology(w)
            upload_data( sys.argv[1]+str(w) )

