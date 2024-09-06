initial date: Mar 10, 2022.  

# Brief of ORISOL Bootloader Driver:
1. communication with RTC by USB.  
2. bootloader driver support partial ONS USB protocol and full BL USB protocol.  
3. USB support hot plug and auto-connect with speicific RTC device. (ONS_RTC / BL_RTC)  

# Quick Start
## Execute Bootloader Driver program
   1. default  
   `$ ./output/debug/bl_driver`  

   2. command line interface  
   `$ ./output/debug/bl_driver -cli`  

   3. help in cli mode  
   `>> help`  

   4. show usb device list from system  
   `>> usb list`  

   5. try blcmd command, communicate with BL_RTC  
   `>> blcmd nop`  

   6. try ons command, communicate with ONS_RTC  
   `>> onscmd ver`  

  🈲 in ONS_RTC mode, cannot send BL_RTC protocol, result RTC in stuck, and hardware reset only.  