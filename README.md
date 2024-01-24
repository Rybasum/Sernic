Sernic
======

### Windows serial (COM) port to localhost network forwarding with gdb remote protocol filtering.

Sernic pipes binary data between a serial COM port on Windows and one to three TCP/IP channels on the local host. Data received from the COM port is replicated and sent to the connected TCP/IP clients. Data received from any of the TCP/IP clients is sent to the COM port.
```

            /--------> TCP/IP localhost:43210
           /
COM3 <----+----------> TCP/IP localhost:43211
           \
            \--------> TCP/IP localhost:43212
			
```
To start Sernic, open a console in Windows and run the app in it. Sernic can only be started when the serial COM port is available. When the COM device is removed from the system (for example, by removing the serial-USB adapter) the application will exit. It can also be closed by pressing Ctrl-C in the console.

The command line syntax is:

```
Sernic COMxx[:baud_rate] [-c console_port] [-g gdb_port] [-r raw_port]
```

Example command line:
```
Sernic COM3:115200 -c 43210 -g 43211 -r 43212
```
`-c port` is the TCP/IP port number for use with a console  
`-g port` is the TCP/IP port number for connecting gdb (in target remote port mode)  
`-r port` is the TCP/IP port number for connecting a raw terminal  

All three TCP/IP ports are optional. You can specify any port numbers provided that they are available and not in use.

One scenario is debugging Linux kernel in an embedded device from a Windows host with WSL. The kernel is started with kgdb over serial connection (kgdboc) and the device is connected to the PC using a serial-USB cable. Linux sources and gdb are in WSL, and debugging is done with VS Code running on Windows.

```

                         /--------> telnet console in the Terminal panel of VS Code
                        /
 Device ~~~~ COMx <----+----------> gdb in WSL Linux, controlled by VS Code
                        \
                         \--------> (optional) telnet for inspecting raw traffic
						 
```

Sernic can also find its use in many scenarios involving a Windows COM port and WSL, that previously required using the `usbip` or `agent-proxy` utilities. Because `localhost` network is shared between Windows and WSL the TCP/IP clients may run inside WSL and in Windows simultaneously. The clients may be telnet or any other terminal emulator, or any program that processes TCP/IP data streams.

Console channel
---------------

The console TCP/IP channel (port number specified with the `-c` option) includes simple data filtering to remove gdb remote protocol data from the console output. This allows using a single serial connection for Linux kernel debugging and system console (similar to the `agent-proxy` utility). The filter attempts to distinguish gdb data packets from console data and remove them from the stream. Because it only sees the responses from the target it does not follow the protocol and may occasionally let a few gdb bytes through.

Typically an instance of telnet is connected to this port. When the target is running (not stopped by the debugger) the user can see the diagnostic messages from the kernel and use the system console for interacting with the Linux system.

To connect to the console port with telnet:
```
telnet localhost 43210
```
where the port number is the one specified with the `-c` option on the Sernic's command line.

GDB channel
-----------

The GDB channel does not filter the data and just passes all traffic through in binary mode. It is meant for connecting gdb using the command:
```
(gdb) target remote localhost:43211
```
where the port number is the one specified with the `-g`option on the Sernic's command line.

Raw channel
-----------

The raw channel (the port specified with the `-r` option) does not filter the data and just passes all traffic through. Technically, it behaves in the same way as the gdb port. It may be used for connecting another telnet instance for monitoring the raw (unfiltered) data received from the serial port.

Using telnet
------------

Telnet can be started under Windows or under Linux inside WSL (also in the Terminal panel of VS Code). The Linux and Windows versions differ slightly in the capabilities and behaviour.

When telnet connects to a port different than the standard telnet port (24) it does not expect a telnet server at the other side and it does not initially negotiate communication and terminal parameters. It does not transmit any data after establishing a TCP/IP connection.

Depending on the system behind the serial COM port, different telnet settings may be needed for proper handling of special keys and control characters such as Ctrl-C and arrow keys.

For debugging a Linux system it is more convenient to use the Linux version of telnet and set it in "single character" mode, CR as end of line and with local echo disabled. The remote Linux system (and also u-boot) echo most characters sent and they properly handle Enter, Ctrl-C, Backspace and arrow keys.

To set telnet in single character mode, start a session and type the Ctrl-] escape sequence to show a telnet prompt. Then type:
```
mode character
```
Using telnet in VS Code
-----------------------

While Sernic is running, a telnet session can be opened within the Terminal window in VS Code, by issuing the same command as from a command console:
```
telnet localhost 43210
```
It is advisable to set telnet in character mode. Also, to allow passing of most special keys (like Fx function keys, tabs and arrows) to telnet instead of the VS Code application, set the "Send Keybindings To Shell" option in the Termianl > Integrated options of VS Code.

