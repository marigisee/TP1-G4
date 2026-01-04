En esta carpeta de adjuntan los archivos correspondientes a una prueba de latencia.
La prueba consiste en realizar un "semi-mock" del emisor, dónde se simula la generación de datos IMU pero la pulsación de botones es real.
Por lo tanto, la idea es que el emisor envíe datos por ESP-NOW a la esp32 receptora solo cuando algun botón es apretado.
La idea de este test es medir "que tanto" se banca la EDUCIAA la llegada de datos y la generación de comandos MIDI al módulo vs1053b.
