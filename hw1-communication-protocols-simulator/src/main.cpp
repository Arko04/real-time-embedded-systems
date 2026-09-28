#include <CPS4042/Sketchs/HardDisk.h>
#include <CPS4042/Sketchs/I2CMultiplexer.h>
#include <CPS4042/Sketchs/Microcontroller.h>
#include <CPS4042/Sketchs/Microcontroller3.h>
#include <CPS4042/Sketchs/Sensor.h>
#include <CPS4042/Units/Bit.h>
#include <CPS4042/Units/Byte.h>
#include <CPS4042/Wires/Pin.h>
#include <CPS4042/main.h>

std::int32_t
main()
{
    Boards::Esp8266 esp8266;
    Boards::I2CMux i2cmux;
    Sensors::Vl530x vl530x1;
    Sensors::Vl530x vl530x2;

    auto linkRed1 = std::make_shared<Link>();
    auto linkRed2 = std::make_shared<Link>();
    auto linkRed3 = std::make_shared<Link>();
    auto linkBlack1 = std::make_shared<Link>();
    auto linkBlack2 = std::make_shared<Link>();
    auto linkBlack3 = std::make_shared<Link>();
    auto linkGreen1 = std::make_shared<Link>();
    auto linkGreen2 = std::make_shared<Link>();
    auto linkYellow1 = std::make_shared<Link>();
    auto linkYellow2 = std::make_shared<Link>();
    auto linkBlue = std::make_shared<Link>();
    auto linkWhite = std::make_shared<Link>();

    CPS_SET_OBJECT_NAME(esp8266);
    CPS_SET_OBJECT_NAME(i2cmux);
    CPS_SET_OBJECT_NAME(vl530x1);
    CPS_SET_OBJECT_NAME(vl530x2);

    CPS_SET_OBJECT_NAME_PTR(linkRed1);
    CPS_SET_OBJECT_NAME_PTR(linkRed2);
    CPS_SET_OBJECT_NAME_PTR(linkRed3);
    CPS_SET_OBJECT_NAME_PTR(linkBlack1);
    CPS_SET_OBJECT_NAME_PTR(linkBlack2);
    CPS_SET_OBJECT_NAME_PTR(linkBlack3);
    CPS_SET_OBJECT_NAME_PTR(linkGreen1);
    CPS_SET_OBJECT_NAME_PTR(linkGreen2);
    CPS_SET_OBJECT_NAME_PTR(linkYellow1);
    CPS_SET_OBJECT_NAME_PTR(linkYellow2);
    CPS_SET_OBJECT_NAME_PTR(linkBlue);
    CPS_SET_OBJECT_NAME_PTR(linkWhite);

    esp8266.gpio().vdd1.attachLink(linkRed1);
    esp8266.gpio().gnd1.attachLink(linkBlack1);
    esp8266.gpio().tx.setCanRead(false);
    esp8266.gpio().tx.attachLink(linkBlue);
    esp8266.gpio().rx.attachLink(linkWhite);

    i2cmux.gpio().vdd1.attachLink(linkRed1);
    i2cmux.gpio().gnd1.attachLink(linkBlack1);
    i2cmux.gpio().vdd2.attachLink(linkRed2);
    i2cmux.gpio().gnd2.attachLink(linkBlack2);
    i2cmux.gpio().vdd3.attachLink(linkRed3);
    i2cmux.gpio().gnd3.attachLink(linkBlack3);
    i2cmux.gpio().tx.attachLink(linkBlue);
    i2cmux.gpio().rx.attachLink(linkWhite);
    i2cmux.gpio().rx.setCanRead(false);
    i2cmux.gpio().scl1.attachLink(linkGreen1);
    i2cmux.gpio().scl2.attachLink(linkGreen2);
    i2cmux.gpio().sda1.attachLink(linkYellow1);
    i2cmux.gpio().sda2.attachLink(linkYellow2);

    vl530x1.gpio().vdd.attachLink(linkRed2);
    vl530x1.gpio().gnd.attachLink(linkBlack2);
    vl530x1.gpio().scl.attachLink(linkGreen1);
    vl530x1.gpio().sda.attachLink(linkYellow1);

    vl530x2.gpio().vdd.attachLink(linkRed3);
    vl530x2.gpio().gnd.attachLink(linkBlack3);
    vl530x2.gpio().scl.attachLink(linkGreen2);
    vl530x2.gpio().sda.attachLink(linkYellow2);

    MicroController3 micro(&esp8266);
    I2CMultiplexer mux(&i2cmux);
    Sensor disSen1(&vl530x1);
    Sensor disSen2(&vl530x2);

    micro.start();
    mux.start();
    disSen1.start();
    disSen2.start();

    return Application::exec();
}
