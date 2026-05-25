# JWM Pymenu Essora 2.4.7-2

Compilar y crear paquete `.deb`:

```bash
chmod +x build-essora-jwm-deb.sh
./build-essora-jwm-deb.sh
```

El paquete queda en:

```bash
deb-output/jwm-pymenu_2.4.7-2_amd64.deb
```

Instalación:

```bash
sudo dpkg -i deb-output/jwm-pymenu_2.4.7-2_amd64.deb
```

Dependencias de compilación recomendadas en Devuan/Debian:

```bash
sudo apt install build-essential autoconf automake pkg-config libx11-dev libxext-dev libxft-dev libxinerama-dev libxpm-dev libjpeg-dev libpng-dev librsvg2-dev
```
