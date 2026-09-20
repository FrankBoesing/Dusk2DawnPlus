/*  Dusk2Dawn.cpp
 *  Get time of sunrise and sunset.
 *  Created by DM Kishi <dm.kishi@gmail.com> on 2017-02-01.
 *  Updated to modern C++ for microcontrollers by Frank B.on 2026-09-20
 *  <https://github.com/FrankBoesing/Dusk2Dawn++>
 */

#include "Dusk2Dawn.hpp"

/******************************************************************************/
/*                                   PUBLIC                                   */
/******************************************************************************/

int Dusk2Dawn::sunrise(int y, int m, int d, bool isDST) const {
    return sunriseSet(true, y, m, d, isDST);
}

int Dusk2Dawn::sunset(int y, int m, int d, bool isDST) const {
    return sunriseSet(false, y, m, d, isDST);
}

/* Convert minutes elapsed since midnight, the figure returned by the public
 * methods sunrise() and sunset(), to a 24-hour clock format, e.g. "23:00".
 */
bool Dusk2Dawn::min2str(char (&str)[6], int minutes) noexcept {
    if (str == nullptr) {
        return false;
    }

    auto writeError = [](char* s) { s[0] = 'E'; s[1] = 'R'; s[2] = 'R'; s[3] = 'O'; s[4] = 'R'; s[5] = '\0'; };

    // 1. Ungültige Minuten abfangen
    if (minutes < 0 || minutes >= 1440) {
        writeError(str);
        return false;
    }

    const auto byteHour   = static_cast<std::uint8_t>(minutes / 60);
    const auto byteMinute = static_cast<std::uint8_t>(minutes % 60);

    char strHour[2]{};
    char strMinute[2]{};

    // 2. Fehler bei der Zeitformatierung abfangen
    if (!zeroPadTime(strHour, byteHour) || !zeroPadTime(strMinute, byteMinute)) {
        writeError(str);
        return false;
    }

    // 3. Erfolgsfall schreiben
    str[0] = strHour[0];
    str[1] = strHour[1];
    str[2] = ':';
    str[3] = strMinute[0];
    str[4] = strMinute[1];
    str[5] = '\0';

    return true;
}

/******************************************************************************/
/*                                  PRIVATE                                   */
/******************************************************************************/

int Dusk2Dawn::sunriseSet(bool isRise, int y, int m, int d, bool isDST) const {
    const float jday = jDay(y, m, d);
    const float timeUTC = sunriseSetUTC(isRise, jday, _latitude, _longitude);

    // Advance the calculated time by a fraction of itself.
    const float newJday = jday + timeUTC / (60.0f * 24.0f);
    const float newTimeUTC = sunriseSetUTC(isRise, newJday, _latitude, _longitude);

    int timeLocal;
    if (!std::isnan(newTimeUTC)) {
        timeLocal = static_cast<int>(std::round(newTimeUTC + (_timezone * 60.0f)));
        timeLocal += isDST ? 60 : 0;
    } else {
        // There is no sunrise or sunset, e.g. it's in the (ant)arctic.
        timeLocal = -1;
    }

    return timeLocal;
}

float Dusk2Dawn::sunriseSetUTC(bool isRise, float jday, float latitude, float longitude) const {
    const float t = fractionOfCentury(jday);
    const float eqTime = equationOfTime(t);
    const float solarDec = sunDeclination(t);
    float hourAngle = hourAngleSunrise(latitude, solarDec);

    hourAngle = isRise ? hourAngle : -hourAngle;
    const float delta = longitude + radToDeg(hourAngle);
    const float timeUTC = 720.0f - (4.0f * delta) - eqTime; // in minutes
    return timeUTC;
}

/* ---------------------------- EQUATION OF TIME ---------------------------- */
float Dusk2Dawn::equationOfTime(float t) const {
    const float epsilon = obliquityCorrection(t);
    const float l0 = geomMeanLongSun(t);
    const float e = eccentricityEarthOrbit(t);
    const float m = geomMeanAnomalySun(t);

    float y = std::tan(degToRad(epsilon) / 2.0f);
    y *= y;

    float sin2l0, cos2l0;
    // Berechnet sin und cos in einem Rutsch
    #if defined(__GNUC__) || defined(__clang__)
        ::sincosf(2.0f * degToRad(l0), &sin2l0, &cos2l0);
    #else
        const float rad2l0 = 2.0f * degToRad(l0);
        sin2l0 = std::sin(rad2l0);
        cos2l0 = std::cos(rad2l0);
    #endif

    const float sinm   = std::sin(degToRad(m));
    const float sin4l0 = 2.0f * sin2l0 * cos2l0;
    const float sin2m  = std::sin(2.0f * degToRad(m));

    const float Etime = y * sin2l0
                  - 2.0f * e * sinm
                  + 4.0f * e * y * sinm * cos2l0
                  - 0.5f * y * y * sin4l0
                  - 1.25f * e * e * sin2m;

    return radToDeg(Etime) * 4.0f; // in minutes of time
}

/* ------------------------ OBLIQUITY OF ECLIPTIC --------------------------- */
float Dusk2Dawn::meanObliquityOfEcliptic(float t) const {
    const float seconds = 21.448f - t * (46.8150f + t * (0.00059f - t * 0.001813f));
    const float e0 = 23.0f + (26.0f + (seconds / 60.0f)) / 60.0f;
    return e0; // in degrees
}

/* --------------------------- SOLAR DECLINATION ---------------------------- */
float Dusk2Dawn::sunDeclination(float t) const {
    const float e = obliquityCorrection(t);
    const float lambda = sunApparentLong(t);

    const float sint = std::sin(degToRad(e)) * std::sin(degToRad(lambda));
    const float theta = radToDeg(std::asin(sint));
    return theta; // in degrees
}

float Dusk2Dawn::sunApparentLong(float t) const {
    const float o = sunTrueLong(t);
    const float omega = 125.04f - 1934.136f * t;
    const float lambda = o - 0.00569f - 0.00478f * std::sin(degToRad(omega));
    return lambda; // in degrees
}

float Dusk2Dawn::sunTrueLong(float t) const {
    const float l0 = geomMeanLongSun(t);
    const float c = sunEqOfCenter(t);
    const float O = l0 + c;
    return O; // in degrees
}

float Dusk2Dawn::sunEqOfCenter(float t) const {
    const float m = geomMeanAnomalySun(t);
    const float mrad = degToRad(m);
    const float sinm = std::sin(mrad);
    const float sin2m = std::sin(mrad * 2.0f);
    const float sin3m = std::sin(mrad * 3.0f);
    const float C = sinm * (1.914602f - t * (0.004817f + 0.000014f * t)) + sin2m * (0.019993f - 0.000101f * t) + sin3m * 0.000289f;
    return C; // in degrees
}

/* ------------------------------- HOUR ANGLE ------------------------------- */
float Dusk2Dawn::hourAngleSunrise(float lat, float solarDec) const {
    // cos(90.833°) als vorkompilierte constexpr-Konstante
    static constexpr float COS_ZENITH = -0.01454389765f;

    const float latRad = degToRad(lat);
    const float sdRad  = degToRad(solarDec);

    const float sinLat = std::sin(latRad);
    const float cosLat = std::cos(latRad);
    const float sinSd  = std::sin(sdRad);
    const float cosSd  = std::cos(sdRad);

    const float denom = cosLat * cosSd;
    if (denom == 0.0f) return 0.0f;

    const float HAarg = (COS_ZENITH - (sinLat * sinSd)) / denom;

    // Absicherung gegen NaN bei Polartag/Polarnacht
    if (HAarg >= 1.0f)  return 0.0f;      // Sonne geht nicht auf
    if (HAarg <= -1.0f) return PI_VAL;    // Sonne geht nicht unter

    return std::acos(HAarg);
}

/* ---------------------------- SHARED FUNCTIONS ---------------------------- */
float Dusk2Dawn::obliquityCorrection(float t) const {
    const float e0 = meanObliquityOfEcliptic(t);
    const float omega = 125.04f - 1934.136f * t;
    const float e = e0 + 0.00256f * std::cos(degToRad(omega));
    return e; // in degrees
}
