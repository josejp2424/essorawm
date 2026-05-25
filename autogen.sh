#!/bin/sh
# JWM (Essora edition - Pymenu patch) autogen.sh
# Regenera todos los archivos necesarios para ./configure.
# No depende de automake porque el proyecto usa Makefile.in plantillas
# directas (heredado del estilo de JWM upstream).

set -e

# 1. Crear directorio para archivos auxiliares.
mkdir -p build-aux

# 2. Copiar archivos auxiliares de automake si faltan.
for f in config.guess config.sub install-sh missing compile; do
    if [ ! -f "build-aux/$f" ]; then
        for d in /usr/share/automake-*/; do
            if [ -f "$d/$f" ]; then
                cp "$d/$f" "build-aux/$f"
                break
            fi
        done
    fi
done

# 3. Generar config.rpath dentro de build-aux (necesario para gettext).
if [ ! -f build-aux/config.rpath ]; then
    if [ -f /usr/share/gettext/config.rpath ]; then
        cp /usr/share/gettext/config.rpath build-aux/config.rpath
    elif [ -f /usr/share/automake-1.16/config.rpath ]; then
        cp /usr/share/automake-1.16/config.rpath build-aux/config.rpath
    else
        touch build-aux/config.rpath
        chmod +x build-aux/config.rpath
    fi
fi
# También copia legacy en raíz (por si AM_GNU_GETTEXT lo busca ahí).
if [ ! -f config.rpath ]; then
    cp build-aux/config.rpath config.rpath 2>/dev/null || touch config.rpath
fi

# 4. Generar po/Makefile.in.in si falta (gettext template).
if [ ! -f po/Makefile.in.in ]; then
    if [ -f /usr/share/gettext/po/Makefile.in.in ]; then
        cp /usr/share/gettext/po/Makefile.in.in po/Makefile.in.in
    fi
fi

# 5. Ejecutar autoconf tools en orden.
aclocal -I m4 2>/dev/null || aclocal
autoheader
autoconf

echo ""
echo "=== JWM (Pymenu patch): build prepared ==="
echo "Now run: ./configure [options]  &&  make"
