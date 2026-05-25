# Essora JWM internal thumbnail preview

Agregado por josejp2424.

Esta versión mueve el preview/thumbnail del `TaskList` dentro de JWM.
Ya no depende de `/usr/local/bin/essora-jwm-preview` ni de un lanzador externo.

Archivos nuevos:

- `src/taskpreview.c`
- `src/taskpreview.h`

Archivos modificados:

- `src/taskbar.c`
- `src/Makefile.in`

Funcionamiento:

- JWM guarda una miniatura/cache de la ventana cuando está visible.
- Al pasar el mouse sobre una tarea del panel, muestra un popup interno con la imagen.
- Si una ventana está minimizada, se intenta usar la última miniatura guardada.
- No muestra texto dentro del preview; solo la imagen con borde verde Essora.

Notas:

- Este primer parche usa X11/XGetImage para mantener pocas dependencias.
- Algunas ventanas pueden no entregar imagen si el toolkit bloquea la captura, si están minimizadas antes de que exista cache, o si el compositor/driver no entrega contenido.
- Una mejora futura sería usar XComposite/XDamage para cache más preciso, similar a tint2.
