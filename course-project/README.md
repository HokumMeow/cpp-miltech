# Курсова: автономна система повернення додому (RTH) з реальною телеметрією

Програма  читає реальні датчики (GPS GY-GPSV3, IMU GY-9250 та BMP388 які були в мене під рукою) або
програє записаний політ із файлу, стежить за каналом керування і при його втраті сама
"повертає дрон додому".

На виході програма видає стан (`CLIMB`, `FLY_HOME`, `LAND` з курсом, відстанню, висотою) для автопилота.

## Як це влаштовано

 датчики або файл          телеметрія             логіка                   вихід

```
| GPS  (UART)    |   |                |   | FailsafeFsm       |   | консоль        |
| GY-9250 (I2C)  |-->| ITelemetry-    |-->| (стани failsafe)  |-->| RouteLogger    |
| BMP388 (I2C)   |   | Source::next() |   | Navigator         |   | (CSV-маршрут)  |
| клавіатура     |   | -> Telemetry   |   | (курс, відстань)  |   
```

Уся логіка працює з однією структурою `Telemetry` і ядро не знає, звідки вона взялась.
Тому те саме ядро працює на реальних датчиках і на файлі.

Частина модулів взята з попередніх ДЗ

## Автомат failsafe

```
WAIT_FIX --(GPS: >= 4 супутників, запам'ятали дім)--> NORMAL
NORMAL --(зв'язок зник)--> LINK_LOST --(зв'язок повернувся)--> NORMAL
LINK_LOST --(мовчить >= 3 с)--> CLIMB --(набрали 30 м над домом)--> RETURN --(ближче 3 м до дому)--> HOME
```

- Після старту `CLIMB` failsafe **зафіксований**: навіть якщо зв'язок повернувся, дрон летить додому.
- Висота береться з барометра BMP388 відносно висоти в момент запам'ятовування дому.


## Збірка і запуск

Програма і тести збираються з кореня репо:

```bash
# один раз: root CMake збирає і drone_hunter з ДЗ, а той вимагає mavlink
git clone --depth 1 https://github.com/mavlink/c_library_v2.git drone_hunter/c_library_v2

cmake --preset debug
cmake --build --preset debug --target rth_app rth_tests
./build/debug/course-project/rth_tests
```

Або можна зібрати окремо від репозиторію: `cmake -S course-project -B build/course`. Тоді
GoogleTest нізвідки взяти, CMake друкує `rth_tests пропущено` і збирає лише `rth_app`.

**На ПК (з файлов без реальних датчиків):**

```bash
./build/debug/course-project/rth_app --replay course-project/data/scenario_signal_loss.csv
./build/debug/course-project/rth_app --replay course-project/data/scenario_link_flap.csv
# перевірка без GPS в приміщенні:
./build/debug/course-project/rth_app --replay course-project/data/scenario_indoor_bench.csv \
    --no-gps --rth-alt 1 --alt-tolerance 0.3
# запис маршрута у файл, щоб потім відтворити:
./build/debug/course-project/rth_app --replay course-project/data/scenario_signal_loss.csv --log route.csv
```

**На Raspberry Pi (датчики):**

```bash
i2cdetect -y 1        # має показати 0x68 (GY-9250) і 0x76 (барометр)
./build/debug/course-project/rth_app --hardware --log route.csv
```

У терміналі: `l` + Enter вмикає/вимикає "втрату зв'язку", `q` + Enter завершує програму.

Рядок стану показує те, що прийшло з датчиків, і аж потім рекомендацію навігації:

```
[   6.5s] CLIMB     hdg= 90 rp= +5/ -4 alt=  +0.3m t=24.3C pos=0.00000,0.00000 sats=0 link=LOST  -> CLIMB +0.7m
           |          |          |        |        |         |                    |
           |          |          |        |        |         |                    супутники і стан зв'язку
           |          |          |        |        |         координати з GPS
           |          |          |        |        температура з барометра
           |          |          |        висота над домом (барометр)
           |          |          крен / тангаж з акселерометра
           |          курс з магнітометра
           стан автомата failsafe
```

Параметри: `--i2c`, `--gps`, `--gps-baud`, `--imu-addr`, `--baro-addr`, `--heading-offset`, `--rth-alt`, `--alt-tolerance`, `--link-timeout`, `--no-gps` (failsafe). або `--help`.

## Формат сценарію і логу

```
t_ms,lat,lon,alt_m,heading_deg,link,sats[,state,roll_deg,pitch_deg,temp_c]
43000,50.4513700,30.5254000,165.00,45.0,0,8,RETURN,0.0,-5.0,24.0
```

`link`: 1 = зв'язок є, 0 = зникнув. `sats = 0` означає, що GPS не має фіксації. Обов'язкові лише
перші 7 колонок - це все, що потрібно логіці. Решту дописує `--log`, і вона читається назад, якщо є:
`state` для інформації, а `roll_deg`, `pitch_deg`, `temp_c` - щоб відтворити на ПК разом із даними датчиків.

## Що спрощено

- Навігація це лише рекомендація, сімуляції автопилота немає.
- Плата GY-9250 без калібрування, при вмиканні повинна лежати ровно.
- Політ додому прямою, висота набирається до фіксованих 30 м.
- Втрата GPS під час `RETURN`: автомат чекає на повернення сигналу, а навігація видає `HOLD`.

