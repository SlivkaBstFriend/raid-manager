[README.md](https://github.com/user-attachments/files/26762903/README.md)
# RAID Manager

Графический менеджер RAID-массивов для Linux с интерфейсом на C++/Qt6.

Приложение работает по принципу **Synaptic** — вы набираете очередь операций, затем применяете их все разом. Все операции выполняются через `mdadm` с запросом привилегий через `pkexec` (PolicyKit).

---

## Возможности

- **Визуализация дисков** — карта блочных устройств в стиле Windows Disk Manager: каждый диск отображается в виде цветной полосы с сегментами
- **Очередь операций** — набор команд выполняется разом после подтверждения (как в Synaptic)
- **Создание массивов** — RAID 0, 1, 5, 6, 10 с живым предпросмотром команды mdadm
- **Управление жизненным циклом** — остановка, запуск, удаление массивов
- **Горячее управление дисками** — добавление запасных дисков, горячее извлечение
- **Подробности** — просмотр полного вывода `mdadm --detail` прямо в GUI
- **Определение деградации** — корректное отображение статуса при отказе диска
- **Защита занятых дисков** — диски уже входящие в массив недоступны для выбора

<img width="957" height="729" alt="изображение" src="https://github.com/user-attachments/assets/a16f79ff-1096-443f-aa28-361c9ecc1a14" />


## Поддерживаемые уровни RAID

| Уровень | Описание | Мин. дисков | Запасные диски |
|---------|----------|-------------|----------------|
| RAID 0  | Чередование (скорость, нет защиты) | 2 | Нет |
| RAID 1  | Зеркало | 2 | Да |
| RAID 5  | Чётность | 3 | Да |
| RAID 6  | Двойная чётность | 4 | Да |
| RAID 10 | Зеркало + чередование | 4 | Да |

---

## Требования

- Linux (Ubuntu 22.04+, Debian 11+, Linux Mint 22+, Astra Linux и др.)
- Qt 6.2+
- CMake 3.16+
- GCC 10+ / Clang 12+
- `mdadm`
- `pkexec` + `polkitd`


---

## Установка зависимостей

**Ubuntu / Debian / Linux Mint:**
```bash
sudo apt install build-essential cmake qt6-base-dev mdadm polkitd pkexec
```
---

## Сборка

```bash
git clone https://github.com/SlivkaBstFriend/raid-manager.git
cd raid-manager
chmod +x build.sh
./build.sh
```

Бинарный файл появится в `build/raid-manager`.

---


```bash
# Автоматическая установка
bash install.sh

# Запуск
raid-manager
```

Скрипт `install.sh`:
- Копирует AppImage в `~/.local/bin/`
- Создаёт ярлык в меню приложений
- Устанавливает вспомогательный скрипт `mdadm-save-conf`

> **Совместимость AppImage:** файл собран на Linux Mint 22.3 (GLIBC 2.39) и совместим с Ubuntu 22.04+, Linux Mint 22+, Debian 12+ и другими современными дистрибутивами.

---

## Запуск

```bash
# Из исходников
./build/raid-manager

# Через AppImage
QT_QPA_PLATFORM=xcb ~/RAID_Manager-x86_64.AppImage
```

При нажатии «Применить» система запросит пароль через графическое окно PolicyKit.

---

## Структура проекта

```
raid-manager/
├── CMakeLists.txt
├── build.sh                  ← Скрипт сборки
├── install.sh                ← Скрипт установки AppImage
├── raid-manager.desktop      ← Файл интеграции с меню
├── icons/
│   └── raid-manager.png
└── src/
    ├── main.cpp              ← Точка входа
    ├── Models.h              ← Структуры данных (DiskInfo, RaidInfo)
    ├── MdadmBackend.cpp/h    ← Парсинг /proc/mdstat, lsblk, выполнение команд
    ├── DiskVisualWidget.cpp/h ← Визуальная карта дисков (кастомный paintEvent)
    ├── RaidTableWidget.cpp/h  ← Таблица массивов
    ├── QueuePanel.cpp/h       ← Панель очереди операций
    ├── CreateRaidDialog.cpp/h ← Диалог создания массива
    └── MainWindow.cpp/h       ← Главное окно
```

---

## Принцип работы

```
Пользователь выбирает операцию
        ↓
Команда добавляется в очередь (QueuePanel)
        ↓
Пользователь нажимает «Применить»
        ↓
Подтверждение со списком команд
        ↓
pkexec выполняет команды последовательно
        ↓
Интерфейс обновляется через /proc/mdstat
```

---

## Тестирование

Приложение протестировано на двух дистрибутивах Linux:

| Дистрибутив | Уровни RAID | Результат |
|-------------|-------------|-----------|
| Ubuntu 26.04 LTS | RAID 1, RAID 5 | ✓ Все тесты пройдены |
| Linux Mint 22.3 | RAID 0, RAID 10 | ✓ Все тесты пройдены |

Проверены операции: создание, остановка, запуск, удаление, извлечение диска, добавление запасного диска, просмотр подробностей.

---

## Настройка PolicyKit (опционально)

Чтобы не вводить пароль при каждой операции:

```bash
sudo tee /etc/polkit-1/rules.d/99-raid-manager.rules << 'EOF'
polkit.addRule(function(action, subject) {
    if (action.id == "org.freedesktop.policykit.exec" &&
        subject.isInGroup("sudo")) {
        return polkit.Result.YES;
    }
});
EOF
```

---
