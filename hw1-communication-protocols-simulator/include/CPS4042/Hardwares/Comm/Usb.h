#ifndef USB_H
#define USB_H

#include <CPS4042/Hardwares/Board.h>
#include <CPS4042/Protocols/Protocol.h>
#include <CPS4042/Units/BaudRate.h>
#include <CPS4042/Wires/Pin.h>
#include <CPS4042/Units/Byte.h>
#include <boost/pfr.hpp>
#include <CPS4042/Utils/ByteStream.h>
#include <CPS4042/Units/Byte.h>

namespace Sensors
{
    using UsbVoltage = VoltageLevel3_3v;

    template <std::uint64_t bar, std::uint64_t btr, typename WorkingVoltageTp>
        requires std::is_base_of_v<AbstractVoltageLevel, WorkingVoltageTp>
    struct UsbGpio
    {
    public:
        Pins::Vdd<WorkingVoltageTp> vdd{bar, btr, "Usb::vdd"}; // Pin 0
        Pins::Gnd<WorkingVoltageTp> gnd{bar, btr, "Usb::gnd"}; // Pin 1
        Pins::Tx<WorkingVoltageTp> tx{bar, btr, "Usb::tx"}; // Pin 2
        Pins::Rx<WorkingVoltageTp> rx{bar, btr, "Usb::rx"}; // Pin 3
    };

    class Usb : public Board<BaudRates::NotSpecified,
                             BitRates::same(BaudRates::NotSpecified),
                             Frequency::F320khz, UsbVoltage, UsbGpio>
    {
    public:
        explicit Usb() : Parent{"Usb::Processor"}
        {
            m_processor->installProtocol(&usart);

            std::cout << "one instance of Usb" << " created." << std::endl;
        };

        void
        fillStorage(std::unordered_map<Byte, Byte> storage)
        {
            m_storage = storage;
        }

        class USART : public Protocols::AbstractUsart<Usb, Gpio>
        {
        public:
            explicit USART(Usb *b) : Protocols::AbstractUsart<Usb, Gpio>{b}
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

            void
            run(Gpio &gpio) override
            {
                // std::cout << "\033[32m" << (int)m_state << " ";
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
        inline void
        startModule() override
        {
        }
        std::unordered_map<Byte, Byte> m_storage;
    };
} // namespace Boards

#endif // USB_H
