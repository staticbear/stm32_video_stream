Simple video stream project based on STM32f407 and Omni Vision sensor OV9655. <br>
<br>
Description:<br>
The stm32 contains the UDP server and after power up it waits clients connection.<br>
From the client side there is PC application used for connection to the UDP server and display video stream.<br>
Also PC client side application provide controls for server. (stream settings, server settings, ov9655 settings)<br>
<br>
After power up the stm32 initialize the UDP server and wait connect from the client.<br>
When a client connects to the UDP server and send command to start stream, the stm32 starts getting frames from the ov9655,<br> 
convert them into UDP frames  and transmit it to the client.<br>
<br>
Features:<br>
- Tiny size of the stm32 "bare metal" software, so no need license for Keil IDE<br>
- No proprietary libraries used in stm32 part of the project<br>
- For getting list of the available udp servers and changing server ip address used broadcast udp packets,<br> 
  so it can be done from any ethernet subnets<br> 
- Client application builded with Qt4<br> 
<br>
Project structure:<br>
- stm32_eth_stream<br>
- control_lib<br>
- control_gui<br>
<br>
Stm32 part compiled in Keil IDE 5.24.1<br>
for building client side software use:<br>
qmake<br>
make<br>


