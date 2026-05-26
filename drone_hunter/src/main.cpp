#include <iostream>
#include <fstream>
#include <cstring>
#include <cmath>
#include "json.hpp"

using namespace std;
using json = nlohmann::json;

#define ENABLE_LOG 1
#define ENABLE_DEBUG 0

#if ENABLE_LOG
#define LOG(msg) cout << "[LOG] " << msg << endl
#else
#define LOG(msg)
#endif

#if ENABLE_DEBUG
#define DEBUG(msg) cout << "[DEBUG] " << msg << endl
#else
#define DEBUG(msg)
#endif

const float g = 9.81f;
const int MAX_STEPS = 10000;
const float PI = 3.14159265f;

enum DroneState
{
    STOPPED,
    ACCELERATING,
    DECELERATING,
    TURNING,
    MOVING
};

struct Coord
{
    float x = 0.f;
    float y = 0.f;
    float z = 0.f;

    // Додавання координат
	Coord operator+(const Coord& other) const {
    	Coord result;
        result.x = x + other.x;
        result.y = y + other.y;
        return result;
	}
 
	// Віднімання координат
	Coord operator-(const Coord& other) const {
    	Coord result;
        result.x = x - other.x;
        result.y = y - other.y;
        return result;
	}
 
	// Множення на скаляр
	Coord operator*(float s) const {
    	Coord result;
        result.x = x * s;
        result.y = y * s;
        return result;
	}

    // Ділення на скаляр
    Coord operator/(float s) const {
        Coord result;
        result.x = x / s;
        result.y = y / s;
        return result;
    }

    // порівняння координат
    bool operator==(const Coord& other) const {
        return x == other.x && y == other.y;
    }
    
};

struct AmmoParams
{
    char name[32];
    float mass;
    float drag;
    float lift;
};

struct DroneConfig
{
    Coord startPos;      // початкова позиція (x, y)
    float altitude;      // висота
    float initialDir;    // початковий напрямок (рад)
    float attackSpeed;   // швидкість атаки (м/с)
    float accelPath;     // шлях розгону (м)
    char ammoName[32];   // обрані боєприпаси
    float arrayTimeStep; // крок часу масиву цілей
    float simTimeStep;   // крок симуляції
    float hitRadius;     // радіус влучення
    float angularSpeed;  // кутова швидкість (рад/с)
    float turnThreshold; // поріг повороту (рад)
};

struct SimStep
{
    Coord pos;             // позиція дрона
    float direction;       // напрямок (рад)
    int state;             // стан автомата (0-4)
    int targetIdx;         // індекс поточної цілі
    Coord dropPoint;       // точка скиду (куди летить дрон)
    Coord aimPoint;        // куди впаде бомба (якщо скинути зараз)
    Coord predictedTarget; // прогнозована позиція цілі
};

int saveResultFile(SimStep* simStep,int N)
{

    json out;
    out["totalSteps"] = N;
    out["steps"] = json::array();
    for (int i = 0; i < N; i++) {
        json step;
        step["position"]        = {{"x", simStep[i].pos.x}, {"y", simStep[i].pos.y}};
        step["direction"]       = simStep[i].direction;
        step["state"]           = simStep[i].state;
        step["targetIndex"]     = simStep[i].targetIdx;
        step["dropPoint"]       = {{"x", simStep[i].dropPoint.x},
                                {"y", simStep[i].dropPoint.y}};
        step["aimPoint"]        = {{"x", simStep[i].aimPoint.x},
                                {"y", simStep[i].aimPoint.y}};
        step["predictedTarget"] = {{"x", simStep[i].predictedTarget.x},
                                {"y", simStep[i].predictedTarget.y}};
        out["steps"].push_back(step);
    }
    ofstream fout("simulation.json");
    fout << out.dump(2);
    fout.close();

    return 0;
}

float calc_t(const AmmoParams& a, float attackSpeed, float zd)
{
    float d = a.drag;
    float m = a.mass;
    float l = a.lift;
    float a_val = d * g * m - 2.f * d * d * l * attackSpeed;
    float b = -3.f * g * m * m + 3.f * d * l * m * attackSpeed;
    float c = 6.f * m * m * zd;
    float p = (-(b * b)) / (3.f * a_val * a_val);
    float q = 2.f * b * b * b / (27.f * a_val * a_val * a_val) + c / a_val;
    double acosArg = 3.f * q / (2.f * p) * sqrtf(-3.f / p);
    if (acosArg < -1.0 || acosArg > 1.0)
    {
        cout << "phi out of range" << endl;
        return -1;
    }
    float phi = acosf(acosArg);
    float t = 2.f * sqrtf(-p / 3.f) * cosf((phi + 4.0 * PI) / 3.f) - b / (3.0 * a_val);
    if (t <= 0)
    {
        return -1;
    }
    return t;
}

float calc_h(const AmmoParams& a, float attackSpeed, float t)
{
    float t2 = t * t, t3 = t2 * t, t4 = t3 * t, t5 = t4 * t;
    float d2 = a.drag * a.drag, d3 = d2 * a.drag, d4 = d3 * a.drag;
    float l2 = a.lift * a.lift, l3 = l2 * a.lift, l4 = l3 * a.lift;
    float m2 = a.mass * a.mass, m3 = m2 * a.mass, m4 = m3 * a.mass;
    float result = attackSpeed * t - t2 * a.drag * attackSpeed 
    / (2.f * a.mass) + t3 * (6.f * a.drag * g * a.lift * a.mass - 6.f * d2 * (l2 - 1.f) * attackSpeed) 
    / (36.f * m2) + t4 * (-6.f * d2 * g * a.lift * (1.f + l2 + l4) * a.mass + 3.f * d3 * l2 * (1.f + l2) * attackSpeed + 6.f * d2 * a.drag * l4 * (1.f + l2) * attackSpeed) 
    / (36.f * (1.f + l2) * (1.f + l2) * m3) + t5 * (3.f * d3 * g * l3 * a.mass - 3.f * d4 * l2 * (1.f + l2) * attackSpeed) 
    / (36.f * (1.f + l2) * m4);
    return result;
}

Coord normalize(Coord c) {
    return c / hypot(c.x, c.y);
}

float length(Coord delta)
{
    return sqrtf((delta.x) * (delta.x) + (delta.y) * (delta.y));
}

void interpolate(float t, float arrayTimeStep, int targetIndex, int timeSteps , Coord** targets, Coord &output)
{
    int idx = (int)floorf(t / arrayTimeStep) % timeSteps;
    int next = (idx + 1) % timeSteps;
    float frac = (t - idx * arrayTimeStep) / arrayTimeStep;
    output.x = targets[targetIndex][idx].x + (targets[targetIndex][next].x - targets[targetIndex][idx].x) * frac;
    output.y = targets[targetIndex][idx].y + (targets[targetIndex][next].y - targets[targetIndex][idx].y) * frac;
}


int main()
{
    int step = 0;
    DroneState droneState = STOPPED;
    float speed = 0.f;
    float currentTime = 0.f;
    float angleDiff = 0.f;

    ////////////////////////////////////////////////////////////////////////
    // Читання JSON config
    ifstream fin("config.json");
    json j;
    fin >> j;

    DroneConfig config;
    config.startPos.x = j["drone"]["position"]["x"];
    config.startPos.y = j["drone"]["position"]["y"];
    config.altitude = j["drone"]["altitude"];
    config.initialDir = j["drone"]["initialDirection"];
    config.attackSpeed = j["drone"]["attackSpeed"];
    config.accelPath = j["drone"]["accelerationPath"];
    config.angularSpeed = j["drone"]["angularSpeed"];
    config.turnThreshold = j["drone"]["turnThreshold"];
    config.arrayTimeStep = j["targetArrayTimeStep"];
    config.simTimeStep   = j["simulation"]["timeStep"];
    config.hitRadius     = j["simulation"]["hitRadius"];

    string ammoStr = j["ammo"].get<string>();
    strncpy(config.ammoName, ammoStr.c_str(), 31);

    LOG("Config loaded: x=" << config.startPos.x);
    LOG("Config loaded: y=" << config.startPos.y);
    LOG("Config loaded: speed=" << config.attackSpeed);
    LOG("Config loaded: altitude=" << config.altitude);
    LOG("Config loaded: initial direction=" << config.initialDir);
    LOG("Config loaded: attack speed=" << config.attackSpeed);
    LOG("Config loaded: turn threshold=" << config.turnThreshold);
    LOG("Config loaded: array time step=" << config.arrayTimeStep);
    LOG("Config loaded: simulation time step=" << config.simTimeStep);
    LOG("Config loaded: hit radius=" << config.hitRadius);

    ////////////////////////////////////////////////////////////////////////
    // Читання JSON ammo
    ifstream f_a("ammo.json");
    if (!f_a.is_open())
    {
        cout << "Error opening ammo file" << endl;
        return 1;
    }

    json j_a;
    f_a >> j_a;
    int ammoCount = j_a.size();
    int selectedAmmo = -1;
    AmmoParams* ammo = new AmmoParams[ammoCount];
    for (int i = 0; i < ammoCount; i++) {
        strncpy(ammo[i].name, j_a[i]["name"].get<string>().c_str(), 31);
        ammo[i].mass = j_a[i]["mass"];
        ammo[i].drag = j_a[i]["drag"];
        ammo[i].lift = j_a[i]["lift"];
        if (strcmp(ammo[i].name, config.ammoName) == 0)
        {
            selectedAmmo = i;
        }
    }
    if (selectedAmmo == -1)
    {
        cout << "Unknown ammo!" << endl;
        delete[] ammo;
        return 1;
    }

    LOG("Config loaded: ammo=" << config.ammoName);

    ////////////////////////////////////////////////////////////////////////
    // Умовно постійні значення t_ballist hDist
    float t_ballist = calc_t(ammo[selectedAmmo], config.attackSpeed, config.altitude);
    if (t_ballist == -1.f)
    {
        cout << "Error calculating ballistic time!" << endl;
        delete[] ammo;
        return 1;
    }
    float hDist = calc_h(ammo[selectedAmmo], config.attackSpeed, t_ballist);
    if (hDist <= 0.f)
    {
        cout << "Error calculating ballistic height!" << endl;
        delete[] ammo;
        return 1;
    }
    DEBUG("  t_ballist=" << t_ballist << " hDist=" << hDist);

    ////////////////////////////////////////////////////////////////////////
    // Читання JSON targets
    ifstream ft("targets.json");
    json jt; ft >> jt;
    int targetsQty = jt["targetCount"];
    int timeSteps = jt["timeSteps"];
    if (targetsQty <= 0) {
        cout << "0 targets!" << endl;
        delete[] ammo;
        ammo = nullptr;
        return 1;
    }
    Coord** targets = new Coord*[targetsQty];
    for (int i = 0; i < targetsQty; i++) {
        targets[i] = new Coord[timeSteps];
        for (int j = 0; j < timeSteps; j++) {
            targets[i][j].x = jt["targets"][i]["positions"][j]["x"];
            targets[i][j].y = jt["targets"][i]["positions"][j]["y"];
        }
    }

    LOG("Config loaded: targetsQty=" << targetsQty);
    LOG("Config loaded: timeSteps=" << timeSteps);

    SimStep* simStep = new SimStep[MAX_STEPS];
    Coord dronePos = config.startPos;
    Coord bestPred;

    float accel = config.attackSpeed * config.attackSpeed / (2.f * config.accelPath);
    float currentDir = config.initialDir;
    int prevBestTarget = -1;

    while (step < MAX_STEPS)
    {

        int bestTarget = -1;
        float minTime = 1e9f;
        float D = 0.f;
        
        Coord targInterp;
        Coord predicted;

        for (int i = 0; i < targetsQty; i++)
        {
            interpolate(currentTime, config.arrayTimeStep, i, timeSteps, targets, targInterp);

            Coord delta = targInterp - dronePos;

            D = length(delta);
            float totalTime = (D - hDist) / config.attackSpeed + t_ballist;

            // виявилось щщо треба декілька разів інтерполювати
            // емперично підібрано що достатньо 3 рази
            for (int k = 0; k < 3; k++)
            {
                interpolate(currentTime + totalTime, config.arrayTimeStep, i, timeSteps, targets, predicted);
                delta = predicted - dronePos;
                D = length(delta);
                totalTime = (D - hDist) / config.attackSpeed + t_ballist;
            }            

            float timeToStop = 0.f;
            if (i != prevBestTarget)
            {
                switch (droneState)
                {
                case STOPPED:
                    timeToStop = 0.f;
                    break;
                case ACCELERATING:
                    timeToStop = speed / accel;
                    break;
                case DECELERATING:
                    timeToStop = speed / accel;
                    break;
                case MOVING:
                    timeToStop = config.attackSpeed / accel;
                    break;
                case TURNING:
                    timeToStop = fabsf(angleDiff) / config.angularSpeed;
                    break;
                }
            }

            if (totalTime + timeToStop < minTime)
            {
                minTime = totalTime + timeToStop;
                bestTarget = i;
                bestPred = predicted;
            }
        }

        DEBUG("  target=" << bestTarget << " state=" << droneState);

        Coord delta = bestPred - dronePos;
        Coord firePoint = bestPred - normalize(delta) * hDist;

        float angleToTarget = atan2f(firePoint.y - dronePos.y, firePoint.x - dronePos.x);

        angleDiff = angleToTarget - currentDir;

        while (angleDiff > PI)
            angleDiff -= 2 * PI;
        while (angleDiff < -PI)
            angleDiff += 2 * PI;

        if (fabsf(angleDiff) > config.turnThreshold)
        {
            switch (droneState)
            {
            case STOPPED:
                droneState = TURNING;
                break;
            case MOVING:
                droneState = DECELERATING;
                break;
            }
        }

        switch (droneState)
        {
        case STOPPED:
            if (fabsf(angleDiff) < config.turnThreshold)
            {
                droneState = ACCELERATING;
                angleDiff = 0.f;
            }
            else
            {
                droneState = TURNING;
            }
            break;
        case ACCELERATING:
            speed += accel * config.simTimeStep;
            if (speed >= config.attackSpeed)
            {
                speed = config.attackSpeed;
                droneState = MOVING;
            }
            dronePos.x += speed * cosf(currentDir) * config.simTimeStep;
            dronePos.y += speed * sinf(currentDir) * config.simTimeStep;
            break;

        case DECELERATING:
            speed -= accel * config.simTimeStep;
            if (speed <= 0.f)
            {
                speed = 0.f;
                droneState = TURNING;
            }
            dronePos.x += speed * cosf(currentDir) * config.simTimeStep;
            dronePos.y += speed * sinf(currentDir) * config.simTimeStep;
            break;
        case MOVING:
            if (fabsf(angleDiff) > config.turnThreshold)
            {
                droneState = DECELERATING;
            }
            else
            {
                if (fabsf(angleDiff) < config.turnThreshold)
                {
                    currentDir = angleToTarget;
                }
                dronePos.x += speed * cosf(currentDir) * config.simTimeStep;
                dronePos.y += speed * sinf(currentDir) * config.simTimeStep;
            }
            break;
        case TURNING:
        {
            float turnStep = config.angularSpeed * config.simTimeStep;
            if (fabsf(angleDiff) <= turnStep)
            {
                currentDir = angleToTarget;
                droneState = ACCELERATING;
                angleDiff = 0.f;
            }
            else
            {
                currentDir += (angleDiff > 0 ? 1.f : -1.f) * turnStep;
            }
            break;
        }
        }

        DEBUG("Step " << step << " pos=(" << dronePos.x << "," << dronePos.y << ")");

        Coord dir = { cos(currentDir), sin(currentDir) };
                
        simStep[step].pos = dronePos;
        simStep[step].direction = currentDir;
        simStep[step].state = droneState;
        simStep[step].targetIdx = bestTarget;
        simStep[step].dropPoint = firePoint;
        simStep[step].aimPoint = dronePos + dir * hDist;
        simStep[step].predictedTarget = bestPred;
        step++;

        Coord hitDiff = simStep[step - 1].aimPoint - simStep[step - 1].predictedTarget;
        if (hitDiff.x * hitDiff.x + hitDiff.y * hitDiff.y <= config.hitRadius * config.hitRadius)
        {
            break;
        }

        prevBestTarget = bestTarget;
        currentTime += config.simTimeStep;
    }

    saveResultFile(simStep, step);

    delete[] ammo;
    ammo = nullptr;
    delete[] simStep;
    simStep = nullptr;
    for (int i = 0; i < targetsQty; i++) {
        delete[] targets[i];
        targets[i] = nullptr;
    }
    delete[] targets;
    targets = nullptr;

    LOG("Simulation finished in " << step << " steps, time=" << currentTime << "s");

    return 0;
}