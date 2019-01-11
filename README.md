# SolarChargerSZBK07ModuleV2
Hardware and software for an MPPT solar charger. It uses an SZBK07 DC converter module, a MOSFET diode. The main software is in the form of an MPPT library header file.

There are 2 versions of the schematic. The original design was suitable for a 60-cell solar panel with an MPP voltage of 30 volts.
The additional version is suitable for a 36-cell solar panel with an MPP voltage of 18 volts. 
The difference is in the design of the MOSFET diode, which in the original design uses an N-channel MOSFET and relies on there being
a significant voltage difference between the panel voltage and the battery voltage. 
The second design uses a P-channel MOSFET and derives its bias voltage from ground, and so does not rely on panel voltage.

