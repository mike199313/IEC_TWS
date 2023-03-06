#!/usr/bin/env python

import getopt, sys
import string
import time

# Goldentalon SCM EEPROM location
g_eeprom_path = "/sys/bus/i2c/devices/14-0050/eeprom"
g_mac_offset = 0x1000

def usage():
    return str('Usage: ' + sys.argv[0] + ' [--help | -w | -r ]\n'
                                     + '--help    : print help messages. \n'
                                     + '-w        : write bmc mac address to eeprom. \n'
                                     + '-r        : read mac address from eeprom. \n')

def CalChecksum(mac_data, len):
    checksum = 0
    for i in range (0, len, 1):
        checksum = checksum + mac_data[i]

    checksum = (~checksum & 0xff) + 1;
    checksum = (checksum & 0xff);
    print "2's complement checksum is %s" %(hex(checksum))
    return checksum

def readMAC():
    f=open(g_eeprom_path, 'r')
    f.seek(g_mac_offset, 0)
    mac = f.read(6)
    f.close()
    node = mac.encode('hex')
    blocks = [node[x:x+2] for x in xrange(0, len(node), 2)]
    macFormatted = ':'.join(blocks)
    print macFormatted

def checkMAC(s):
    tempchars = "".join(chr(a) for a in range(256))
    delchars = set(tempchars) - set(string.hexdigits)
    mac = s.translate("".join(tempchars),"".join(delchars))
    if len(mac) != 12:
        raise ValueError, ("Ethernet MACs are 12 hex characters, you entered %s",mac)
    return mac.upper()

def writeMAC():
    macaddr = raw_input("Please input MAC Address:")
    macaddr = checkMAC(macaddr)
    mac =[]
    write_mac=[]
    Checksum=0

    for i in range( 0, 12 ):
        if '0'<=macaddr[i] and macaddr[i] <= '9':
            mac.append(int(ord(macaddr[i])-ord('0')))
        elif 'A'<=macaddr[i] and macaddr[i] <= 'F':
            mac.append(int(ord(macaddr[i])-ord('A')+10))

    for i in range(0,12,2):
      #print "%s %s" %(mac[i]<<4,mac[i+1])
        j = (mac[i]<<4 | mac[i+1])
        write_mac.append(j)

    data=bytearray(write_mac)
    Checksum = CalChecksum(data, len(data))
    write_mac.append(Checksum)
    data=bytearray(write_mac)
    print "Write MAC Address (%x:%x:%x:%x:%x:%x) to EEPROM !!" %(write_mac[0],write_mac[1],write_mac[2],write_mac[3],write_mac[4],write_mac[5])
    try:
        f = open(g_eeprom_path, "wb")
        f.seek(g_mac_offset,0)
        f.write(data)
        time.sleep(0.1)
        f.close()
        print "Success"
    except IOError as error:
        print "Write: " + str(error)
        print "Delay 1 sec and write again"
        time.sleep(1)
        f = open(g_eeprom_path, "wb")
        f.seek(g_mac_offset,0)
        f.write(data)
        f.close()
        print "Success"

def main():
    try:
        opts, args=getopt.getopt(sys.argv[1:], "hwr", ["help", "w", "r"])
    except getopt.GetoptError as err:
        print usage()
        sys.exit(2)
    if len(opts) < 1:
        print usage()
        sys.exit(2)

    for opt, arg in opts:
        if opt in ("-h", "--help"):
            print usage()
            sys.exit()
        elif opt == "-w":
            writeMAC()
            sys.exit()
        elif opt == "-r":
            readMAC()
            sys.exit()
        else:
            assert False, "Incorrect parameter"

if __name__ == "__main__":
    main()
