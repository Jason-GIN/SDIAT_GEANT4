#include "Rotation.hh"

namespace mylib
{

    Rotation::Rotation()
    {
    }

    Rotation::~Rotation()
    {
    }

    double Rotation::deg2rad(double deg)
    {
        double rad;
        rad = deg / 180.0 * M_PI;
        return rad;
    };

    double Rotation::rad2deg(double rad)
    {
        double deg;
        deg = rad / M_PI * 180.0;
        return deg;
    };

    std::vector<double> Rotation::xyz2r(const std::vector<double> &xyz)
    {
        std::vector<double> rtp(3);
        rtp[0] = std::sqrt(xyz[0] * xyz[0] + xyz[1] * xyz[1] + xyz[2] * xyz[2]);
        rtp[1] = std::acos(xyz[2] / rtp[0]);
        rtp[2] = std::atan2(xyz[1], xyz[0]);

        return rtp;
    };

    std::vector<double> Rotation::VectorGTODinZenith(
        const std::vector<double> &xyz2, const std::vector<double> &xyz1)
    {
        std::vector<double> xyzn(3), rtp(3);
        double t, p;
        rtp = xyz2r(xyz1);
        t = rtp[1];
        p = rtp[2];
        xyzn[0] = std::cos(p) * std::cos(t) * xyz2[0] 
                + std::sin(p) * std::cos(t) * xyz2[1] 
                - std::sin(t) * xyz2[2];
        xyzn[1] = -std::sin(p) * xyz2[0] + std::cos(p) * xyz2[1];
        xyzn[2] = std::cos(p) * std::sin(t) * xyz2[0] 
                + std::sin(p) * std::sin(t) * xyz2[1] 
                + std::cos(t) * xyz2[2];

        return xyzn;
    }

    std::vector<double> Rotation::VectorGTODinZenith1(
        const std::vector<double> &xyz1, double t, double p)
    {
        std::vector<double> xyzn(3);

        xyzn[0] = std::cos(p) * std::cos(t) * xyz1[0] 
                + std::sin(p) * std::cos(t) * xyz1[1] 
                - std::sin(t) * xyz1[2];
        xyzn[1] = -std::sin(p) * xyz1[0] + std::cos(p) * xyz1[1];
        xyzn[2] = std::cos(p) * std::sin(t) * xyz1[0] 
                + std::sin(p) * std::sin(t) * xyz1[1] 
                + std::cos(t) * xyz1[2];

        return xyzn;
    }

    std::vector<double> Rotation::VectorZenithinGTOD(
        const std::vector<double> &xyz2, std::vector<double> xyz1)
    {
        std::vector<double> xyzn(3), rtp(3);
        double t, p;
        rtp = xyz2r(xyz1);
        t = rtp[1];
        p = rtp[2];
        xyzn[0] = std::cos(p) * std::cos(t) * xyz2[0] 
                - std::sin(p) * xyz2[1] 
                + std::cos(p) * std::sin(t) * xyz2[2];
        xyzn[1] = std::sin(p) * std::cos(t) * xyz2[0] 
                + std::cos(p) * xyz2[1] 
                + std::sin(p) * std::sin(t) * xyz2[2];
        xyzn[2] = -std::sin(t) * xyz2[0] + std::cos(t) * xyz2[2];

        return xyzn;
    }

    std::vector<double> Rotation::VectorZenithinGTOD1(
        const std::vector<double> &xyz1, double t, double p)
    {
        std::vector<double> xyzn(3);

        xyzn[0] = std::cos(p) * std::cos(t) * xyz1[0] 
                - std::sin(p) * xyz1[1] 
                + std::cos(p) * std::sin(t) * xyz1[2];
        xyzn[1] = std::sin(p) * std::cos(t) * xyz1[0] 
                + std::cos(p) * xyz1[1] 
                + std::sin(p) * std::sin(t) * xyz1[2];
        xyzn[2] = -std::sin(t) * xyz1[0] + std::cos(t) * xyz1[2];

        return xyzn;
    }
}