# rig-pty-semi-automatic

**OVERVIEW:** 

<img
src="pics/architecture.jpg"
border="1"
raw=true
/>


*** 
**TRAFFIC LIGHT STATES OF THE DISPLAY:**

<img
src="pics/traffic_light_states.jpg"
border="1"
raw=true
/>

***
**NEXTION TOUCH DISPLAY NOTES:**

CONFIGURING THE NEXTION LIBRARY:  
Make sure you edit the NexConfig.h file on the library folder to set the
correct serial port for the display.
By default it's set to Serial1, which most arduino boards don't have.
Change "#define nexSerial Serial1" to "#define nexSerial Serial"
if you are using arduino uno, nano, etc.
***
NEXTION SWITCH STATES LIST:  
Every nextion switch button needs a switchstate variable (bool)  
• to update the screen after a page change (all buttons)  
• to control switchtoggle (dualstate buttons)  
• to prevent screen flickering because of unnecessary permanent update (momentary buttons)
***
VARIOUS COMMANDS:  

CLICK A BUTTON:

    Serial2.print("click bt1,1");  
    send_to_nextion();
A switch (Dual State Button)will be toggled with this command
A button (normal momentary button) will be set permanently pressed)

    Serial2.print("click b3,0");
    send_to_nextion();

Releases a push button again.
Has no effect on a dual state Button

HIDE AN OBJECT:  
    
    Serial2.print("vis t0,0");
    send_to_nextion();

***
