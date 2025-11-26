#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
MX downloader version 1.4
Copyright (c) 2012-2016 Lucjan Bryndza, Tomasz Sowiński - VERIFONE
"""

import socket, time, datetime
import sys, os, os.path, argparse
#import fcntl
#import termios,
import struct
from functools import partial

class mxdownload:
    def __init__(self, host, port, progress_handler = None):
        self.__host = host
        self.__port = port
        self.__progress_handler = progress_handler
        self.__send_hello()

    def __del(self):
        self.__disconnect()

    def __connect(self):
        self.__sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.__sock.connect((self.__host, self.__port))
        self.__sock.settimeout(5*60)

    def __disconnect(self):
        self.__sock.close()

    def __send_command_no_connect( self, cmdline ):
        b = bytes(cmdline,'ascii')
        b += bytes.fromhex('00')
        self.__sock.sendall( b )
        return self.__sock.recv(1024)

    def __send_hello( self, ver = '1.02.00' ):
        self.__connect()
        self.__sock.sendall( bytes('VER:','ascii') )
        self.__send_command_no_connect( ver )
        self.__disconnect( )

    def __send_command( self, cmdline ):
        self.__connect()
        data = self.__send_command_no_connect( cmdline )
        if data != b'OK\x00':
            raise Exception('Invalid terminal response ' + str(data,'ascii'))
        self.__disconnect( )

    def send_file( self, filename, user="usr1" , mode="644", full_mode = False):
        print('Uploading %s:' % filename)
        with open( filename, "rb") as f:
            NUM_CHUNKS = 100
            f.seek( 0, 2 )
            fsize = f.tell();
            f.seek( 0, 0 )
            chunk_size = fsize // NUM_CHUNKS
            if chunk_size == 0: chunk_size = fsize
            self.__progress_handler(0)
            self.__connect()
            ret = self.__send_command_no_connect("DLD:" +
                                           os.path.basename(filename) +
                                           "\x00" + str(fsize) +
                                           "\x00" + mode +
                                           "\x00" + user +
                                           "\x00users" +
                                           "\x00" + ("F" if full_mode else "P")
                                           )
            if ret != b'OK\x00':
                raise Exception('Invalid terminal response ' + str(ret,'ascii'))
            while f.tell() < fsize:
                buffer = f.read(chunk_size)
                self.__sock.sendall( buffer )
                self.__progress_handler( f.tell() / fsize )
            ret =  self.__sock.recv(1024)
            self.__disconnect()
            print();
            if ret not in (b'DNLD_EXP\x00', b'DNLD_DONE\x00', b'DNLD_OK\x00', b'DNLD_RBOOT\x00'):
                raise Exception('Invalid terminal response ' + str(ret,'ascii'))
            if ret == b'DNLD_RBOOT\x00':
                print('Terminal performed required reboot.')
                sys.exit(0)

    def kill_running_apps( self ):
        print('Killing running apps...')
        self.__send_command('ASP:')

    def set_rtc( self ):
        dt = datetime.datetime.today()
        print('Syncing RTC with host time (%s)...' % dt)
        dt_str = "RTC:" + dt.strftime('%m%d%H%M%Y.%S')
        self.__send_command(dt_str)

    def execute_app( self ):
        print('Executing app...')
        self.__send_command('ART:')

    def reset_terminal( self ):
        print('Resetting terminal...')
        self.__send_command('RST:')

# NOTE - this function is a
def ioctl(fd, op, arg=0, mutable_flag=True):
    if mutable_flag:
        return 0
    else:
        return ""


def get_term_width():
    if sys.platform.startswith('win'):
        try:
            h = windll.kernel32.GetStdHandle(-12)
            csbi = create_string_buffer(22)
            res = windll.kernel32.GetConsoleScreenBufferInfo(h, csbi)
            if res:
                (bufx, bufy, curx, cury, wattr,
                 left, top, right, bottom, maxx, maxy) = struct.unpack("hhhhHhhhhhh", csbi.raw)
                width = right - left + 1
        except:
            width = 80
    else:
        print('Uncomment the  statement in the try below and uncomment the imports of fnctl and termios to get it working on linux')
        return 0
        try:
#            hw = struct.unpack('hh', fcntl.ioctl(1, termios.TIOCGWINSZ, '1234'))
#            hw = struct.unpack('hh', ioctl(1, termios.TIOCGWINSZ, '1234'))
            width = hw[1]
        except:
            try:
                width = os.environ['COLUMNS']
            except:
                width = 80
    return width


def progress_handler(term_width , percent):
    term_width -= 10
    sys.stdout.write('[')
    for p in range(term_width):
        if int(term_width * percent + 0.5) > p: sys.stdout.write('=')
        else: sys.stdout.write(' ')
    sys.stdout.write('] ')
    sys.stdout.write(str(int(percent*100)) + '%\r')
    sys.stdout.flush()


def chmod_str_is_valid(str):
    if len(str) != 3: raise Exception('Invalid chmod length')
    if str[0] < '0' or str[0]>'7': raise Exception('Invalid chmod U permission valid range 0-7')
    if str[1] < '0' or str[1]>'7': raise Exception('Invalid chmod G permission valid range 0-7')
    if str[2] < '0' or str[2]>'7': raise Exception('Invalid chmod O permission valid range 0-7')


def mxdownload_main():
    parser = argparse.ArgumentParser( description = __doc__,
        formatter_class=argparse.RawTextHelpFormatter )
    parser.add_argument('-k','--kill', action='store_true', help='Kill running apps before download')
    parser.add_argument('-e','--execute', action='store_true', help='Execute after download')
    parser.add_argument('-t','--sync-rtc', action='store_true', help='Sync real-time clock with host computer')
    parser.add_argument('-r','--reset', action='store_true', help='Reset terminal')
    parser.add_argument('host', help='Terminal hostname or IP')
    parser.add_argument('file', nargs='*', help='File(s) to upload on terminal')

    filegrp = parser.add_argument_group('upload','Upload file optional attributes')
    filegrp.add_argument('--mode', help='Set file mode(default: %(default)s)', default='644')
    filegrp.add_argument('--user', help='Set file owner (default: %(default)s)',  default='usr1')
    filegrp.add_argument('--full', help='Full download mode', action='store_true')

    args = parser.parse_args()

    chmod_str_is_valid( args.mode )

    files = args.file
    for f in files:
        if not os.path.isfile(f):
            sys.exit('Error: File ' + f + " doesn't exist\r\n")

    dld_port = 5142
    dld = mxdownload( args.host, dld_port, partial(progress_handler, get_term_width()) )

    if args.kill: dld.kill_running_apps()
    if args.sync_rtc: dld.set_rtc()

    for f in files:
        dld.send_file(f, args.user , args.mode, args.full)

    if args.execute: dld.execute_app()
    if args.reset: dld.reset_terminal()

if __name__ == "__main__":
    mxdownload_main()
