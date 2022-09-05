#pragma once
#include <cmath>


static constexpr double pi = 3.1415926535897932384626433832795;
inline constexpr double D2R = pi / 180.0;
inline constexpr double R2D = 180.0 / pi;

inline constexpr double rad(double deg){
    return deg * D2R;
}
inline constexpr double deg(double rad){
    return rad * R2D;
}

inline constexpr auto mod(double val, double m){
    int r = val / m;
    return val - r*m;
}

inline constexpr auto modpos(double val, double m){
    auto res = mod(val, m);
    if(res < 0) res += m;
    return res;
}
inline constexpr auto modmid(double val, double m){
    auto res = modpos(val, m);
    if(res > m/2) res -= m;
    return res;
}

inline constexpr double getAzimuthDelta(double target, double min, double len){
    target           = modpos(target, 360);
    min              = modpos(min, 360);
    double delta_min = modpos(target - min, 360);
    if(delta_min * len < 0)
        delta_min -= 360;

    if((len < 0 && delta_min >= len) || (len > 0 && delta_min <= len))
        return delta_min;

    double mid = modpos(min + len * 0.5, 360);
    double delta_mid = modmid(target - mid, 360);
    if(delta_mid * len > 0)
        return len;

    return 0;
}

struct SunPosition{
    double altitude;
    double azimuth;
};

static_assert(sizeof(double) == 8);
static_assert(sizeof(long long) == 8);

SunPosition getSunPosition(long long timestamp, double longitude_west, double latitude_north){


    static constexpr double e = rad(23.4393);
    static const double se = std::sin(e);
    static const double ce = std::cos(e);

    static constexpr double JD_0 = 2440587.5;
    static constexpr double JD_2000 = 2451545;
    static constexpr double s_per_d = 86400;
    double JD = (timestamp / s_per_d) + JD_0 - JD_2000;

    static constexpr double M0_deg = 357.5291 - 360;
    static constexpr double M1_deg = 0.98560028;
    double M_deg = M0_deg + M1_deg * JD;
    double M = rad(M_deg);

    static constexpr double T0_deg = 280.1470;
    static constexpr double T1_deg = 360.9856235;
    double T_deg = T0_deg + T1_deg * JD - longitude_west;
    double T = rad(T_deg);


    static constexpr double C1 = 1.9148;
    static constexpr double C2 = 0.0200;
    static constexpr double C3 = 0.0003;
    static constexpr double EL = rad(102.9373);
    double L = pi + EL + M + rad(
        C1 * std::sin(M) +
        C2 * std::sin(M*2) +
        C3 * std::sin(M*3)
    );

    double sL = std::sin(L);
    double cL = std::cos(L);
    double d = std::asin(sL*se);
    double Lat = rad(latitude_north);
    double H = T-std::atan2(sL*ce, cL);
    double sH = std::sin(H), cH = std::cos(H);
    double sLat = std::sin(Lat), cLat = std::cos(Lat);

    SunPosition res;
    res.azimuth  = deg(std::atan2(sH, cH*sLat-std::tan(d)*cLat));
    res.altitude = deg(std::asin(sLat*std::sin(d) + cLat*std::cos(d)*cH ));


    return res;
}