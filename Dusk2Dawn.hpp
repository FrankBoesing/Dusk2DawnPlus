/*  Dusk2Dawn.hpp
 *  Get time of sunrise and sunset.
 *  Created by DM Kishi <dm.kishi@gmail.com> on 2017-02-01.
 *  Updated to modern C++ for microcontrollers by Frank B.on 2026-09-20
 *  <https://github.com/FrankBoesing/Dusk2Dawn++>
 */

#ifndef DUSK2DAWN_HPP
#define DUSK2DAWN_HPP

#include <cmath>
#include <cstdint>

class Dusk2Dawn {
  public:
    constexpr Dusk2Dawn(float latitude, float longitude, float timezone) noexcept
        : _latitude(latitude), _longitude(longitude), _timezone(timezone) {}

    int sunrise(int y, int m, int d, bool isDST = false) const;
    int sunset(int y, int m, int d, bool isDST = false) const;
    static bool min2str(char (&str)[6], int minutes) noexcept;

  private:
    float _latitude;
    float _longitude;
    float _timezone;

    static constexpr float PI_VAL = 3.14159265358979323846f;

    static constexpr float degToRad(float deg) noexcept {
        return deg * (PI_VAL / 180.0f);
    }

    static constexpr float radToDeg(float rad) noexcept {
        return rad * (180.0f / PI_VAL);
    }

    static constexpr float fractionOfCentury(float jd) noexcept {
        return (jd - 2451545.0f) / 36525.0f;
    }

    /* Convert Gregorian calendar date to Julian Day. */
    static constexpr float jDay(int year, int month, int day) noexcept {
        if (month <= 2) {
            year -= 1;
            month += 12;
        }
        const int A = year / 100;
        const int B = 2 - A + (A / 4);
        return static_cast<float>(
            static_cast<long>(365.25f * (year + 4716)) +
            static_cast<long>(30.6001f * (month + 1)) +
            day + B - 1524.5f
        );
    }

    /* Zero-pad a component of time, e.g. 1 -> "01", 24 -> "24". */
    static constexpr bool zeroPadTime(char* str, std::uint8_t timeComponent) noexcept {
        if (timeComponent >= 100) {
            return false;
        }
        str[0] = static_cast<char>((timeComponent / 10) + '0');
        str[1] = static_cast<char>((timeComponent % 10) + '0');
        return true;
    }

    static constexpr float geomMeanLongSun(float t) noexcept {
        float L0 = std::fmod(280.46646f + t * (36000.76983f + t * 0.0003032f), 360.0f);
        if (L0 < 0.0f) {
            L0 += 360.0f;
        }
        return L0;
    }

    static constexpr float geomMeanAnomalySun(float t) noexcept {
        return 357.52911f + t * (35999.05029f - 0.0001537f * t);
    }

    static constexpr float eccentricityEarthOrbit(float t) noexcept {
        return 0.016708634f - t * (0.000042037f + 0.0000001267f * t);
    }

    int sunriseSet(bool isRise, int y, int m, int d, bool isDST) const;
    float sunriseSetUTC(bool isRise, float jday, float latitude, float longitude) const;
    float equationOfTime(float t) const;
    float meanObliquityOfEcliptic(float t) const;
    float sunDeclination(float t) const;
    float sunApparentLong(float t) const;
    float sunTrueLong(float t) const;
    float sunEqOfCenter(float t) const;
    float hourAngleSunrise(float lat, float solarDec) const;
    float obliquityCorrection(float t) const;
};

#endif // DUSK2DAWN_HPP
