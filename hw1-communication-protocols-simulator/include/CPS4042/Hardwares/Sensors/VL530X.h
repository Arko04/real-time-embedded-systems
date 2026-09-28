#ifndef VL53_X_H
#define VL53_X_H

#include <CPS4042/Hardwares/Board.h>
#include <CPS4042/Units/BaudRate.h>
#include <CPS4042/Wires/Pin.h>
#include <boost/pfr.hpp>
#include <iostream>
#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_int_distribution.hpp>
#include <CPS4042/Utils/ByteStream.h>
#include <CPS4042/Units/Byte.h>

inline boost::random::mt19937 gen(std::time(nullptr));
inline boost::random::uniform_int_distribution<int> dist(0, 4000);

namespace Sensors
{
    using Vl530xVoltage = VoltageLevel3_3v;

    template <std::uint64_t bar, std::uint64_t btr, typename WorkingVoltageTp>
        requires std::is_base_of_v<AbstractVoltageLevel, WorkingVoltageTp>
    struct Vl530xGpio
    {
    public:
        Pins::Vdd<WorkingVoltageTp> vdd{bar, btr, "Vl530x::vdd"}; // Pin 0
        Pins::Gnd<WorkingVoltageTp> gnd{bar, btr, "Vl530x::gnd"}; // Pin 1
        Pins::Sda<WorkingVoltageTp> sda{bar, btr, "Vl530x::sda"}; // Pin 2
        Pins::Scl<WorkingVoltageTp> scl{bar, btr, "Vl530x::scl"}; // Pin 3
    };

    class Vl530x : public Board<BaudRates::NotSpecified,
                                BitRates::same(BaudRates::NotSpecified),
                                Frequency::Drived, Vl530xVoltage, Vl530xGpio>
    {
    public:
        inline static constexpr Byte address = 0x29;

        explicit Vl530x() : Parent{"Vl530x::processor"}
        {
            m_processor->installProtocol(&i2c);

            std::cout << "one instance of Vl530x" << " created." << std::endl;
        }

        class I2C : public Protocols::AbstractI2C<Vl530x, Gpio>
        {

        public:
            explicit I2C(Vl530x *b) : Protocols::AbstractI2C<Vl530x, Gpio>{b}
            {
            }

            void
            init(Byte address) override
            {
                m_state = State::Idle;
                m_currentByte = 0;
            }

            void
            write(Byte byte) override
            {
            }

            Byte
            read() override
            {
                return 0;
            }

            void
            run(Gpio &gpio) override
            {
                // std::cout << "\033[32m" << (int)m_state << " ";
                switch (m_state)
                {
                case State::Idle:
                    handleStart(gpio);
                    if (reverse(m_currentByte) == address)
                    {
                        m_randomValue = dist(gen);
                        std::cout << "\033[32m" << "Measured Data: " << (int)m_randomValue << std::endl;
                        m_state = State::Ack;
                    }
                    break;

                case State::Ack:
                    if (!gpio.sda.hasBitToWrite())
                    {
                        sendACK(gpio);
                        m_state = State::Write0;
                        m_currentByte = getByte<0>(m_randomValue);
                    }
                    break;

                case State::Write0:
                    if (!gpio.sda.hasBitToWrite())
                    {
                        sendByte(gpio, false);
                        m_state = State::Write1;
                        m_currentByte = getByte<1>(m_randomValue);
                    }
                    break;

                case State::Write1:
                    if (!gpio.sda.hasBitToWrite())
                    {
                        sendByte(gpio, false);
                        Byte greater, smaller;
                        ByteVector<std::uint32_t> data(m_randomValue);
                        greater = data[0] > data[1] ? data[0] : data[1];
                        smaller = data[0] > data[1] ? data[1] : data[0];
                        m_currentByte = greater - smaller;
                        m_state = State::WriteCHS;
                    }
                    break;

                case State::WriteCHS:
                    if (!gpio.sda.hasBitToWrite())
                    {
                        sendByte(gpio, true);
                        m_state = State::Idle;
                        m_currentByte = 0;
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
                Ack,
                Write0,
                Write1,
                WriteCHS,
            } m_state = State::Idle;

            Byte m_currentByte;
            std::uint32_t m_randomValue;

            void
            sendByte(Gpio &gpio, bool doRelease = false)
            {
                gpio.sda.write(m_currentByte);
                // std::cout << "\033[32m" << (int)m_currentByte << (int)m_state << " ";
                if (doRelease)
                    gpio.sda.write(Bit::Z);
            }

            void
            sendACK(Gpio &gpio)
            {
                gpio.sda.write(Bit::One);
                // std::cout << "\033[32m" << 1 << (int)m_state << " ";
            }

            void
            handleStart(Gpio &gpio)
            {
                if (!gpio.sda.hasBitToRead())
                    return;
                Bit b = gpio.sda.readBit();
                // std::cout << "\033[32m" << (int)b << (int)m_state << " ";
                m_currentByte <<= 1;
                if (b == Bit::One)
                    m_currentByte |= 1;
            }


        } mutable i2c{this};

    protected:
        inline void
        startModule() override
        {
            m_gpio.scl.onNextEdge([this](Vl530xVoltage level)
                                  {
            auto bit = Voltage::toBit(level);

            if(bit == Bit::One)    // positive edge
            {
                m_processor->nextCycle(m_gpio);
            } });
        }
    };

} // namespace Sensors

#endif // VL53_X_H
