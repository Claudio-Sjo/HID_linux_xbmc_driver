Generic HID Remote Driver for XBMC
==================================

This is a generic hid driver for use with XBMC if your computer came with
a hid remote or you bought one. The base code was provided by [coldsource](http://forum.xbmc.org/member.php?u=80895)

After having successfully used this for a while, I started having trouble
when I upgraded my OS to Ubuntu 11.04 and later to Ubuntu 11.10. I debugged
it a bit and provided this slightly hacked version that made it work for me.

This is provided asis and without any guarantee. Please see LICENCE for more
details.


Usage
-----

http://forum.xbmc.org/showthread.php?tid=88560 contains the details...

###### Compile the drivers
$ make

###### Make sure your HID remote is recognized (listed)
$ sudo ./hid_mapper --list-devices

###### Run with learn mode so you can create your map file
find out the manufacturer and product of your device using

$ cat /proc/bus/input/devices

replace 1d57 and ac01 with the values of your device

$ sudo ./hid_mapper --lookup-id --learn --manufacturer 1d57 --product ac01

This will display the key codes for each key-down and key-up event 

Pipe the output into a file to save some typing and push every button on the remote that you want to map
(remember the sequence you pushed the buttons :-) )

$ sudo ./hid_mapper --lookup-id --learn --manufacturer 1d57 --product ac01 > mydevice.map

Now edit mydevice.map, remove the key codes of the key-up events (usually only zeros), remove the blanks
and assign the keys as you like. The result should look like this:

<pre>
010000520000000000:KEY_UP  
010000510000000000:KEY_DOWN  
...
</pre>

Valid key names are defined in /usr/include/linux/input.h

###### Run with the map file to use it
$ sudo ./run_remote

to run this on every system start, put this into /etc/rc.local

###### Blacklisting the device in Xorg
It is necessary to blacklist the device in Xorg so it does not conflict with the mapper. 
Create a new file in /usr/share/X11/xorg.conf.d/. I have named it 50-remote.conf and it contains:

<pre>
# see output of lsusb for the USB id
Section "InputClass"
        Identifier "Remote blacklist"
        MatchUSBID "1d57:ac01"
        Option "Ignore" "on"
EndSection
</pre>

Restart Xorg and you're done.

At this point the remote is not recognized anymore.

###### Extra steps if you need autostart
TODO

