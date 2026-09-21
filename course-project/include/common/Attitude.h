#pragma once

// GY-9250
// Система осей плати по шовкографії: x- вперед, y - ліво, z - вгору
namespace attitude {

double rollDeg(double ay, double az);   // + нахил праворуч
double pitchDeg(double ax, double ay, double az);  // + ніс угору
double headingDeg(double magX, double magY, double offsetDeg = 0.0); // offsetDeg - як плата повернута відносно носа

}  // namespace attitude
