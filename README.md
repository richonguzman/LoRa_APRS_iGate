### This Fork of richonguzman/LoRa_APRS_iGate/ is based on V2.3 of his firmware.

# You will see a more bi-directional view on the iGate-functionality, including:

* all position pakets that come form IS are gated to the LoRa interface (not only objects). This leads to a much better useability for stations that have their tracker connected to a device with map(s) and is therefore ideal for blackout, offgrid and offline (internet) use and for users in the field.



* dynamic filtering: in short: if you have a (e.g.) 150km-range covered by your iGate, then there could be a lot of "useless" trafic if no client is around. with dynamic filtering there will be a 15km range around each lora-station applied to the IS-filter (see [Serverside Filtering](https://www.aprs-is.net/javAPRSFilter.aspx)). this is done without reconnecting the IS-server.
  

# coming next

* Websocket packet-display/logging of the iGate
* more useability of the time-offset --> timezones-implementation
* region-based selection of APRS-IS servers to prevent useless traffic via internet


