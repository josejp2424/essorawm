#!/bin/bash
# build-essora-jwm-deb.sh
# Autor: josejp2424 / Essora
# Crea un paquete .deb de EssoraWM basado en JWM.
# Versión del paquete: 0.1

set -e

PKGNAME="essorawm"
VERSION="0.1"
ARCH="$(dpkg --print-architecture 2>/dev/null || echo amd64)"
MAINTAINER="josejp2424 <puppylinuxjosejp2424@gmail.com>"
HOMEPAGE="https://github.com/josejp2424/essorawm"

SRC_DIR="$(cd "$(dirname "$0")" && pwd)"
BUILD_ROOT="${SRC_DIR}/build-deb"
STAGE="${BUILD_ROOT}/${PKGNAME}_${VERSION}_${ARCH}"
OUT_DIR="${SRC_DIR}/deb-output"

msg() { printf '\033[1;32m==>\033[0m %s\n' "$*"; }
err() { printf '\033[1;31mERROR:\033[0m %s\n' "$*" >&2; exit 1; }

check_cmd() {
    command -v "$1" >/dev/null 2>&1 || err "Falta el comando: $1"
}

msg "Comprobando herramientas necesarias..."
for cmd in make gcc dpkg-deb install strip sed awk; do
    check_cmd "$cmd"
done

if [ ! -x "${SRC_DIR}/configure" ]; then
    if [ -x "${SRC_DIR}/autogen.sh" ]; then
        msg "No existe configure, ejecutando autogen.sh..."
        (cd "$SRC_DIR" && ./autogen.sh)
    else
        err "No existe configure ni autogen.sh ejecutable."
    fi
fi

msg "Limpiando compilaciones anteriores..."
rm -rf "$BUILD_ROOT" "$OUT_DIR"
mkdir -p "$STAGE" "$OUT_DIR"

msg "Configurando JWM..."
(
    cd "$SRC_DIR"
    make distclean >/dev/null 2>&1 || true
    ./configure \
        --prefix=/usr \
        --sysconfdir=/etc \
        --localstatedir=/var
)

# agregado por josejp2424
# EssoraWM conserva el binario/textdomain como jwm.
# Por eso los locales deben instalarse como:
#   /usr/share/locale/<lang>/LC_MESSAGES/jwm.mo
if [ -f "${SRC_DIR}/po/Makefile" ]; then
    sed -i 's|^LOCALEDIR *=.*|LOCALEDIR = /usr/share/locale|' "${SRC_DIR}/po/Makefile"
    sed -i 's|^PACKAGE *=.*|PACKAGE = jwm|' "${SRC_DIR}/po/Makefile"
fi
if [ -f "${SRC_DIR}/po/Makefile.in" ]; then
    sed -i 's|^LOCALEDIR *=.*|LOCALEDIR = /usr/share/locale|' "${SRC_DIR}/po/Makefile.in"
    sed -i 's|^PACKAGE *=.*|PACKAGE = jwm|' "${SRC_DIR}/po/Makefile.in"
fi

msg "Compilando..."
make -C "$SRC_DIR" -j"$(nproc 2>/dev/null || echo 2)"

msg "Instalando en carpeta temporal..."
make -C "$SRC_DIR" DESTDIR="$STAGE" install


BAD_LOCALE="${STAGE}@LOCALEDIR@"
GOOD_LOCALE="${STAGE}/usr/share/locale"
if [ -d "$BAD_LOCALE" ]; then
    mkdir -p "$GOOD_LOCALE"
    cp -a "$BAD_LOCALE"/. "$GOOD_LOCALE"/ 2>/dev/null || true
    rm -rf "$BAD_LOCALE"
fi

# agregado por josejp2424
# Fallback fuerte: asegurar que todos los .po/.gmo queden como jwm.mo
# dentro del paquete, aunque el Makefile de po no los instale por alguna razón.
if [ -d "${SRC_DIR}/po" ]; then
    for po_file in "${SRC_DIR}"/po/*.po; do
        [ -f "$po_file" ] || continue
        lang="$(basename "$po_file" .po)"
        gmo_file="${SRC_DIR}/po/${lang}.gmo"

        if [ ! -s "$gmo_file" ]; then
            if command -v msgfmt >/dev/null 2>&1; then
                msgfmt -o "$gmo_file" "$po_file" 2>/dev/null || true
            fi
        fi

        if [ -s "$gmo_file" ]; then
            locale_dir="${STAGE}/usr/share/locale/${lang}/LC_MESSAGES"
            install -d "$locale_dir"
            install -m 0644 "$gmo_file" "$locale_dir/jwm.mo"
        fi
    done
fi

# Preview/thumbnail integrado dentro de JWM.
# Agregado por josejp2424: ya no se instala helper externo; el código vive en src/taskpreview.c.

# Documentación mínima.
install -d "${STAGE}/usr/share/doc/${PKGNAME}"
{
    echo "${PKGNAME} (${VERSION}) Essora build"
    echo
    echo "EssoraWM 0.1 basado en JWM 2.4.7."
    echo "Incluye llamada a /usr/local/bin/pymenu y preview/thumbnail interno, agregado por josejp2424."
} > "${STAGE}/usr/share/doc/${PKGNAME}/README.Essora"

gzip -9 -n "${STAGE}/usr/share/doc/${PKGNAME}/README.Essora" || true

# Quitar símbolos para reducir tamaño.
if [ -x "${STAGE}/usr/bin/jwm" ]; then
    strip --strip-unneeded "${STAGE}/usr/bin/jwm" 2>/dev/null || true
fi

msg "Creando metadatos Debian..."
install -d "${STAGE}/DEBIAN"

INSTALLED_SIZE="$(du -sk "$STAGE" | awk '{print $1}')"

cat > "${STAGE}/DEBIAN/control" <<CONTROL
Package: ${PKGNAME}
Version: ${VERSION}
Section: x11
Priority: optional
Architecture: ${ARCH}
Maintainer: ${MAINTAINER}
Installed-Size: ${INSTALLED_SIZE}
Depends: libc6, libx11-6, libxext6, libxft2, libxinerama1, libxpm4, libjpeg62-turbo | libjpeg62, libpng16-16, librsvg2-2
Recommends: hsetroot | feh | xwallpaper
Provides: jwm
Conflicts: jwm
Replaces: jwm
Homepage: ${HOMEPAGE}
Description: EssoraWM based on JWM with Pymenu launcher support
 EssoraWM 0.1 keeps the /usr/bin/jwm binary for compatibility.
 Based on JWM 2.4.7.
 Calls /usr/local/bin/pymenu, includes TaskList thumbnail-only previews, and adds a native wallpaper selector, agregado por josejp2424.
CONTROL

cat > "${STAGE}/DEBIAN/postinst" <<'POSTINST'
#!/bin/sh
set -e

# Regenerar menú de Essora si existe el generador.
if [ -x /usr/local/essora-kit/essora-menu-gen.py ]; then
    /usr/local/essora-kit/essora-menu-gen.py >/dev/null 2>&1 || true
fi

exit 0
POSTINST
chmod 755 "${STAGE}/DEBIAN/postinst"

cat > "${STAGE}/DEBIAN/postrm" <<'POSTRM'
#!/bin/sh
set -e
exit 0
POSTRM
chmod 755 "${STAGE}/DEBIAN/postrm"

msg "Corrigiendo permisos..."
find "$STAGE" -type d -exec chmod 755 {} \;
find "$STAGE" -type f -name '*.la' -delete

DEB_FILE="${OUT_DIR}/${PKGNAME}_${VERSION}_${ARCH}.deb"
msg "Construyendo paquete .deb..."
dpkg-deb --build "$STAGE" "$DEB_FILE"

# agregado por josejp2424
# Dejar también el .deb en la misma carpeta donde se compila el source.
cp -f "$DEB_FILE" "${SRC_DIR}/${PKGNAME}_${VERSION}_${ARCH}.deb"

msg "Listo: $DEB_FILE"
msg "Para instalar: sudo dpkg -i '$DEB_FILE'"
