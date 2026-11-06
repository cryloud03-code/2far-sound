# 2far sound — VST3 (JUCE / C++)

Efecto de audio con prensa hidráulica animada: CRUSH, COMPRESSOR, FLANGER y DISTORTION.

## Requisitos (Windows 10/11)
- Visual Studio 2022 (carga de trabajo "Desarrollo de escritorio con C++")
- CMake 3.22 o superior
- Git (CMake descarga JUCE 8.0.4 automáticamente la primera vez)

## Compilar
```
cd TwoFarSound
cmake -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
```
Resultado: `build\TwoFarSound_artefacts\Release\VST3\2far sound.vst3`

## Instalar
1. Copia la carpeta `2far sound.vst3` completa a `C:\Program Files\Common Files\VST3\` (requiere permisos de administrador).
2. FL Studio: *Options > Manage plugins > Find installed plugins* (activa "Verify plugins").
3. Cárgalo desde un slot FX del mezclador: Add > More plugins > *2far sound*.

## Arquitectura
- `Source/Parameters.h`  — todos los parámetros (APVTS, automatizables desde FL Studio)
- `Source/DSP.h`         — Compressor, Flanger, Distortion, BitCrusher y máquina de estados de la prensa
- `Source/PluginProcessor.*` — cadena de audio (Distortion → Flanger → Compressor → Crush), detector de transientes, sincronía al tempo
- `Source/PressView.h`   — animación procedural de la prensa y el personaje (deformación progresiva, impacto, salpicaduras)
- `Source/UIComponents.h`, `Source/PluginEditor.*` — knobs metálicos, LEDs, medidores, paneles

## Estado
Ver el mensaje de entrega: el código NO ha sido compilado ni probado todavía en FL Studio.
