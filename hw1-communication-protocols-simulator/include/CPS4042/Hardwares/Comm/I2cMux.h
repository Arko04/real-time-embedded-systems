#ifndef I2C_MUX_H
#define I2C_MUX_H

#include <CPS4042/Hardwares/Board.h>
#include <CPS4042/Protocols/Protocol.h>
#include <CPS4042/Units/BaudRate.h>
#include <CPS4042/Wires/Pin.h>
#include <CPS4042/Units/Byte.h>
#include <boost/pfr.hpp>
#include <CPS4042/Utils/ByteStream.h>

namespace Boards
{

    using I2CMuxVoltage = VoltageLevel3_3v;

    template <BaudRate BR, BitRate BTR, typename WorkingVoltageTp>
        requires std::is_base_of_v<AbstractVoltageLevel, WorkingVoltageTp>
    struct I2CMuxGpio
    {
    public:
        Pins::Vdd<WorkingVoltageTp> vdd1{BR, BTR, "I2CMux::vdd1"};
        Pins::Gnd<WorkingVoltageTp> gnd1{BR, BTR, "I2CMux::gnd1"};
        Pins::Vdd<WorkingVoltageTp> vdd2{BR, BTR, "I2CMux::vdd2"};
        Pins::Gnd<WorkingVoltageTp> gnd2{BR, BTR, "I2CMux::gnd2"};
        Pins::Vdd<WorkingVoltageTp> vdd3{BR, BTR, "I2CMux::vdd3"};
        Pins::Gnd<WorkingVoltageTp> gnd3{BR, BTR, "I2CMux::gnd3"};

        Pins::Sda<WorkingVoltageTp> sda1{BR, BTR, "I2CMux::sda1"};
        Pins::Sda<WorkingVoltageTp> sda2{BR, BTR, "I2CMux::sda2"};

        Pins::Scl<WorkingVoltageTp> scl1{BR, BTR, "I2CMux::scl1"};
        Pins::Scl<WorkingVoltageTp> scl2{BR, BTR, "I2CMux::scl2"};

        Pins::Rx<WorkingVoltageTp> rx{BR, BTR, "I2CMux::rx"};
        Pins::Tx<WorkingVoltageTp> tx{BR, BTR, "I2CMux::tx"};
    };

    class I2CMux : public Board<BaudRates::B115200, BitRates::same(BaudRates::B115200),
                                Frequency::F320khz, I2CMuxVoltage, I2CMuxGpio>
    {
    public:
        int m_sensorNum = 3;
        bool sensor_en = false;
        Byte m_data;

        explicit I2CMux() : Parent{"I2CMux::Processor"}
        {

            m_processor->communicationClockChanged.connect(
                [this](Bit edge)
                { m_gpio.scl1.nextEdge(edge); });
            m_processor->communicationClockChanged.connect(
                [this](Bit edge)
                { m_gpio.scl2.nextEdge(edge); });

            m_processor->installProtocol(&i2c0);
            m_processor->installProtocol(&i2c1);
            m_processor->installProtocol(&usart);

            std::cout << "one instance of I2CMux (with dual I2C) created." << std::endl;
        };

        class I2C : public Protocols::AbstractI2C<I2CMux, Gpio>
        {
        public:
            explicit I2C(I2CMux *b, Pins::Sda<I2CMuxVoltage> &sda)
                : Protocols::AbstractI2C<I2CMux, Gpio>{b},
                  m_sda{sda}
            {
            }

            void
            init(Byte address) override
            {
                m_state = State::SendAddress;
                m_sendAddress = address;
                m_currentByte = address;
                m_bitIndex = 0;
            }

            void
            write(Byte address) override
            {
                if (m_state == State::Idle)
                {
                    m_state = State::SendAddress;
                    m_sendAddress = address;
                    m_currentByte = address;
                    m_bitIndex = 0;
                }
            }

            Byte
            read() override
            {
                if (!isDataAvailable())
                    return 0;
                Byte b = m_buffer.front();
                m_buffer.pop();
                return b;
            }

            void
            run(Gpio &gpio) override
            {
                // std::cout << "\033[34m" << (int)m_state << " ";
                switch (m_state)
                {
                case State::Idle:
                    break;

                case State::SendAddress:
                    sendByte(gpio, false);
                    m_state = State::Wait;
                    break;

                case State::Wait:
                    if (m_sda.hasBitToRead() && receiveACK(gpio) == Bit::One)
                        m_state = State::Read0;
                    break;

                case State::Read0:
                    if (m_sda.hasByteToRead())
                    {
                        receiveByte(gpio);
                        m_state = State::Read1;
                        dataStream << m_currentByte;
                        // std::cout << "\033[34mReceived" << (int)m_currentByte << " ";
                    }
                    break;

                case State::Read1:
                    if (m_sda.hasByteToRead())
                    {
                        receiveByte(gpio);
                        m_state = State::ReadCHS;
                        dataStream << m_currentByte;
                        // std::cout << "\033[34mReceived" << (int)m_currentByte << " ";
                    }
                    break;

                case State::ReadCHS:
                    if (m_sda.hasByteToRead())
                    {
                        receiveByte(gpio);
                        m_data = dataStream.take();
                        Byte data0 = getByte<0>(m_data);
                        Byte data1 = getByte<1>(m_data);
                        Byte chksum = data0 > data1 ? data0 - data1 : data1 - data0;
                        if (chksum == m_currentByte)
                        {
                            m_buffer.push(getByte<0>(m_data));
                            m_buffer.push(getByte<1>(m_data));
                        }
                        else
                            std::cerr << "\033[34m" << "Received checksum doesn't match calculated checksum in I2C" << std::endl;
                        m_currentByte = m_sendAddress;
                        m_state = State::Idle;
                    }
                    break;

                default:
                    break;
                }
            }

        private:
            enum class State
            {
                Idle,
                SendAddress,
                Wait,
                Read0,
                Read1,
                ReadCHS,
            } m_state = State::Idle;

            Byte m_sendAddress;
            Byte m_currentByte;
            uint8_t m_bitIndex = 0;
            ByteStream<std::uint16_t> dataStream;
            std::uint16_t m_data;
            Pins::Sda<I2CMuxVoltage> &m_sda;

            void
            sendByte(Gpio &gpio, bool doRelease = false)
            {
                m_sda.write(m_currentByte);
                if (doRelease)
                    m_sda.write(Bit::Z);
            }

            Bit
            receiveACK(Gpio &gpio)
            {
                return m_sda.readBit();
            }

            void
            receiveByte(Gpio &gpio)
            {
                m_currentByte = m_sda.read();
            }
        };
        mutable I2C i2c0{this, m_gpio.sda1};
        mutable I2C i2c1{this, m_gpio.sda2};

        class USART : public Protocols::AbstractUsart<I2CMux, Gpio>
        {
        public:
            explicit USART(I2CMux *b) : Protocols::AbstractUsart<I2CMux, Gpio>{b}
            {
            }

            void
            write(Byte byte) override
            {
                m_data = byte;
                m_state = State::SendStart;
                m_isWritingFinished = false;
            }

            Byte
            read() override
            {
                if (!isDataAvailable())
                    return 0;
                Byte b = m_buffer.front();
                m_buffer.pop();
                return b;
            }

            bool
            isWritingFinished()
            {
                return m_isWritingFinished;
            }

            void
            run(Gpio &gpio) override
            {
                // std::cout << "\033[34m" << (int)m_state << " ";
                switch (m_state)
                {
                case State::Wait:
                    m_isWritingFinished = true;
                    if (gpio.tx.hasBitToRead() && receiveBit(gpio) == Bit::Zero)
                        m_state = State::ReceiveAddress;
                    break;

                case State::ReceiveAddress:
                    if (gpio.tx.hasByteToRead())
                    {
                        data = receiveByte(gpio);
                        m_state = State::ReceiveStop;
                    }
                    break;

                case State::ReceiveStop:
                    if (gpio.tx.hasBitToRead() && receiveBit(gpio) == Bit::One)
                    {
                        m_buffer.push(data);
                        m_state = State::LoadData;
                    }
                    else if (gpio.tx.hasBitToRead())
                    {
                        std::cerr << "\033[34m" << "Didn't receive the Stop Bit in USART" << std::endl;
                        m_state = State::Wait;
                    }
                    break;

                case State::LoadData:
                    break;

                case State::SendStart:
                    if (!gpio.rx.hasBitToWrite())
                    {
                        sendBit(gpio, Bit::Zero);
                        m_state = State::SendData;
                    }
                    break;

                case State::SendData:
                    if (!gpio.rx.hasBitToWrite())
                    {
                        sendByte(gpio, m_data);
                        m_state = State::SendStop;
                    }
                    break;

                case State::SendStop:
                    if (!gpio.rx.hasBitToWrite())
                    {
                        sendBit(gpio, Bit::One);
                        m_state = State::Wait;
                    }
                    break;

                default:
                    break;
                }
            }

        private:
            enum class State
            {
                Wait,
                ReceiveAddress,
                ReceiveStop,
                LoadData,
                SendStart,
                SendData,
                SendStop,
            };

            State m_state = State::Wait;
            Byte m_data;
            Byte data;

            void
            sendBit(Gpio &gpio, Bit b)
            {
                gpio.rx.write(b);
                // std::cout << "\033[34m" << (int)b << (int)m_state << " ";
            }

            void
            sendByte(Gpio &gpio, Byte b)
            {
                gpio.rx.write(b);
                // std::cout << "\033[34m" << (int)b << (int)m_state << " ";
            }

            Bit
            receiveBit(Gpio &gpio)
            {
                Bit b = gpio.tx.readBit();
                // std::cout << "\033[34m" << (int)(b) << (int)m_state << " ";
                return b;
            }

            Byte
            receiveByte(Gpio &gpio)
            {
                Byte b = gpio.tx.read();
                // std::cout << "\033[34m" << (int)(b) << (int)m_state << " ";
                return b;
            }

        } mutable usart{this};

    protected:
        void startModule() override {}
    };
} // namespace Boards

#endif // I2CMUX_H
