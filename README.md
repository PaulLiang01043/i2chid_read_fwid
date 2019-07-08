# i2chid_read_fwid
Elan Touchscreen Firmware ID Tool (I2C-HID Interface)
---
    Get ELAN Touchscreen Firmware ID for Current System.

Compilation
--- 
    make: to build the exectue project "i2chid_read_fwid".
    $ make
   
Run
---
Get Device Information :

    ./i2chid_read_fwid -P {hid_pid} -i
ex:

    ./i2chid_read_fwid -P 2a03 -i

Get Firmware ID :

    ./i2chid_read_fwid -P {hid_pid} -f {fwid_mapping_table_file} -s {system}

ex:

    ./i2chid_read_fwid -P 2a03 -f /tmp/fwid_mapping_table.txt -s chrome

    ./i2chid_read_fwid -P 2a03 -f /tmp/fwid_mapping_table.txt -s windows

Enable Silent Mode :
    ./i2chid_read_fwid -P {hid_pid} -f {fwid_mapping_table_file} -s {system} -q

ex:
    ./i2chid_read_fwid -P 2a03 -f /tmp/fwid_mapping_table.txt -s chrome -q

    ./i2chid_read_fwid -P 2a03 -f /tmp/fwid_mapping_table.txt -s windows -q

