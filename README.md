# FunkeeB - Corne V4 ProMicro Edition

ZMK config and relevant files for my custom built Corne v4 Promicro Edition by klouderone.

This board is based on Corne v4 but uses ProMicro controller (nice!nano v2 clone) for more connection stability and ease of buiding.
Compatible with both Choc and MX swithes!
It features 3x6 column staggered keys and 3 key thumb cluster. 
Compatible with ZMK Studio. 


#### Components

-   `MCU`: nice!nano v2 _compatible board_ (wireless)
-   `PCB`: Corne v4 ProMicro Edition [cornev4 promicro](https://github.com/klouderone/cornev4promicroedition) designs
-   `Case`: Custom Stainless steel 316l case for a Choc version and Nylon PA12S-HP case for the MX version. I highly recommend using JLC3DP for printing those cases!
-   `Batteries`: 110mah Lipo
-   `Sockets`: Hotswap sockets
-   `Diodes`: 1N4148W SMD Diode SOD-123
-   `Switches`: MX or Choc v1 / v2

#### Software

Flash MCU with a software generated in the actions section of Github. Later you can use ZMK Studio. 

Each workflow run produces these hardware-specific firmware files:

- `funkeeb-corne-left-niceview-bongo`: left/central half with nice!view and Bongo Cat
- `funkeeb-corne-right-niceview-status`: right half with nice!view status screen
- `funkeeb-corne-right-tps43-touchpad`: right half with the TPS43/IQS5xx touchpad

Flash the left file together with exactly one of the two right-half variants.

The left nice!view firmware also includes the optional
[`zmk-widget-bridge`](https://github.com/finestedm/zmk-widget-bridge). Without
the Linux companion it behaves exactly like the regular Bongo Cat screen.
While the companion heartbeat is active, it rotates through Bongo Cat,
weather, and the next Google Calendar events.

#### Keymap:



## Images

|          Corne v4 MX          |            Photos             |
| :---------------------------: | :---------------------------: |
| ![Photo 1](assets/PXL_20260611_052722855.PORTRAIT.jpg) | ![Photo 2](assets/PXL_20260611_172306024.MP.jpg) |
| ![Photo 3](assets/PXL_20260611_171750737.MP.jpg) | ![Photo 4](assets/PXL_20260611_172642045.MP.jpg) |
