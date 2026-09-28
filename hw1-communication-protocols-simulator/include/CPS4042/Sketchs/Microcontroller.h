#ifndef MICROCONTROLLER_H
#define MICROCONTROLLER_H

#include <CPS4042/Hardwares/Boards/Esp8266.h>
#include <CPS4042/Sketchs/AbstractSketch.h>
#include <CPS4042/Utils/ByteStream.h>
#include <CPS4042/Utils/Wave.h>
#include <bitset>

class MicroController : public AbstractSketch<Boards::Esp8266>
{
public:
    explicit MicroController(Boards::Esp8266* node) :
        AbstractSketch<Boards::Esp8266> {node}
    {}

    std::int32_t
    setup(Boards::Esp8266::Gpio& gpio) override
    {
        std::cout << "esp8266 setup completed." << std::endl;
        delay(1'000);
        node()->i2c.init(0x29);
        delay(1'000);
        node()->usart.write(37);
        return 0;
    }

    std::int32_t
    loop(Boards::Esp8266::Gpio& gpio) override
    {
        // delay(100);
        // VL530X
        if (!receivedFromSensor && node()->i2c.isDataAvailable())
            dataStream << node()->i2c.read();
        if (dataStream.isReady()) {
            std::cout << "\033[31m" << "Received Data from VL530X: " << (int)dataStream.take() << std::endl;
            receivedFromSensor = true;
        }

        // Usb
        if (node()->usart.isDataAvailable())
            std::cout << "\033[31m" << "Received Data from Usb: " << (int)node()->usart.read() << std::endl;
        return 0;
    }

private:
    ByteStream<std::uint16_t> dataStream;
    bool receivedFromSensor = false;
};


#endif    // MICROCONTROLLER_H
