#!/bin/bash
# build-essora-jwm-deb.sh
# Autor: josejp2424 / Essora
# Compila EssoraWM y crea un paquete Debian sin ensuciar el árbol del source.

set -euo pipefail

PKGNAME="essorawm"
VERSION="0.1.6"
ARCH="$(dpkg --print-architecture 2>/dev/null || echo amd64)"
MAINTAINER="josejp2424 <puppylinuxjosejp2424@gmail.com>"
HOMEPAGE="https://github.com/josejp2424/essorawm"

SRC_DIR="$(cd "$(dirname "$0")" && pwd)"
OUT_DIR="${SRC_DIR}/deb-output"
WORK_ROOT="${TMPDIR:-/tmp}/essorawm-${VERSION}-package-$$"
COMPILE_DIR="${WORK_ROOT}/source"
STAGE="${WORK_ROOT}/package/${PKGNAME}_${VERSION}_${ARCH}"

msg() { printf '\033[1;32m==>\033[0m %s\n' "$*"; }
err() { printf '\033[1;31mERROR:\033[0m %s\n' "$*" >&2; exit 1; }
check_cmd() { command -v "$1" >/dev/null 2>&1 || err "Falta el comando: $1"; }

cleanup() {
    rm -rf "$WORK_ROOT"
}

abort_build() {
    trap - EXIT INT TERM HUP
    cleanup
    printf '\033[1;31mERROR:\033[0m Compilación interrumpida; no se creó un paquete parcial.\n' >&2
    exit 130
}

trap cleanup EXIT
trap abort_build INT TERM HUP

msg "Comprobando herramientas necesarias..."
for cmd in make gcc pkg-config dpkg-deb install strip awk sed tar gzip \
           autoconf autoheader aclocal; do
    check_cmd "$cmd"
done

rm -rf "$WORK_ROOT"
mkdir -p "$COMPILE_DIR" "$STAGE" "$OUT_DIR"

msg "Copiando el source a un directorio temporal..."
tar -C "$SRC_DIR" \
    --exclude='./.git' \
    --exclude='./autom4te.cache' \
    --exclude='./build' \
    --exclude='./build-deb' \
    --exclude='./deb-output' \
    --exclude='./package-root' \
    --exclude='./*.deb' \
    --exclude='./*.o' \
    --exclude='./src/jwm' \
    --exclude='./Makefile' \
    --exclude='./src/Makefile' \
    --exclude='./contrib/Makefile' \
    --exclude='./po/Makefile' \
    --exclude='./po/Makefile.in.in' \
    --exclude='./po/*.gmo' \
    --exclude='./config.cache' \
    --exclude='./config.log' \
    --exclude='./config.status' \
    --exclude='./config.h' \
    --exclude='./config.h.in' \
    --exclude='./configure' \
    --exclude='./jwm.1' \
    -cf - . | tar -C "$COMPILE_DIR" -xf -

msg "Generando los archivos de configuración dentro del directorio temporal..."
(
    cd "$COMPILE_DIR"
    chmod +x ./autogen.sh
    ./autogen.sh
)

msg "Configurando EssoraWM..."
(
    cd "$COMPILE_DIR"
    CONFIG_SHELL=/bin/bash SHELL=/bin/bash ./configure \
        --prefix=/usr \
        --sysconfdir=/etc \
        --localstatedir=/var
)

# EssoraWM conserva el binario y el textdomain jwm por compatibilidad.
if [ -f "${COMPILE_DIR}/po/Makefile" ]; then
    sed -i 's|^LOCALEDIR *=.*|LOCALEDIR = /usr/share/locale|' \
        "${COMPILE_DIR}/po/Makefile"
    sed -i 's|^PACKAGE *=.*|PACKAGE = jwm|' \
        "${COMPILE_DIR}/po/Makefile"
fi

msg "Compilando EssoraWM con escritorio nativo..."
make -C "$COMPILE_DIR" -j"$(nproc 2>/dev/null || echo 2)"

msg "Instalando en el árbol temporal del paquete..."
make -C "$COMPILE_DIR" DESTDIR="$STAGE" install

BAD_LOCALE="${STAGE}@LOCALEDIR@"
GOOD_LOCALE="${STAGE}/usr/share/locale"
if [ -d "$BAD_LOCALE" ]; then
    mkdir -p "$GOOD_LOCALE"
    cp -a "$BAD_LOCALE"/. "$GOOD_LOCALE"/ 2>/dev/null || true
    rm -rf "$BAD_LOCALE"
fi

# Compilar e instalar todas las traducciones disponibles.
if [ -d "${COMPILE_DIR}/po" ]; then
    for po_file in "${COMPILE_DIR}"/po/*.po; do
        [ -f "$po_file" ] || continue
        lang="$(basename "$po_file" .po)"
        gmo_file="${COMPILE_DIR}/po/${lang}.gmo"
        if command -v msgfmt >/dev/null 2>&1; then
            msgfmt -o "$gmo_file" "$po_file" 2>/dev/null || true
        elif command -v python3 >/dev/null 2>&1 \
             && [ -x "${COMPILE_DIR}/tools/po_tools.py" ]; then
            python3 "${COMPILE_DIR}/tools/po_tools.py" compile \
                "$po_file" "$gmo_file" 2>/dev/null || true
        fi
        if [ -s "$gmo_file" ]; then
            locale_dir="${STAGE}/usr/share/locale/${lang}/LC_MESSAGES"
            install -d "$locale_dir"
            install -m 0644 "$gmo_file" "$locale_dir/jwm.mo"
        fi
    done
fi

msg "Instalando documentación..."
install -d "${STAGE}/usr/share/doc/${PKGNAME}"
cat > "${STAGE}/usr/share/doc/${PKGNAME}/README.Essora" <<DOC
${PKGNAME} (${VERSION}) Essora build

EssoraWM ${VERSION}, basado en JWM 2.4.7.
Incluye escritorio X11 nativo, iconos de aplicaciones y unidades, lectura y
escritura compatible de PuppyPin, movimiento persistente, menú contextual,
selector de wallpaper con SVG, Pymenu, menús con iconos y previews internos.

No ejecuta ROX-Filer ni desktop_drive_icons para administrar el escritorio.
DOC
install -m 0644 "${SRC_DIR}/README.md" \
    "${STAGE}/usr/share/doc/${PKGNAME}/README.md"
install -m 0644 "${SRC_DIR}/README-Native-Desktop.md" \
    "${STAGE}/usr/share/doc/${PKGNAME}/README-Native-Desktop.md"
install -m 0644 "${SRC_DIR}/README-Desktop-Applications.md" \
    "${STAGE}/usr/share/doc/${PKGNAME}/README-Desktop-Applications.md"
gzip -9 -n "${STAGE}/usr/share/doc/${PKGNAME}/README.Essora" || true

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
Depends: libc6, libx11-6, libjpeg62-turbo | libjpeg62, libpng16-16, util-linux
Recommends: udisks2, librsvg2-bin | imagemagick | inkscape, hsetroot | feh | xwallpaper, essorafm | xdg-utils
Provides: jwm
Conflicts: jwm
Replaces: jwm
Homepage: ${HOMEPAGE}
Description: EssoraWM based on JWM with a native X11 desktop
 EssoraWM ${VERSION} keeps /usr/bin/jwm for compatibility and is based on
 JWM 2.4.7. It includes a native desktop window, PuppyPin-compatible
 application and drive icons, persistent drag positions, configurable layouts,
 contextual actions, wallpaper integration, Pymenu and internal previews. It
 does not require ROX-Filer or the external desktop_drive_icons program.
CONTROL

cat > "${STAGE}/DEBIAN/postinst" <<'POSTINST'
#!/bin/sh
set -e
if [ -x /usr/local/essora-kit/essora-menu-gen.py ]; then
    /usr/local/essora-kit/essora-menu-gen.py >/dev/null 2>&1 || true
fi

# Puppy expects this conventional launcher. Preserve any script already
# supplied by Puppy or the distribution.
if [ ! -e /usr/local/bin/defaultfilemanager ] \
   && [ ! -L /usr/local/bin/defaultfilemanager ]; then
    mkdir -p /usr/local/bin
    ln -s /usr/bin/essorawm-filemanager /usr/local/bin/defaultfilemanager
fi
exit 0
POSTINST
chmod 755 "${STAGE}/DEBIAN/postinst"

cat > "${STAGE}/DEBIAN/postrm" <<'POSTRM'
#!/bin/sh
set -e
if [ "${1:-}" = purge ] \
   && [ -L /usr/local/bin/defaultfilemanager ] \
   && [ "$(readlink /usr/local/bin/defaultfilemanager 2>/dev/null || true)" = /usr/bin/essorawm-filemanager ]; then
    rm -f /usr/local/bin/defaultfilemanager
fi
exit 0
POSTRM
chmod 755 "${STAGE}/DEBIAN/postrm"

find "$STAGE" -type d -exec chmod 755 {} \;
find "$STAGE" -type f -name '*.la' -delete

DEB_FILE="${OUT_DIR}/${PKGNAME}_${VERSION}_${ARCH}.deb"
rm -f "$DEB_FILE"
msg "Construyendo paquete .deb..."
dpkg-deb --build "$STAGE" "$DEB_FILE"

msg "Listo: $DEB_FILE"
msg "El source permaneció limpio; los archivos temporales serán eliminados."
msg "Instalación como root: dpkg -i '$DEB_FILE'"
