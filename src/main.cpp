#include <Cosa/RTT.hh>
#include <Cosa/UART.hh>
#include <Cosa/Trace.hh>

#include <wlib/fsm/StateMachine.h>

#include "genfunc.h"
#include "SimplexNoise.h"

#ifndef ASSERT_TRUE
#define ASSERT_TRUE
#endif

using namespace wlp;

class PodData : public EventData {
public:
    double time;
};

class PodMachine : public StateMachine {
public:
    PodMachine() :
        StateMachine(States::ST_MAX_STATES, States::ST_IDLE),
        state(ST_IDLE),
        a(0),
        v(0),
        s(0),
        last_t(0) {}

    void init(double start_time) {
        PodData data;
        data.time = start_time;
        BEGIN_TRANSITION_MAP
                TRANSITION_MAP_ENTRY(ST_READY)          // ST_IDLE
                TRANSITION_MAP_ENTRY(EVENT_IGNORED)     // ST_READY
                TRANSITION_MAP_ENTRY(EVENT_IGNORED)     // ST_ACCEL
                TRANSITION_MAP_ENTRY(EVENT_IGNORED)     // ST_COAST
                TRANSITION_MAP_ENTRY(EVENT_IGNORED)     // ST_BRAKE
                TRANSITION_MAP_ENTRY(EVENT_IGNORED)     // ST_STOPPED
        END_TRANSITION_MAP(&data, PodData)
    }

    void deinit(double start_time) {
        PodData data;
        data.time = start_time;
        BEGIN_TRANSITION_MAP
                TRANSITION_MAP_ENTRY(EVENT_IGNORED)     // ST_IDLE
                TRANSITION_MAP_ENTRY(ST_IDLE)           // ST_READY
                TRANSITION_MAP_ENTRY(EVENT_IGNORED)     // ST_ACCEL
                TRANSITION_MAP_ENTRY(EVENT_IGNORED)     // ST_COAST
                TRANSITION_MAP_ENTRY(EVENT_IGNORED)     // ST_BRAKE
                TRANSITION_MAP_ENTRY(EVENT_IGNORED)     // ST_STOPPED
        END_TRANSITION_MAP(&data, PodData)
    }

    void start(double start_time) {
        PodData data;
        data.time = start_time;
        BEGIN_TRANSITION_MAP
                TRANSITION_MAP_ENTRY(EVENT_IGNORED)     // ST_IDLE
                TRANSITION_MAP_ENTRY(ST_ACCEL)          // ST_READY
                TRANSITION_MAP_ENTRY(EVENT_IGNORED)     // ST_ACCEL
                TRANSITION_MAP_ENTRY(EVENT_IGNORED)     // ST_COAST
                TRANSITION_MAP_ENTRY(EVENT_IGNORED)     // ST_BRAKE
                TRANSITION_MAP_ENTRY(EVENT_IGNORED)     // ST_STOPPED
        END_TRANSITION_MAP(&data, PodData)
    }

    void brake(double start_time) {
        PodData data;
        data.time = start_time;
        BEGIN_TRANSITION_MAP
                TRANSITION_MAP_ENTRY(EVENT_IGNORED)     // ST_IDLE
                TRANSITION_MAP_ENTRY(EVENT_IGNORED)     // ST_READY
                TRANSITION_MAP_ENTRY(ST_BRAKE)          // ST_ACCEL
                TRANSITION_MAP_ENTRY(EVENT_IGNORED)     // ST_COAST
                TRANSITION_MAP_ENTRY(EVENT_IGNORED)     // ST_BRAKE
                TRANSITION_MAP_ENTRY(EVENT_IGNORED)     // ST_STOPPED
        END_TRANSITION_MAP(&data, PodData)
    }

    void noop(double) {}

    void compile(double t) {
        if (last_t > t) {
            last_t = t;
        }
        double dt = t - last_t;
        v += a * dt;
        s += v * dt;
        last_t = t;
        if (v <= 0.035 && state == ST_BRAKE) {
            stop();
        }
    }

    void stop() {
        PodData data;
        data.time = 0;
        BEGIN_TRANSITION_MAP
                TRANSITION_MAP_ENTRY(EVENT_IGNORED)     // ST_IDLE
                TRANSITION_MAP_ENTRY(EVENT_IGNORED)     // ST_READY
                TRANSITION_MAP_ENTRY(EVENT_IGNORED)     // ST_ACCEL
                TRANSITION_MAP_ENTRY(EVENT_IGNORED)     // ST_COAST
                TRANSITION_MAP_ENTRY(ST_STOPPED)        // ST_BRAKE
                TRANSITION_MAP_ENTRY(EVENT_IGNORED)     // ST_STOPPED
        END_TRANSITION_MAP(&data, PodData)
    }

    void s_accel(double time) {
        double t = time - start_time;
        a = p_accel(t);
        compile(t);
    }

    void s_brake(double time) {
        double t = time - start_time;
        a = p_brake(t);
        compile(t);
    }


    void tick(double time) {
        if (state == ST_BRAKE) { s_brake(time); }
        else if (state == ST_ACCEL) { s_accel(time); }
        else { noop(time); }
    }

    enum States {
        ST_IDLE = 0,
        ST_READY,
        ST_ACCEL,
        ST_COAST,
        ST_BRAKE,
        ST_STOPPED,
        ST_MAX_STATES
    };

    States state;
    double start_time;

    double a;
    double v;
    double s;

    double last_t;

private:
    phase_accel p_accel;
    phase_brake p_brake;

    STATE_DECLARE(PodMachine, Idle, PodData);

    STATE_DECLARE(PodMachine, Ready, PodData);

    STATE_DECLARE(PodMachine, Accel, PodData);

    STATE_DECLARE(PodMachine, Coast, PodData);

    STATE_DECLARE(PodMachine, Brake, PodData);

    STATE_DECLARE(PodMachine, Stopped, PodData);

BEGIN_STATE_MAP
                STATE_MAP_ENTRY(&Idle)
                STATE_MAP_ENTRY(&Ready)
                STATE_MAP_ENTRY(&Accel)
                STATE_MAP_ENTRY(&Coast)
                STATE_MAP_ENTRY(&Brake)
                STATE_MAP_ENTRY(&Stopped)
    END_STATE_MAP
};

STATE_DEFINE(PodMachine, Idle, PodData) {
    state = ST_IDLE;
    a = 0;
    v = 0;
    s = 0;
}

STATE_DEFINE(PodMachine, Ready, PodData) {
    state = ST_READY;
    a = 0;
    v = 0;
    s = 0;
}

STATE_DEFINE(PodMachine, Accel, PodData) {
    state = ST_ACCEL;
    start_time = data->time;
    a = 0;
    v = 0;
    s = 0;
}

STATE_DEFINE(PodMachine, Coast, PodData) {
    state = ST_COAST;
}

STATE_DEFINE(PodMachine, Brake, PodData) {
    state = ST_BRAKE;
    start_time = data->time;
}

STATE_DEFINE(PodMachine, Stopped, PodData) {
    state = ST_STOPPED;
    a = 0;
    v = 0;
}

#define INIT    0x01
#define DEINIT  0x02
#define START   0x03
#define BRAKE   0x04

#define P_START         0x56
#define P_END           0x23
#define STATE_ID           0xff00
#define IMU_1           0xff51
#define IMU_2           0xff52
#define IMU_3           0xff53
#define TEMP_1          0xff71
#define TEMP_2          0xff72
#define TEMP_3          0xff73
#define TEMP_4          0xff74

// 8 16 32 32 8
// 1  2  4  4 1 => 12

static double start_time = 0;

static_assert(sizeof(float) == 4, "");
static_assert(sizeof(uint32_t) == 4, "");

void make_pack_i(int val, double time, char *dat, uint16_t id) {
    double t = time - start_time;
    float tp = (float) t;
    dat[0] = P_START;
    *reinterpret_cast<uint16_t *>(dat + 1) = id;
    *reinterpret_cast<uint32_t *>(dat + 1 + 2) = static_cast<uint32_t>(val);
    *reinterpret_cast<float *>(dat + 1 + 2 + 4) = tp;
    dat[11] = P_END;
}

void make_pack_d(double val, double time, char *dat, uint16_t id) {
    double t = time - start_time;
    float tp = (float) t;
    float tv = (float) val;
    dat[0] = P_START;
    *reinterpret_cast<uint16_t *>(dat + 1) = id;
    *reinterpret_cast<float *>(dat + 1 + 2) = tv;
    *reinterpret_cast<float *>(dat + 1 + 2 + 4) = tp;
    dat[11] = P_END;
}

struct float_to_char4 {
    union {
        float v;
        char c[4];
    };
};

static PodMachine pod;

void setup() {
    RTT::begin();
    start_time = RTT::millis() / 1000.0;

    uart.begin(115200);
    trace.begin(&uart);
}

#define IMU_ERR_PERC 17.0

double imu_rand(double a, int id, double time) {
    double maxv = fabs(a * IMU_ERR_PERC / 100.0);
    if (maxv < 1) {
        maxv = 1;
    }
    double noise = SimplexNoise::noise(static_cast<float>(time), id / 10.0f, rand() / 10000.0f) * maxv;
    return a + noise;
}

#define TEMP_ERR_VAL 1.2

double temp_rand(double t, int id, double time) {
    double noise = SimplexNoise::noise(static_cast<float>(time), id / 10.0f, rand() / 10000.0f) * TEMP_ERR_VAL;
    return t + noise;
}

static double prev_time = 0;

static double t1_brake = 10;
static double t3_mot = 10;
static double t4_chip = 10;

#include <math.h>

void chip_t(double time) {
    double t = time - start_time;
    t4_chip = 10 + log(t + 1) * 8;
}

void mot_t(double time) {
    double dt = time - prev_time;
    if (pod.state == PodMachine::ST_ACCEL) {
        t3_mot += dt * 2.7 / (t3_mot + 20) * t3_mot * fabs(pod.a + 4.5) / 20.0;
    } else {
        if (t3_mot >= 10) {
            t3_mot -= 1.5 * dt * (t3_mot - 10) / (t3_mot + 10);
        }
    }
}

void brake_t(double time) {
    double dt = time - prev_time;
    if (pod.state == PodMachine::ST_ACCEL) {
        t1_brake += dt * 0.3f / (t1_brake + 20) * t1_brake;
    } else if (pod.state == PodMachine::ST_BRAKE) {
        t1_brake += dt * 1.5 * (t1_brake + 50) / t1_brake * fabs(pod.a) / 25.0;
    } else {
        if (t1_brake >= 10) {
            t1_brake -= 1.5 * dt * (t1_brake - 10) / (t1_brake + 10);
        }
    }
}

void loop() {
    double time = RTT::millis() / 1000.0;
    double imu_val_raw = pod.a;
    char pack[12];

    double t = time - start_time;


    int cmd = uart.getchar();
    if (cmd >= 0) {
        switch (cmd) {
            case INIT:
                pod.init(t);
                break;
            case DEINIT:
                pod.deinit(t);
                break;
            case START:
                pod.start(t);
                break;
            case BRAKE:
                pod.brake(t);
                break;
            default:
                break;
        }
    }


    memset(pack, 0, 12);
    // State
    make_pack_i(pod.state, time, pack, STATE_ID);
    trace << pack;
    // IMU 1
    make_pack_d(imu_rand(imu_val_raw, IMU_1, t), time, pack, IMU_1);
    trace << pack;
    // IMU 2
    make_pack_d(imu_rand(imu_val_raw, IMU_2, t), time, pack, IMU_2);
    trace << pack;
    // IMU 3
    make_pack_d(imu_rand(imu_val_raw, IMU_3, t), time, pack, IMU_3);
    trace << pack;
    // TEMP 1
    make_pack_d(temp_rand(t1_brake, TEMP_1, t), time, pack, TEMP_1);
    trace << pack;
    // TEMP 2
    make_pack_d(temp_rand(t1_brake, TEMP_2, t), time, pack, TEMP_2);
    trace << pack;
    // TEMP 3
    make_pack_d(temp_rand(t3_mot, TEMP_3, t), time, pack, TEMP_3);
    trace << pack;
    // TEMP 4
    make_pack_d(temp_rand(t4_chip, TEMP_4, t), time, pack, TEMP_4);
    trace << pack;

    make_pack_d(pod.v, time, pack, 5);
    trace << pack;

    make_pack_d(pod.a, time, pack, 6);
    trace << pack;

    make_pack_d(pod.s, time, pack, 7);
    trace << pack;

    make_pack_d(t1_brake, time, pack, 8);
    trace << pack;

    make_pack_d(t3_mot, time, pack, 9);
    trace << pack;

    make_pack_d(t4_chip, time, pack, 10);
    trace << pack;

    // Whoops pod crashed
    if (pod.s >= 30000) { for (;;); }

    pod.tick(t);
    brake_t(t);
    mot_t(t);
    chip_t(time);
    prev_time = t;
}
