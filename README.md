[README.md](https://github.com/user-attachments/files/26762903/README.md)
# RAID Manager

Графический менеджер RAID-массивов для Linux с интерфейсом на C++/Qt6.

Приложение работает по принципу **Synaptic** — вы набираете очередь операций, затем применяете их все разом. Все операции выполняются через `mdadm` с запросом привилегий через `pkexec` (PolicyKit).

---

## Возможности

- **Визуализация дисков** — карта блочных устройств в стиле Windows Disk Manager: каждый диск отображается в виде цветной полосы с сегментами
- **Управление массивами** — создание, остановка, запуск, удаление RAID-массивов
- **Очередь операций** — набор команд выполняется разом после подтверждения (как в Synaptic)
- **Запасные диски** — добавление spare-дисков в массивы RAID 1/5/6/10
- **Горячее извлечение** — корректное удаление диска через `--fail` + `--remove`
- **Подробности** — просмотр полного вывода `mdadm --detail` прямо в GUI
- **Реальные данные** — читает `/proc/mdstat` и `lsblk` без демо-данных
- **Поддержка уровней** — RAID 0, 1, 5, 6, 10

<img width="957" height="729" alt="изображение" src="https://github.com/user-attachments/assets/a16f79ff-1096-443f-aa28-361c9ecc1a14" />


## Поддерживаемые уровни RAID

| Уровень | Описание | Мин. дисков | Запасные |
|---------|----------|-------------|----------|
| RAID 0  | Чередование (скорость, нет защиты) | 2 | Нет |
| RAID 1  | Зеркало | 2 | Да |
| RAID 5  | Чётность | 3 | Да |
| RAID 6  | Двойная чётность | 4 | Да |
| RAID 10 | Зеркало + чередование | 4 | Да |

---

## Требования

- Linux (Ubuntu 20.04+, Debian 11+, Astra Linux и др.)
- Qt 6.2+ (или Qt 5.12+ с минимальными правками)
- CMake 3.16+
- GCC 10+ / Clang 12+
- `mdadm`
- `pkexec` + `polkitd` (для выполнения команд без постоянного sudo)

---

## Установка зависимостей

**Ubuntu / Debian:**
```bash
sudo apt install build-essential cmake qt6-base-dev mdadm policykit-1
```

**Fedora / RHEL:**
```bash
sudo dnf install gcc-c++ cmake qt6-qtbase-devel mdadm polkit
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

## Запуск

```bash
./build/raid-manager
```

> Приложение запускается от обычного пользователя. При выполнении операций с дисками система запросит пароль через графическое окно PolicyKit.

### Настройка PolicyKit (опционально)

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

## Структура проекта

```
raid-manager/
├── CMakeLists.txt
├── build.sh
└── src/
    ├── main.cpp              # Точка входа
    ├── Models.h              # Структуры данных (DiskInfo, RaidInfo)
    ├── MdadmBackend.cpp/h    # Парсинг /proc/mdstat, lsblk, выполнение команд
    ├── DiskVisualWidget.cpp/h # Визуальная карта дисков (кастомный paintEvent)
    ├── RaidTableWidget.cpp/h  # Таблица RAID-массивов
    ├── QueuePanel.cpp/h       # Панель очереди операций
    ├── CreateRaidDialog.cpp/h # Диалог создания массива
    └── MainWindow.cpp/h       # Главное окно
```

---

## Принцип работы

```
Пользователь выбирает операцию
        ↓
Команда добавляется в очередь (QueuePanel)
        ↓
Пользователь нажимает "Применить"
        ↓
Подтверждение со списком команд
        ↓
pkexec выполняет команды последовательно
        ↓
Интерфейс обновляется через /proc/mdstat
```

---

## Пример команд которые генерирует приложение

**Создание RAID 5:**
```bash
mdadm --create /dev/md0 --level=5 --raid-devices=3 --run /dev/sdb /dev/sdc /dev/sdd
mdadm-save-conf
```

**Остановка массива:**
```bash
mdadm --stop /dev/md0
```

**Извлечение диска:**
```bash
mdadm /dev/md0 --fail /dev/sdb
mdadm /dev/md0 --remove /dev/sdb
```

**Добавление запасного диска:**
```bash
mdadm /dev/md0 --add /dev/sde
```
