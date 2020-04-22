[![License](https://img.shields.io/badge/license-GPL%20v3.0%20or%20later-brightgreen.svg)](https://github.com/chiforbogdan/atlas_client/blob/master/LICENSE)

# ATLAS IoT Security Platform - general information
ATLAS consists in a 3-tier IoT security platform which offers the following modules:
* A lightweight software client which runs on the IoT device ([ATLAS_Client])
* A gateway software which runs on the network edge and manages all the clients from the network ([ATLAS_Gateway])
* A cloud platform which allows managing the gateways and the clients ([ATLAS_Cloud])

ATLAS provides security management for a fleet of IoT devices and enables a reputation based Sensing-as-a-service platform. It also offers the capability to inspect the IoT device telemetry values and supports the CoAP lightweight protocol for the communication between the IoT device and the gateway.
On the IoT data plane layer, ATLAS provides an API which can be integrated with a user application and offers the following capabilities:
* Install a firewall rule on the gateway side
* Send packet statistics to the gateway and cloud
* Get the device with the most trusted reputation within a category and provide reputation feedback

## ATLAS Client
ATLAS Client is a software module which runs on the IoT devices and empowers the integration with the ATLAS Gateway software. This component consists in a lightweight agent which monitors various system parameters and which exposes an API to a custom application (data plane). ATLAS Client API has the following functions:
* Allows a custom application to install an MQTT application layer firewall rule on the gateway side. This firewall rule shapes the IoT device ingress traffic.
* Transmits network statistics to ATLAS Gateway component.
* Allows obtaining the device with the highest reputation within a sensor category (enables a Sensing-as-a-service architecture).
* Allows delivering reputation feedback for an IoT device which publishes data within a sensor category.

ATLAS Gateway installs *push alerts (update data periodically to the gateway)* and *threshold alerts (scan the system frequently and update data to the gateway only when a threshold is passed)* to ATLAS Client side in order to monitor the IoT device state in *real-time*.
ATLAS Client communicates with ATLAS Gateway using CoAPs protocol (secured using DTLS-PSK) and accepts asynchronous CoAPs transported commands from ATLAS Gateway.

----

#### How to build it
Generally, there are two steps are involved:
* Step 1. Install required dependencies by executing the __dependencies.sh__ script from the __scripts__ folder
* Step 2. Execute the __build.sh__ script from __root__ folder

Depending on the platform you are using, minor adjustments might be necessary to be made. See the output messages shown during execution of the above mentioned scripts in case of errors.

----

#### How to use it
```
./atlas_client -h <ATLAS_GATEWAY_HOST> -p <TLAS_GATEWAY_PORT> -i <LOCAL_INTERFACE> -l <LOCAL_LISTEN_PORT>
```

Arguments:
* __-h__ &nbsp;&nbsp;&nbsp;&nbsp; _Hostname or IP address for ATLAS Gateway software_
* __-p__ &nbsp;&nbsp;&nbsp;&nbsp; _Port on which the ATLAS Gateway software listens for incoming ATLAS Client connections_
* __-i__ &nbsp;&nbsp;&nbsp;&nbsp; _Local interface used to receive asynchronous commands from ATLAS Gateway (ATLAS Client acts as CoAP server)_
* __-l__ &nbsp;&nbsp;&nbsp;&nbsp; _Local port for receiving asynchronous commands from ATLAS Gateway (ATLAS Client acts as CoAP server)_

Example:
```
./atlas_client -h 192.168.0.1 -p 10100 -i eth0 -l 10001
```

----

### Supplementary module
Besides the main software module mentioned above, the project will build another supplementary software component, _data_plane_. This tool can be used to simulate an active IoT end-point instance and test the functionality of the ATLAS Client module and how it integrates in the ATLAS IoT Security Framework. The _data_plane_ application is an example and can be used as model to build a custom IoT application which uses the ATLAS Client API.
It can be used as follows:
````
./data_plane --publish "<sensor feature>:<publish rate in seconds>:<target value>:<deviation>:<qos>:<default | forced packet length>" --subscribe "<sensor feature1>:<sensor feature2>" --hostname protocol://host:port --qos-firewall <qos> --ppm-firewall <ppm> --maxlen-firewall <maxlen> --reputation "<subscribed sensor feature>:<query rate in seconds>:<window size in seconds>:<average | target value>"
````
where:
* --publish <args> &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; _args_ define the behaviour of a sensor publishing simulated data. This generated data can be consumed by another instance of _data_plane_ that can then offer feedback, based on which the first endpoint will have its reputation score computed by ATLAS Gateway. The *sensor feature* parameter indicates the published data sensor feature type (e.g. temperature). The *publish rate in seconds* parameter indicates how often the device will generate data. The *target value* parameter indicates the base numerical value for the published data and the *deviation* parameter indicates the deviation from this base value (e.g. if target value is 10 and deviation is 1, then the published value will be in the interval [9, 11]). The *qos* parameter indicates the MQTT QoS value used when publishing the data (accepted values are 0,1 or 2). The *default | forced packet length* parameter indicates the packet length: if *default* is specified the packet will contain the normal simlated numerical value, otherwise if a *forced packet length* (numerical value) is specified, then the instance will publish a packet of the indicated length with a pre-defined pattern. Several publish data arguments can be specified (separated by comma).
* --subscribe <args> &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; _args_ represents one or more *sensor feature* topics that this instance will subscribe to and consume the received data. This parameter accepts several values separated by comma.
* --hostname <arg> &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; _arg_ represents the MQTT broker (data plane) from the ATLAS Gateway. The format is the following: _tcp://hostname:port_.
* --qos-firewall <arg> &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; _arg_ represents the maximum QoS value of the incoming MQTT messages. This parameter generates a firewall rule which is installed on ATLAS Gateway. Possible values are 0,1 or 2.
* --ppm-firewall <arg> &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; _arg_ represents the maximum Packets-per-minute MQTT packet rate for the incoming MQTT messages. This parameter generates a firewall rule which is installed on ATLAS Gateway. 
* --maxlen-firewall <arg>  &nbsp;&nbsp;&nbsp; _arg_ represents the maximum packet length for the incoming messagess. This parameter generates a firewall rule which is installed on ATLAS Gateway.
* --reputation <args> &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; _args_ define the behaviour of a sensor when consuming data with the highest reputation and when delivering reputation feedback (this parameter is optional). The *subscribed sensor feature* parameter indicates the *sensor feature* for which this instance wants to receive the most trusted data (e.g. if this parameter is set to *temperature*, the instance obtains from ATLAS Gateway the device identity which delivers the most trusted *temperature* value). The *query rate in seconds* indicates the time rate at which the instance will obtain the most trusted device within a sensor category. The *window size in seconds* parameter indicates the time period during which the instance will accept data from other devices: the instance will query the most trusted device, then it will broadcast the system and obtain the same data type from several devices. At the end of this window, the instance will consume data only from the most trusted device, but it will deliver feedback for all the publisher devices in order to allow them to increase their reputation. The *average | target value* parameter indicates the method to compute the reputation feedback: *average* will compute the average value during the *window size* and the feedback score will measure the *distance* between the average value and the published device value, *target value* indicates a numerical value for which the reputation score will measure the *distance* between this target value and the published device value.

Example of usage:
````
./data_plane --publish "air_pressure:10:30:2:1:default" --subscribe "temperature" --hostname tcp://127.0.0.1:18830 --qos-firewall 2 --ppm-firewall 1000 --maxlen-firewall 1000 --reputation "temperature:60:20:30"
````

----

#### Authors
ATLAS Client was developed by:
* Bogdan-Cosmin Chifor
* Stefan-Ciprian Arseni
* Ioana Cismas
* Mihai Coca
* Mirabela Medvei

ATLAS project is sponsored by [UEFISCDI].

----

#### License
GNU General Public License v3.0 or later.

See LICENSE file to read the full text.

[ATLAS_Client]: https://github.com/chiforbogdan/atlas_client
[ATLAS_Gateway]: https://github.com/chiforbogdan/atlas_gateway
[ATLAS_Cloud]: https://github.com/chiforbogdan/atlas_cloud
[UEFISCDI]: https://uefiscdi.gov.ro/
