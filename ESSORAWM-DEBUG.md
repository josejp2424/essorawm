# Diagnóstico del parpadeo de wallpaper y duplicación de wbar

Esta revisión incorpora un registro de diagnóstico liviano. Está desactivado
por defecto y puede habilitarse sin reiniciar EssoraWM.

## Cómo reproducir el problema

```sh
essorawm-debug start
```

Después:

1. Abrir el selector de wallpaper.
2. Abrir otra ventana mientras el selector continúa abierto.
3. Esperar unos segundos si comienza el parpadeo.
4. Aplicar un wallpaper para comprobar también el comportamiento de wbar.
5. Detener el registro:

```sh
essorawm-debug stop
```

Guardar una copia permanente:

```sh
essorawm-debug collect /root/essorawm-problem.log
```

También se puede consultar directamente:

```sh
essorawm-debug show
```

## Qué registra

- Inicio, reinicio y cierre del proceso principal de EssoraWM.
- Ejecución repetida de los comandos de inicio.
- Creación y eventos `Expose`/`ConfigureNotify` del selector de wallpaper.
- Creación, recarga, redimensionado y repintado del escritorio nativo.
- Cambios detectados en la firma de unidades.
- Cantidad de procesos `wbar` antes y después de cambiar el wallpaper.

El archivo predeterminado es:

```text
/tmp/essorawm-debug-UID.log
```

`UID` corresponde al número del usuario que ejecuta EssoraWM. En Puppy,
normalmente será `/tmp/essorawm-debug-0.log`.

## Correcciones preventivas incluidas

- Los `ConfigureNotify` que sólo modifican el apilado ya no provocan un
  repintado completo del escritorio.
- Los eventos `Expose` acumulados del selector se agrupan antes de dibujar.
- El reinicio de wbar espera la finalización de la instancia anterior y no
  inicia otra si un autostart o supervisor ya la volvió a ejecutar.
