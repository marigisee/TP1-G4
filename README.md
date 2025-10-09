[![License](https://img.shields.io/badge/License-BSD%203--Clause-blue.svg)](https://opensource.org/licenses/BSD-3-Clause) [![Build Status](https://travis-ci.com/epernia/firmware_v3.svg?branch=master)](https://travis-ci.com/epernia/firmware_v3) (Travis CI status)

# DIPESIM - Dispositivo inalámbrico para ejecución y simulación de instrumentos MIDI 

### Descripción

Se desea diseñar un dispositivo electrónico que permita la ejecución y simulación de instrumentos musicales MIDI de forma inalámbrica. El dispositivo a partir de los pulsadores y sensores correspondientes debe ser capaz de reproducir los acordes o notas que el usuario desee. El dispositivo estará divido en dos bloques: bloque portátil y bloque fijo.

El bloque portátil estará compuesto por un microcontrolador ESP32, una serie de pulsadores y un MPU6050 (acelerómetro y giroscopio) que permitirán al usuario ejecutar las notas o acordes deseados realizando diferentes gestos o movimientos con la mano.

El bloque fijo estará compuesto por un microcontrolador ESP32, la placa EDU-CIAA y el módulo sintetizador MIDI VS1053B. Este bloque recibirá las notas o acordes ejecutados por el bloque portátil y los reproducirá a través del módulo sintetizador MIDI.s