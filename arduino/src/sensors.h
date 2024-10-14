#pragma once

#include <libmodule.h>
#include <RtcDS1302.h>
#include <time.h>

struct HD44780Bidirectional : public libmodule::utility::Bidirectional<uint8_t> {
    static constexpr auto N = 4;
    uint8_t get() const override;
    void set(uint8_t const state) override;
    HD44780Bidirectional(uint8_t const pins[N]);
private:
    uint8_t const pm_pins[N];
};

template <typename T>
struct CycleSensor : public libmodule::utility::Input<T> {
    //Will return the value of the sensor for this cycle
    T get() const override { return cycle_value; }
    //Will read the sensor value from hardware and store it for the cycle
    void cycle_read() { cycle_value = get_sensor_value(); }
protected:
    //Should return the value of the sensor as found in hardware
    virtual T get_sensor_value() = 0;
    T cycle_value;
};

using Sensor_t = CycleSensor<float>;
using DigiSensor_t = CycleSensor<bool>;

struct DFDpad {
    enum Button : uint8_t {
        None, Left, Right, Up, Down, Centre
    };
    struct AnalogToDpadButton : public CycleSensor<Button> {
        int pin;
    private:
        Button get_sensor_value() override;
    } atod;
    struct DirectionOut : public libmodule::utility::Input<bool> {
        bool get() const override;
        DirectionOut(AnalogToDpadButton &button, Button const dir);
    private:
        AnalogToDpadButton &button;
        Button dir;
    } left{atod, Left}, right{atod, Right},
      up{atod, Up},     down{atod, Down},
      centre{atod, Centre};
};

namespace rtc {
    extern RtcDS1302<ThreeWire> *ds1302;
    template<typename wire_t>
    void setup_rtc(RtcDS1302<wire_t> &Rtc);
    void update_from_rtc();
    void write_time(time_t const time);
}

template <typename wire_t>
void rtc::setup_rtc(RtcDS1302<wire_t> &Rtc)
{
    ds1302 = &Rtc;
    // From https://github.com/Makuna/Rtc/blob/master/examples/DS1302_Simple/DS1302_Simple.ino

    Serial.print("compiled: ");
    Serial.print(__DATE__);
    Serial.println(__TIME__);

    Rtc.Begin();

    RtcDateTime compiled = RtcDateTime(__DATE__, __TIME__);
    Serial.println();

    if (!Rtc.IsDateTimeValid()) 
    {
        // Common Causes:
        //    1) first time you ran and the device wasn't running yet
        //    2) the battery on the device is low or even missing

        Serial.println("RTC lost confidence in the DateTime!");
        Rtc.SetDateTime(compiled);
    }

    if (Rtc.GetIsWriteProtected())
    {
        Serial.println("RTC was write protected, enabling writing now");
        Rtc.SetIsWriteProtected(false);
    }

    if (!Rtc.GetIsRunning())
    {
        Serial.println("RTC was not actively running, starting now");
        Rtc.SetIsRunning(true);
    }

    RtcDateTime now = Rtc.GetDateTime();
    if (now < compiled) 
    {
        Serial.println("RTC is older than compile time!  (Updating DateTime)");
        Rtc.SetDateTime(compiled);
    }
    else if (now > compiled) 
    {
        Serial.println("RTC is newer than compile time. (this is expected)");
    }
    else if (now == compiled) 
    {
        Serial.println("RTC is the same as compile time! (not expected but all is fine)");
    }

    Rtc.SetDateTime(compiled);
}