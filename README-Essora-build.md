# Compilar EssoraWM 0.1.7

El repositorio está preparado para mantenerse limpio. El script de compilación
copia el source a un directorio temporal, genera allí los archivos de Autotools,
compila y crea el árbol del paquete sin dejar `Makefile`, `config.h`, objetos,
catálogos `.gmo` ni binarios dentro del repositorio.

Como usuario root:

```sh
chmod +x build-essora-jwm-deb.sh
./build-essora-jwm-deb.sh
```

El paquete final queda en:

```text
deb-output/essorawm_0.1.7_<arquitectura>.deb
```

Ejemplo para amd64:

```sh
dpkg -i deb-output/essorawm_0.1.7_amd64.deb
```

## Dependencias de compilación

```text
build-essential autoconf automake gettext pkg-config
libx11-dev libxext-dev libxrender-dev libxmu-dev libxinerama-dev
libxpm-dev libjpeg-dev libpng-dev libcairo2-dev librsvg2-dev
libxft-dev libpango1.0-dev
```

El escritorio nativo no necesita GTK2, GTK3 ni ROX-Filer.

## Compilación manual

```sh
./autogen.sh
./configure --prefix=/usr --sysconfdir=/etc --localstatedir=/var
make -j"$(nproc)"
```

Los archivos generados por una compilación manual están cubiertos por
`.gitignore`. Para quitarlos:

```sh
make distclean
```
