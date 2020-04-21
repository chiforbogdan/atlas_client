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

## ATLAS IoT Security Client
ATLAS IoT Security Client is a software module which runs on the network endpoints and empowers the integration with the ATLAS_Gateway software.

----

#### How to build it
Generally, there are two steps are involved:
* Step 1. Install required dependencies by executing the __dependencies.sh__ script from the __scripts__ folder
* Step 2. Execute the __build.sh__ script from __root__ folder

Depending on the platform you are using, minor adjustments might be necessary to be made. See the output messages shown during execution of the above mentioned scripts in case of errors.

----

#### How to use it
```
./atlas_client -h <ATLAS_GATEWAY_HOST> -p <LISTEN_PORT_OF_ATLAS_GATEWAY> -i <LOCAL_IF> -l <LISTEN_PORT_FOR_DATA_PLANE>
```

Arguments:
* __-h__ &nbsp;&nbsp;&nbsp;&nbsp; _Hostname or IP address for the ATLAS_GATEWAY software_
* __-p__ &nbsp;&nbsp;&nbsp;&nbsp; _Port on which the ATLAS_GATEWAY software listen for incoming ATLAS_CLIENT connections_
* __-i__ &nbsp;&nbsp;&nbsp;&nbsp; _Local interface used to communicate with the ATLAS_DATA_PLANE software (an optional software module that one can use to simulate a sensor generating data with defined constraints)_
* __-l__ &nbsp;&nbsp;&nbsp;&nbsp; _Local port for connecting with the ATLAS_DATA_PLANE software module_

Example:
```
./atlas_client -h 165.10.100.13 -p 10111 -i lo -l 10001
```

----

### Supplementary module
Besides the main software module mentioned above, the project will build another supplementary software component, _data_plane_. This tool can be used to simulate an active IoT endpoint instance and test the functionality of the ATLAS_Client module and how it integrates in the ATLAS IoT Security Framework.
It can be used as follows:
````
./data_plane --publish "<sensor feature>:<publish rate in seconds>:<target value>:<deviation>:<qos>:<default | forced packet length>" --subscribe "<sensor feature1>:<sensor feature2>" --hostname protocol://host:port --qos-firewall <qos> --ppm-firewall <ppm> --maxlen-firewall <maxlen> --reputation "<subscribed sensor feature>:<query rate in seconds>:<window size in seconds>:<average | target value>"
````
where:
* --publish "<args>" &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; _args_ define the behaviour of a sensor the tool will simulate. This generated data can be consumed by another instance of _data_plane_ that can then offer feedback, based on which the first endpoint will have its reputation score calculated by the ATLAS_Gateway.
* --subscribe "<args>" &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; _args_ represents one or a series of topics that this instance will subscribe to and consume received data.
* --hostname <arg> &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; _arg_ represents the Mosquitto Broker from the ATLAS_Gateway layer.
* --qos-firewall <arg> &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; _arg_ represents the max QoS value that the firewall will accept for incoming messages.
* --ppm-firewall <arg> &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; _arg_ represents the max Packets-per-minute rate that the firewall will accept for incoming messages.
* --maxlen-firewall <arg>  &nbsp;&nbsp;&nbsp; _arg_ represents the max Length of packets that the firewall will accept for incoming messages.
* --reputation "<args>" &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; _args_ define the behaviour of a sensor when sending feedback for received data for a specific feature.

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
