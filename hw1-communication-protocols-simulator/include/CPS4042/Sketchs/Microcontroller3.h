#ifndef MICROCONTROLLER3_H
#define MICROCONTROLLER3_H

#include <CPS4042/Hardwares/Boards/Esp8266.h>
#include <CPS4042/Sketchs/AbstractSketch.h>
#include <CPS4042/Utils/ByteStream.h>
#include <CPS4042/Utils/Wave.h>
#include <bitset>

class MicroController3 : public AbstractSketch<Boards::Esp8266>
{
public:
    explicit MicroController3(Boards::Esp8266* node) :
        AbstractSketch<Boards::Esp8266> {node}
    {}

    std::int32_t
    setup(Boards::Esp8266::Gpio& gpio) override
    {
        std::cout << "esp8266 setup completed." << std::endl;
        delay(1'000);
        node()->usart.write(0);
        return 0;
    }

    std::int32_t
    loop(Boards::Esp8266::Gpio& gpio) override
    {
        // delay(100);
        // I2CMux
        if (!receivedFromSensor && node()->usart.isDataAvailable())
            dataStream << node()->usart.read();
        if (dataStream.isReady()) {
            std::cout << "\033[31m" << "Received Data from I2C Mux: " << (int)dataStream.take() << std::endl;
            receivedFromSensor = true;
        }
        return 0;
    }

private:
    ByteStream<std::uint16_t> dataStream;
    bool receivedFromSensor = false;
    Byte data;
};


#endif    // MICROCONTROLLER3_H
