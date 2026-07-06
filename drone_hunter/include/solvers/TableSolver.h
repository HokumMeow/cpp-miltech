#pragma once
#include <string>
#include <vector>
#include "interfaces/IBallisticSolver.h"

// 5 осей — кожна зі своїм набором вузлів (нерівномірний крок)
struct BallisticTable {

    std::vector<float> axisZ0; // висота
    std::vector<float> axisV0; // швидкість
    std::vector<float> axisM;  // маса
    std::vector<float> axisD;  // опір
    std::vector<float> axisL;  // підйомна сила

    // Результат в кожному вузлі сітки
    struct Result {
        float t; // час польоту
        float hDist; // горизонтальна дистанція
    };

    std::vector<Result> data; // |Z0| * |V0| * |M| * |D| * |L|

    // Індекс у плоскому масиві: [iZ0][iV0][iM][iD][iL]
    size_t index(int iz, int iv, int im, int id, int il) const {
        return ((((size_t)iz * axisV0.size() + iv)
                    * axisM.size() + im)
                    * axisD.size() + id)
                    * axisL.size() + il;
    }

    const Result& at(int iz, int iv, int im, int id, int il) const {
        return data[index(iz, iv, im, id, il)];
    }

    bool load(const std::string& path);
    Result lookup(float Z0, float V0, float m, float d, float l) const;
};

class TableSolver : public IBallisticSolver {
public:
    explicit TableSolver(const std::string& tablePath);

    std::optional<Coord> solve(Coord dronePos, Coord targetPos,
                                float speed, float alt,
                                const AmmoParams& ammo) override;
    bool precompute(float speed, float alt, const AmmoParams& ammo) override;
    float getBallisticTime() const override;
    float getHorizDist() const override;

private:
    BallisticTable table_;
    bool loaded_;
    float t_ballist_ = 0.f;
    float h_ballist_ = 0.f;
};
