#!/bin/bash
# build.sh — сборка RAID Manager
set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
BUILD_DIR="$SCRIPT_DIR/build"

echo "=== RAID Manager — сборка ==="

# Проверка зависимостей
check() {
    command -v "$1" &>/dev/null || { echo "Ошибка: не найден $1. Установите: $2"; exit 1; }
}
check cmake   "sudo apt install cmake"
check make    "sudo apt install build-essential"
check pkg-config "sudo apt install pkg-config"

# Проверка Qt6
if ! pkg-config --exists Qt6Widgets 2>/dev/null; then
    echo "Qt6 не найден. Установка:"
    echo "  sudo apt install qt6-base-dev qt6-tools-dev cmake"
    exit 1
fi

mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

echo "→ Конфигурация CMake..."
cmake "$SCRIPT_DIR" -DCMAKE_BUILD_TYPE=Release

echo "→ Сборка ($(nproc) потоков)..."
make -j"$(nproc)"

echo ""
echo "✓ Готово: $BUILD_DIR/raid-manager"
echo ""
echo "Запуск:"
echo "  $BUILD_DIR/raid-manager"
echo ""
echo "Для работы с реальными дисками нужны mdadm и pkexec:"
echo "  sudo apt install mdadm pkexec polkitd"
