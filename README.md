[![License](https://img.shields.io/badge/license-GPL%20v3.0%20or%20later-brightgreen.svg)](https://github.com/chiforbogdan/atlas_client/blob/master/LICENSE)

## ATLAS IoT Security Client
ATLAS IoT Security Client is a software module which runs on the network endpoints and empowers the integration with the [ATLAS IoT Security Gateway][Atlas_gateway] software.

----

#### How to build it
Generally, there are two steps are involved:
* Step 1. Install required dependencies by executing the [dependencies.sh][dep_script] script from the [scripts][script_dir] folder
* Step 2. Execute the [build.sh][build_script] script from [root][root_dir] folder

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

#### Authors
ATLAS_CLIENT Security Component was developed by:
* Bogdan-Cosmin Chifor
* Stefan-Ciprian Arseni
* Ioana Cismas
* Mihai Coca
* Mirabela Medvei

The ATLAS project is sponsored by [UEFISCDI].

----

#### License
GNU General Public License v3.0 or later.

See [LICENSE] file to read the full text.

[Atlas_gateway]: https://github.com/chiforbogdan/atlas_gateway
[UEFISCDI]: https://uefiscdi.gov.ro/
[LICENSE]: https://github.com/chiforbogdan/atlas_client/blob/master/LICENSE
[dep_script]: https://github.com/chiforbogdan/atlas_client/tree/scripts/scripts/dependencies.sh
[script_dir]: https://github.com/chiforbogdan/atlas_client/tree/scripts/scripts/
[build_script]: https://github.com/chiforbogdan/atlas_client/blob/scripts/build.sh
[root_dir]: https://github.com/chiforbogdan/atlas_client/tree/scripts/
