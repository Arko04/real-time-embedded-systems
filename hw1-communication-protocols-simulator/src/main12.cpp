#include <CPS4042/Sketchs/HardDisk.h>
// #include <CPS4042/Sketchs/I2CMultiplexer.h>
#include <CPS4042/Sketchs/Microcontroller.h>
#include <CPS4042/Sketchs/Sensor.h>
#include <CPS4042/Units/Bit.h>
#include <CPS4042/Units/Byte.h>
#include <CPS4042/Wires/Pin.h>
#include <CPS4042/main.h>

std::int32_t
main()
{
    Boards::Esp8266 esp8266;
    Sensors::Vl530x vl530x;
    Sensors::Usb usb;

    auto            linkRed1   = std::make_shared<Link>();
    auto            linkRed2   = std::make_shared<Link>();
    auto            linkBlack1 = std::make_shared<Link>();
    auto            linkBlack2 = std::make_shared<Link>();
    auto            linkGreen  = std::make_shared<Link>();
    auto            linkYellow = std::make_shared<Link>();
    auto            linkBlue   = std::make_shared<Link>();
    auto            linkWhite  = std::make_shared<Link>();

    CPS_SET_OBJECT_NAME(esp8266);
    CPS_SET_OBJECT_NAME(vl530x);
    CPS_SET_OBJECT_NAME(usb);

    CPS_SET_OBJECT_NAME_PTR(linkRed1);
    CPS_SET_OBJECT_NAME_PTR(linkRed2);
    CPS_SET_OBJECT_NAME_PTR(linkBlack1);
    CPS_SET_OBJECT_NAME_PTR(linkBlack2);
    CPS_SET_OBJECT_NAME_PTR(linkGreen);
    CPS_SET_OBJECT_NAME_PTR(linkYellow);
    CPS_SET_OBJECT_NAME_PTR(linkBlue);
    CPS_SET_OBJECT_NAME_PTR(linkWhite);

    esp8266.gpio().vdd1.attachLink(linkRed1);
    esp8266.gpio().vdd2.attachLink(linkRed2);
    esp8266.gpio().gnd1.attachLink(linkBlack1);
    esp8266.gpio().gnd2.attachLink(linkBlack2);
    esp8266.gpio().scl.attachLink(linkGreen);
    esp8266.gpio().sda.attachLink(linkYellow);
    esp8266.gpio().tx.setCanRead(false);
    esp8266.gpio().tx.attachLink(linkBlue);
    esp8266.gpio().rx.attachLink(linkWhite);

    vl530x.gpio().vdd.attachLink(linkRed1);
    vl530x.gpio().gnd.attachLink(linkBlack1);
    vl530x.gpio().scl.attachLink(linkGreen);
    vl530x.gpio().sda.attachLink(linkYellow);

    usb.gpio().vdd.attachLink(linkRed2);
    usb.gpio().gnd.attachLink(linkBlack2);
    usb.gpio().tx.attachLink(linkBlue);
    usb.gpio().rx.attachLink(linkWhite);
    usb.gpio().rx.setCanRead(false);

    MicroController micro(&esp8266);
    Sensor          disSen(&vl530x);
    HardDisk        disHD(&usb);

    micro.start();
    disSen.start();
    disHD.start();

    return Application::exec();
}
