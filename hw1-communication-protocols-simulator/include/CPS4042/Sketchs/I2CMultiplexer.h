#ifndef I2C_MULTIPLEXER_H
#define I2C_MULTIPLEXER_H

#include "CPS4042/Sketchs/AbstractSketch.h"
#include "CPS4042/Hardwares/Comm/I2CMux.h"
#include "CPS4042/Utils/ByteStream.h"
#include <cstdint>
#include <iostream>

class I2CMultiplexer : public AbstractSketch<Boards::I2CMux>
{
public:
    explicit I2CMultiplexer(Boards::I2CMux *node)
        : AbstractSketch<Boards::I2CMux>(node)
    {
    }

protected:
    std::int32_t setup(Boards::I2CMux::Gpio &gpio) override
    {
        std::cout << "I2CMux setup completed." << std::endl;
        delay(1'000);
        return 0;
    }

    std::int32_t loop(Boards::I2CMux::Gpio &gpio) override
    {
        // delay(200);
        switch (m_state)
        {
        case State::R_USART:
            if (node()->usart.isDataAvailable())
            {
                m_channel = node()->usart.read();
                std::cout << "\033[34m" << "Received Channel from Microcontroller: " << (int)m_channel << std::endl;
                if (m_channel == 0)
                    node()->i2c0.init(0x29);
                else if (m_channel == 1)
                    node()->i2c1.init(0x29);
                m_state = State::I2C;
            }
            break;

        case State::I2C:
            if (m_channel == 0 && node()->i2c0.isDataAvailable())
                dataStream << node()->i2c0.read();
            else if (m_channel == 1 && node()->i2c1.isDataAvailable())
                dataStream << node()->i2c1.read();
            if (dataStream.isReady())
            {
                m_state = State::W_USART1;
                m_data = dataStream.take();
                std::cout << "\033[34m" << "Received Data from Sensor " << (int)m_channel << ": " << (int)m_data << std::endl;
            }
            break;

        case State::W_USART1:
            if (node()->usart.isWritingFinished())
            {
                node()->usart.write(getByte<1>(m_data));
                m_state = State::W_USART2;
            }
            break;

        case State::W_USART2:
            if (node()->usart.isWritingFinished())
            {
                node()->usart.write(getByte<0>(m_data));
                m_state = State::R_USART;
            }
            break;

        default:
            break;
        }
        return 0;
    }

private:
    ByteStream<std::uint16_t> dataStream;
    Byte m_channel;
    std::uint16_t m_data;

    enum class State
    {
        R_USART,
        I2C,
        W_USART1,
        W_USART2,
    } m_state = State::R_USART;
};

#endif