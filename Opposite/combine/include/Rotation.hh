#ifndef ROTATION_H
#define ROTATION_H

#include <cmath>
#include <vector>

namespace mylib
{
    class Rotation
    {
    public:
        Rotation();
        ~Rotation();

        double deg2rad(double deg);
        double rad2deg(double rad);

        std::vector<double> xyz2r(const std::vector<double> &xyz);
        std::vector<double> VectorGTODinZenith(
            const std::vector<double> &xyz2, const std::vector<double> &xyz1);
        std::vector<double> VectorGTODinZenith1(const std::vector<double> &xyz1, double tt, double pp);
        std::vector<double> VectorZenithinGTOD(
            const std::vector<double> &xyz2, std::vector<double> xyz1);
        std::vector<double> VectorZenithinGTOD1(const std::vector<double> &xyz1, double tt, double pp);

    private:
    };
}

#endif