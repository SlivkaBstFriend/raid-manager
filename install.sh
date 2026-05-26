#!/bin/bash
APPIMAGE="RAID_Manager-x86_64.AppImage"
INSTALL_DIR="$HOME/.local/bin"
DESKTOP_DIR="$HOME/.local/share/applications"

echo "=== Установка RAID Manager ==="

mkdir -p "$INSTALL_DIR"
mkdir -p "$DESKTOP_DIR"

# Создаём wrapper-скрипт вместо прямого копирования AppImage
cp "$APPIMAGE" "$INSTALL_DIR/RAID_Manager-x86_64.AppImage"
chmod +x "$INSTALL_DIR/RAID_Manager-x86_64.AppImage"

# Wrapper который устанавливает нужные переменные
cat > "$INSTALL_DIR/raid-manager" << WRAPPER
#!/bin/bash
export QT_QPA_PLATFORM=xcb
exec "$INSTALL_DIR/RAID_Manager-x86_64.AppImage" "\$@"
WRAPPER
chmod +x "$INSTALL_DIR/raid-manager"

cat > "$DESKTOP_DIR/raid-manager.desktop" << DESKTOP
[Desktop Entry]
Name=RAID Manager
Comment=GUI manager for mdadm RAID arrays
Exec=$INSTALL_DIR/raid-manager
Icon=drive-harddisk
Terminal=false
Type=Application
Categories=System;
DESKTOP

if ! grep -q '.local/bin' ~/.bashrc; then
    echo 'export PATH="$HOME/.local/bin:$PATH"' >> ~/.bashrc
fi

echo "✓ Установлено"
echo "Запуск: $INSTALL_DIR/raid-manager"

# Создаём вспомогательный скрипт сохранения конфига
sudo tee /usr/local/bin/mdadm-save-conf << 'SCRIPT'
#!/bin/bash
CONF=/etc/mdadm/mdadm.conf
grep -v '^ARRAY' "$CONF" > /tmp/mdadm-base.conf
mdadm --detail --scan >> /tmp/mdadm-base.conf
cp /tmp/mdadm-base.conf "$CONF"
update-initramfs -u
SCRIPT
sudo chmod +x /usr/local/bin/mdadm-save-conf
echo "✓ mdadm-save-conf установлен"
