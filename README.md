Currently written to support a [D1 mini](https://docs.platformio.org/en/latest/boards/espressif8266/d1_mini.html).

## Setup

Before compiling:

* [Create a certificate](https://docs.aws.amazon.com/iot/latest/developerguide/device-certs-create.html) for the IoT device.
* Download the certificate and private key.
* Grab the Amazon Root CA 1 from [here](https://docs.aws.amazon.com/iot/latest/developerguide/managing-device-certs.html#server-authentication).
* Go to IoT Core Settings and copy the "Custom endpoint".
* Run the following:
```shell
echo \#define AWS_IOT_ADDRESS \"<IOT CUSTOM ENDPOINT>\" > config.h
xxd -i <CA FILE> >> config.h
xxd -i <CERT FILE> >> config.h
xxd -i <KEY FILE> >> config.h
```
* Copy config.h to src/