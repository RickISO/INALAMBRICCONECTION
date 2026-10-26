
#include <Arduino.h>

// Pines de motores
const uint32_t motorPWM[4] = {
    PA8, PB10, PB4, PB6
};

const uint32_t motorIN1[4] = {
    PC7, PA6, PA0, PA4
};

const uint32_t motorIN2[4] = {
    PA9, PA5, PA1, PB0
};

// Encoders
const uint32_t encoderA[4] = {
    PA10, PB5, PC6, PC9
};

const uint32_t encoderB[4] = {
    PB3, PA7, PC8, PC10
};

// Invertir motores 2, 3 y 4
const bool motorInvertido[4] = {
    false, true, true, true
};

// Ajustar segun el sentido de cada encoder
const int encoderSigno[4] = {
    1, 1, 1, 1
};

// Configuracion de prueba
const unsigned long TIEMPO_MUESTREO = 100;
const unsigned long TIEMPO_PRBS = 500;
const unsigned long DURACION_PRUEBA = 60000;

// PWM de 8 bits
const int PWM_BAJO = 77;   // 30 %
const int PWM_ALTO = 153;  // 60 %

// IMPORTANTE: calibrar con tu encoder real
const float CUENTAS_POR_REV = 360.0f;

volatile long pulsos[4] = {0, 0, 0, 0};

void contarEncoder(int i)
{
    bool a = digitalRead(encoderA[i]);
    bool b = digitalRead(encoderB[i]);

    if (a == b)
        pulsos[i]++;
    else
        pulsos[i]--;
}

void encoder1() { contarEncoder(0); }
void encoder2() { contarEncoder(1); }
void encoder3() { contarEncoder(2); }
void encoder4() { contarEncoder(3); }

void configurarMotor(int i, int pwm)
{
    pwm = constrain(pwm, 0, 255);

    if (pwm == 0)
    {
        analogWrite(motorPWM[i], 0);
        digitalWrite(motorIN1[i], LOW);
        digitalWrite(motorIN2[i], LOW);
        return;
    }

    digitalWrite(
        motorIN1[i],
        motorInvertido[i] ? LOW : HIGH
    );

    digitalWrite(
        motorIN2[i],
        motorInvertido[i] ? HIGH : LOW
    );

    analogWrite(motorPWM[i], pwm);
}

void detenerMotores()
{
    for (int i = 0; i < 4; i++)
        configurarMotor(i, 0);
}

void setup()
{
    Serial.begin(115200);

    for (int i = 0; i < 4; i++)
    {
        pinMode(motorPWM[i], OUTPUT);
        pinMode(motorIN1[i], OUTPUT);
        pinMode(motorIN2[i], OUTPUT);

        pinMode(encoderA[i], INPUT_PULLUP);
        pinMode(encoderB[i], INPUT_PULLUP);
    }

    detenerMotores();

    attachInterrupt(
        digitalPinToInterrupt(encoderA[0]),
        encoder1,
        CHANGE
    );

    attachInterrupt(
        digitalPinToInterrupt(encoderA[1]),
        encoder2,
        CHANGE
    );

    attachInterrupt(
        digitalPinToInterrupt(encoderA[2]),
        encoder3,
        CHANGE
    );

    attachInterrupt(
        digitalPinToInterrupt(encoderA[3]),
        encoder4,
        CHANGE
    );

    analogWriteResolution(8);

    randomSeed(analogRead(PA3));

    delay(1500);

    Serial.println(
        "tiempo_ms,"
        "pwm1_pct,pwm2_pct,pwm3_pct,pwm4_pct,"
        "pulsos1,pulsos2,pulsos3,pulsos4,"
        "rpm1,rpm2,rpm3,rpm4,"
        "motor1_rad_s,motor2_rad_s,"
        "motor3_rad_s,motor4_rad_s"
    );
}

void loop()
{
    static bool iniciado = false;
    static bool terminado = false;

    static unsigned long inicio = 0;
    static unsigned long ultimoMuestreo = 0;
    static unsigned long ultimoPRBS = 0;

    static long pulsosAnteriores[4] = {
        0, 0, 0, 0
    };

    static int pwmActual = PWM_BAJO;

    if (terminado)
        return;

    unsigned long ahora = millis();

    if (!iniciado)
    {
        iniciado = true;

        inicio = ahora;
        ultimoMuestreo = ahora;
        ultimoPRBS = ahora;

        pwmActual = random(0, 2)
                    ? PWM_ALTO : PWM_BAJO;

        for (int i = 0; i < 4; i++)
            configurarMotor(i, pwmActual);

        return;
    }

    unsigned long tiempo = ahora - inicio;

    if (tiempo >= DURACION_PRUEBA)
    {
        detenerMotores();
        terminado = true;
        Serial.println("# FIN_PRBS");
        return;
    }

    // Cambiar PWM cada 500 ms
    if (ahora - ultimoPRBS >= TIEMPO_PRBS)
    {
        ultimoPRBS += TIEMPO_PRBS;

        pwmActual = random(0, 2)
                    ? PWM_ALTO : PWM_BAJO;

        for (int i = 0; i < 4; i++)
            configurarMotor(i, pwmActual);
    }

    // Adquirir datos cada 100 ms
    if (ahora - ultimoMuestreo >= TIEMPO_MUESTREO)
    {
        float dt = (ahora - ultimoMuestreo) / 1000.0f;

        long conteos[4];

        noInterrupts();

        for (int i = 0; i < 4; i++)
            conteos[i] = pulsos[i];

        interrupts();

        float rpm[4];
        float velocidad[4];

        for (int i = 0; i < 4; i++)
        {
            long delta = conteos[i] - pulsosAnteriores[i];

            rpm[i] = encoderSigno[i] *
                     (delta * 60.0f) /
                     (CUENTAS_POR_REV * dt);

            velocidad[i] = rpm[i] * 2.0f * PI / 60.0f;

            pulsosAnteriores[i] = conteos[i];
        }

        Serial.print(ahora - inicio);

        // PWM aplicado
        for (int i = 0; i < 4; i++)
        {
            Serial.print(",");
            Serial.print(pwmActual * 100.0f / 255.0f, 2);
        }

        // Pulsos acumulados
        for (int i = 0; i < 4; i++)
        {
            Serial.print(",");
            Serial.print(conteos[i]);
        }

        // RPM
        for (int i = 0; i < 4; i++)
        {
            Serial.print(",");
            Serial.print(rpm[i], 2);
        }

        // Velocidad angular rad/s
        for (int i = 0; i < 4; i++)
        {
            Serial.print(",");
            Serial.print(velocidad[i], 3);
        }

        Serial.println();

        ultimoMuestreo = ahora;
    }
}
