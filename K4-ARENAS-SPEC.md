# K4-Arenas - Полная спецификация для реимплементации

> **ВАЖНО**: При реализации каждой функции проводить тщательную проверку соответствия оригиналу.
> Если функциональность неясна или отсутствует, сверяться с эталонными репозиториями.

## Эталонные репозитории для проверки

| Репозиторий | Путь | Назначение |
|-------------|------|------------|
| **K4-Arenas** | `/home/code/cs2-build/K4-Arenas` | Оригинальный плагин (основной эталон) |
| **cs2-menus-new-fixed** | `/home/code/cs2-build/cs2-menus-new-fixed` | Система меню, UI компоненты |
| **CSS-src** | `/home/code/cs2-build/CSS-src` | CounterStrikeSharp исходники, API |
| **metamod-source** | `/home/code/cs2-build/metamod-source` | Metamod интерфейсы, хуки |
| **hl2sdk-cs2** | `/home/code/cs2-build/hl2sdk-cs2` | SDK CS2, игровые структуры, энтити |

---

## 1. Структура проекта

```
K4-Arenas/
├── src-plugin/                    # Основной плагин арен
│   ├── Plugin/
│   │   ├── Plugin.cs              # Ядро плагина, инициализация
│   │   ├── PluginConfig.cs        # Определения конфигурации
│   │   ├── PluginDatabase.cs      # MySQL операции
│   │   ├── PluginAPI.cs           # Shared API обработчик
│   │   ├── PluginCommands.cs      # Команды игроков
│   │   ├── PluginEvents.cs        # Обработчики игровых событий
│   │   ├── PluginListeners.cs     # Tick и listener обработчики
│   │   ├── PluginStock.cs         # Утилиты
│   │   └── Models/
│   │       ├── ArenaModel.cs      # Состояние одной арены
│   │       ├── ArenaPlayerModel.cs # Данные игрока арены
│   │       ├── ArenasModel.cs     # Коллекция всех арен
│   │       ├── ArenaResultModel.cs # Отслеживание результатов раунда
│   │       ├── ArenaRoundTypeModel.cs # Определения типов раундов
│   │       ├── WeaponModel.cs     # Классификация оружия
│   │       ├── ChallengeModel.cs  # Система вызовов/дуэлей
│   │       ├── GameConfigModel.cs # Загрузчик игровой конфигурации
│   │       └── ArenaFinder.cs     # Обнаружение спавн-точек
│   └── lang/
│       ├── en.json                # Английские переводы
│       └── pl.json                # Польские переводы
├── src-botsplugin/                # Аддон поддержки ботов
│   └── K4-Arenas-Bots.cs
└── src-shared/                    # Shared API для плагинов
    └── K4-ArenaSharedApi.cs
```

---

## 2. Схема базы данных

### Таблица: `k4-arenas` (с настраиваемым префиксом)

```sql
CREATE TABLE `k4-arenas` (
    `steamid64` BIGINT UNIQUE,
    `rifle` INT,
    `sniper` INT,
    `shotgun` INT,
    `smg` INT,
    `lmg` INT,
    `pistol` INT,
    `rounds` VARCHAR(256) NOT NULL,
    `lastseen` TIMESTAMP NOT NULL
);
```

### Поля

| Поле | Тип | Описание |
|------|-----|----------|
| `steamid64` | BIGINT | Steam ID игрока (первичный ключ) |
| `rifle` | INT | Предпочтение оружия (CsItem enum) |
| `sniper` | INT | Предпочтение снайперки |
| `shotgun` | INT | Предпочтение дробовика |
| `smg` | INT | Предпочтение ПП |
| `lmg` | INT | Предпочтение пулемёта |
| `pistol` | INT | Предпочтение пистолета |
| `rounds` | VARCHAR(256) | Список включённых типов раундов через запятую |
| `lastseen` | TIMESTAMP | Время последнего посещения |

### Операции с БД

| Метод | Описание |
|-------|----------|
| `LoadPlayerAsync()` | Загрузка предпочтений при входе; создание записи если новый |
| `SavePlayerPreferencesAsync()` | Сохранение предпочтений оружия и раундов (при отключении) |
| `PurgeDatabaseAsync()` | Удаление записей старше N дней (по умолчанию 30) |
| `CreateTableAsync()` | Инициализация таблицы при необходимости |

> **ПРОВЕРКА**: Сверить SQL запросы с `PluginDatabase.cs` в оригинале.
> При проблемах с MySQL смотреть `CSS-src` для примеров работы с БД.

---

## 3. Конфигурация

### Файл: `K4-Arenas.json`

#### Настройки базы данных

```json
{
  "Database": {
    "Host": "localhost",
    "Username": "root",
    "Password": "",
    "Database": "cs2",
    "Port": 3306,
    "SSLMode": "preferred",
    "TablePrefix": "",
    "PurgeDays": 30
  }
}
```

#### Настройки команд

| Параметр | Значение по умолчанию | Описание |
|----------|----------------------|----------|
| `gun-commands` | `guns,gunpref,weaponpref` | Команды меню оружия |
| `round-commands` | `rounds,roundpref` | Команды меню раундов |
| `queue-commands` | `queue` | Команды статуса очереди |
| `afk-commands` | `afk` | Команды переключения AFK |
| `challenge-commands` | `challenge,duel` | Команды вызова на дуэль |
| `challenge-accept` | `caccept,capprove` | Команды принятия вызова |
| `challenge-decline` | `cdecline,cdeny` | Команды отклонения вызова |
| `center-menu-mode` | `true` | Центральное меню vs чат |
| `center-announcements` | `true` | HTML объявления в центре |
| `freeze-in-menu` | `false` | Заморозка в меню |
| `show-menu-credits` | `true` | Показывать кредиты в меню |

#### Настройки совместимости

| Параметр | Описание |
|----------|----------|
| `force-arena-clantags` | Принудительное обновление клантегов каждый тик |
| `block-flash-of-not-opponent` | Блокировать ослепление от других арен |
| `block-damage-of-not-opponent` | Блокировать урон от других арен |
| `give-knife-by-default` | Всегда давать нож |
| `disable-clantags` | Отключить изменение клантегов |
| `prevent-draw-rounds` | Случайно выбирать победителя при ничьей |

#### Оружие по умолчанию

```json
{
  "default-rifle": "AK47",
  "default-sniper": "AWP",
  "default-smg": "MP7",
  "default-lmg": "M249",
  "default-shotgun": "XM1014",
  "default-pistol": "Deagle",
  "default-round": 1
}
```

---

## 4. Типы раундов

### Структура RoundType

```cpp
struct RoundType {
    int ID;                          // Уникальный идентификатор
    string Name;                     // Ключ перевода
    CsItem* PrimaryWeapon;           // Конкретное оружие (null = предпочтение)
    WeaponType* PrimaryPreference;   // Тип предпочитаемого оружия
    CsItem* SecondaryWeapon;         // Вторичное оружие
    bool UsePreferredPrimary;        // Использовать предпочтение игрока?
    bool UsePreferredSecondary;      // Использовать предпочтение пистолета?
    bool Armor;                      // Давать броню?
    bool Helmet;                     // Давать шлем?
    int TeamSize;                    // Размер команды (1-10)
    bool EnabledByDefault;           // Включён для новых игроков?
    Action StartFunction;            // Кастомный обработчик старта
    Action EndFunction;              // Кастомный обработчик конца
};
```

### Стандартные типы раундов

| ID | Тип | Размер | Оружие | Броня | Шлем |
|----|-----|--------|--------|-------|------|
| 1 | Rifle | 1v1 | Предпочитаемая винтовка | Да | Да |
| 2 | Sniper | 1v1 | Предпочитаемая снайперка | Да | Да |
| 3 | Shotgun | 1v1 | Предпочитаемый дробовик | Да | Да |
| 4 | Pistol | 1v1 | Предпочитаемый пистолет | Да | Да |
| 5 | Scout | 1v1 | SSG08 | Да | Да |
| 6 | AWP | 1v1 | AWP | Да | Да |
| 7 | Deagle | 1v1 | Только Deagle | Нет | Нет |
| 8 | SMG | 1v1 | Предпочитаемый ПП | Да | Да |
| 9 | LMG | 1v1 | Предпочитаемый пулемёт | Да | Да |
| 10 | Knife | 1v1 | Только нож | Нет | Нет |
| 11 | 2v2 | 2v2 | Полная экипировка | Да | Да |
| 12 | 3v3 | 3v3 | Полная экипировка | Да | Да |

---

## 5. Классификация оружия

### WeaponType enum

```cpp
enum WeaponType {
    Rifle,
    Sniper,
    Shotgun,
    SMG,
    LMG,
    Pistol
};
```

### Список оружия по типам

| Тип | Оружие |
|-----|--------|
| **Rifle** | AK47, M4A1S, M4A1, GalilAR, Famas, SG556, AUG |
| **Sniper** | AWP, SSG08 (Scout), SCAR20, G3SG1 |
| **Shotgun** | XM1014, Nova, MAG7, SawedOff |
| **SMG** | Mac10, MP9, MP7, P90, MP5SD, Bizon, UMP45 |
| **LMG** | M249, Negev |
| **Pistol** | Deagle, Glock, USPS, HKP2000, Elite, Tec9, P250, CZ75, FiveSeven, Revolver |

> **ПРОВЕРКА**: Сверить CsItem enum значения с `hl2sdk-cs2` для корректных ID.

---

## 6. Команды

### Команды игроков

| Команда | Тип | Функция |
|---------|-----|---------|
| `!queue` | Статус | Проверить позицию в очереди |
| `!guns` / `!gunpref` / `!weaponpref` | Меню | Открыть меню предпочтений оружия |
| `!rounds` / `!roundpref` | Меню | Открыть меню предпочтений раундов |
| `!afk` | Переключатель | Переключить AFK статус |
| `!challenge` / `!duel [имя]` | Действие | Вызвать игрока на дуэль |
| `!caccept` / `!capprove` | Действие | Принять вызов |
| `!cdecline` / `!cdeny` | Действие | Отклонить вызов |

> **ПРОВЕРКА**: Меню реализации сверять с `cs2-menus-new-fixed` для корректного отображения.

---

## 7. Система арен

### Архитектура арены

Каждая арена содержит:
- Два списка спавн-точек (T и CT)
- Команда 1 (Террористы)
- Команда 2 (Контр-террористы)
- Текущий тип раунда
- ID арены (номер, -1=warmup, -2=challenge)
- Счёт (для расчёта позиции в очереди)
- Отслеживание результата (Win/Tie/NoOpponent/Empty)

### Состояния арены

| Состояние | Описание |
|-----------|----------|
| **Active Arena** | Есть игроки обеих команд, никто не AFK |
| **Has Finished** | Все игроки мертвы, одна команда ушла, или один игрок в дуэли |
| **Has Real Players** | Минимум один не-бот игрок присутствует |
| **Empty Arena** | Нет назначенных игроков |

### Поток матчмейкинга

```
1. Игрок заходит на сервер → Добавляется в WaitingArenaPlayers

2. EventRoundPrestart (между раундами):
   a) Подсчёт результатов предыдущих арен
   b) Обработка победителей арен → ранжированная очередь (высокий приоритет)
   c) Обработка проигравших арен → ранжированная очередь (низкий приоритет)
   d) Обработка ожидающих игроков → ранжированная очередь (самый низкий)
   e) Перемешивание арен для честности
   f) Сначала обработка челленджей (получают выделенную арену -2)
   g) Затем назначение оставшихся игроков на обычные арены:
      - Сначала попытка командных раундов (2v2, 3v3)
      - Откат на 1v1 матчи
      - Одинокий игрок получает арену один (состояние NoOpponent)
   h) Оставшиеся игроки остаются в очереди ожидания

3. Подсчёт очков арены:
   Отображаемый счёт = (Всего_Арен - ID_Текущей_Арены) * 50
```

### Обнаружение спавн-точек (ArenaFinder)

```
1. Найти все info_player_terrorist и info_player_counterterrorist спавны
2. Проверить наличие info_teleport_destination (карты CYBERSHOKE)
3. Если телепорты есть: объединить их как отдельные арены
4. Иначе: использовать алгоритм пространственной кластеризации:
   - MERGE_THRESHOLD = 1.5 (единицы расстояния)
   - FACTOR = 1.1 (коэффициент масштабирования)
   - Объединить географически близкие спавн-точки
5. Залогировать поддерживаемые режимы арен
```

> **ПРОВЕРКА**: Для работы с энтити `info_player_*` и `info_teleport_destination`
> смотреть `hl2sdk-cs2` для корректных имён классов и методов доступа.

---

## 8. Система вызовов/дуэлей

### Структура ChallengeModel

```cpp
struct ChallengeModel {
    ArenaPlayer* Player1;
    ArenaPlayer* Player2;
    int Player1Placement;      // Где был в очереди
    int Player2Placement;      // Где был в очереди
    bool IsAccepted;
    bool IsEnded;
};
```

### Поток вызова

1. Игрок A вызывает игрока B через `!challenge <имя>`
2. Вызываемый получает уведомление с опциями `!caccept` / `!cdecline`
3. Оба игрока должны быть на активной арене (не в ожидании)
4. Боты автоматически принимают вызов через 1 секунду
5. Челлендж получает выделенную арену (ArenaID = -2)
6. По завершении:
   - Победитель помещается впереди в очереди на своё оригинальное место
   - Проигравший помещается после победителя
   - Челлендж помечается завершённым и удаляется

---

## 9. Система предпочтений

### Предпочтения раундов

- Булево переключение для каждого типа раунда
- Минимум 1 раунд должен быть включён
- Хранится как список ID через запятую
- Значения по умолчанию на основе `EnabledByDefault`

### Предпочтения оружия

- Выбор одного оружия на тип
- Может быть null (случайное)
- Меню показывает галочки для выбранного
- Сохраняется в БД сразу при изменении

---

## 10. Система AFK

### Поведение

```
При включении AFK:
  - Игрок перемещается в наблюдатели
  - Клантег меняется на "AFK |"
  - Перемещение в WaitingArenaPlayers очередь
  - Раунд арены завершается если игрок был в активном матче

При отключении:
  - Клантег меняется на "WAITING |"

Каждый раунд:
  - Напоминание если всё ещё AFK
```

---

## 11. Обработка событий

| Событие | Обработчик | Действия |
|---------|------------|----------|
| `OnMapStart` | Обнаружение спавнов, инит арен, перезагрузка конфигов, настройка игроков |
| `OnMapEnd` | Очистка арен, очистка очередей |
| `EventPlayerActivate` | Настройка нового игрока в очереди |
| `EventPlayerDisconnect` | Удаление из очереди/арен, завершение раунда при необходимости |
| `EventRoundPrestart` | Подсчёт результатов, переназначение игроков на арены |
| `EventRoundStart` | Очистка флага between-rounds |
| `EventRoundEnd` | Подсчёт победителей, отслеживание MVP |
| `EventPlayerSpawn` | Настройка позиции/оружия игрока для типа раунда |
| `EventPlayerBlind` | Блокировка вспышки от не-оппонентов (если включено) |
| `EventPlayerHurt` | Блокировка урона от не-оппонентов (если включено) |
| `EventRoundFreezeEnd` | Очистка центральных сообщений |

> **ПРОВЕРКА**: Для регистрации событий сверять с `CSS-src` (`IEventManager`, `RegisterEventHandler`).
> Для хуков Metamod смотреть `metamod-source`.

---

## 12. Клантеги

| Состояние | Клантег |
|-----------|---------|
| Арена N | `ARENA N \|` |
| Ожидание | `WAITING \|` |
| AFK | `AFK \|` |
| Warmup | `WARMUP \|` |
| Челлендж | `CHALLENGE \|` |

---

## 13. Shared API

### Интерфейс IK4ArenaSharedApi

```cpp
interface IK4ArenaSharedApi {
    int AddSpecialRound(
        string name,
        int teamSize,
        bool enabledByDefault,
        Action<List<CCSPlayerController>*, List<CCSPlayerController>*> startFunc,
        Action<List<CCSPlayerController>*, List<CCSPlayerController>*> endFunc
    );

    void RemoveSpecialRound(int id);
    int GetArenaPlacement(CCSPlayerController player);
    string GetArenaName(CCSPlayerController player);
    bool IsAFK(CCSPlayerController player);
    List<CCSPlayerController> FindOpponents(CCSPlayerController player);
    void TerminateRoundIfPossible();
    void PerformAFKAction(CCSPlayerController player, bool afk);
    CsItem* GetPlayerWeaponPreference(CCSPlayerController player, WeaponType weaponType);
};
```

### Пример использования

```cpp
auto capability = PluginCapability<IK4ArenaSharedApi>("k4-arenas:sharedapi");
auto api = capability.Get();
api->AddSpecialRound("Custom Round", 1, true, StartFunc, EndFunc);
```

---

## 14. Тайминги и циклы

| Операция | Интервал |
|----------|----------|
| Принудительное обновление клантегов | 1 секунда |
| Заполнение warmup | 2 секунды |
| Отображение центральных сообщений | Каждый тик (OnTick) |

---

## 15. Типы результатов

| Результат | Описание |
|-----------|----------|
| **Win** | Обе команды присутствовали, явный победитель |
| **Tie** | Обе команды присутствовали, ничья по живым |
| **NoOpponent** | Только одна команда, засчитывается как победа |
| **Empty** | Нет команд (не должно назначать арену) |

---

## 16. Настройка оружия при спавне

```
SetupWeapons(RoundType):
  1. Очистить всё оружие
  2. Дать нож (если включено в конфиге)
  3. Если раунд имеет конкретное PrimaryWeapon: дать его
     Иначе если UsePreferredPrimary: дать предпочитаемое оружие типа
  4. Если раунд имеет конкретное SecondaryWeapon: дать его
     Иначе если UsePreferredSecondary: дать предпочитаемый пистолет
  5. Установить значения брони/шлема
```

> **ПРОВЕРКА**: Для выдачи оружия использовать `GiveNamedItem` из `hl2sdk-cs2`.
> Для удаления оружия смотреть `CSS-src` методы инвентаря.

---

## 17. Локализация

### Поддерживаемые языки

- Английский (`en.json`)
- Польский (`pl.json`)

### Формат файла

```json
{
  "arena.round.rifle": "Rifle",
  "arena.round.sniper": "Sniper",
  "arena.queue.position": "Your queue position: {0}",
  ...
}
```

---

## 18. Чеклист проверки при реализации

### Для каждой функции:

- [ ] Сверить логику с оригинальным `K4-Arenas`
- [ ] Проверить корректность enum значений через `hl2sdk-cs2`
- [ ] Проверить регистрацию событий через `CSS-src`
- [ ] Проверить работу меню через `cs2-menus-new-fixed`
- [ ] Проверить хуки Metamod через `metamod-source`

### Критические компоненты:

| Компонент | Файл оригинала | Эталон для проверки |
|-----------|----------------|---------------------|
| Регистрация событий | `PluginEvents.cs` | `CSS-src` |
| Работа с игроком | `ArenaPlayerModel.cs` | `hl2sdk-cs2`, `CSS-src` |
| Меню | `PluginCommands.cs` | `cs2-menus-new-fixed` |
| База данных | `PluginDatabase.cs` | `CSS-src` (MySqlConnector) |
| Энтити (спавны) | `ArenaFinder.cs` | `hl2sdk-cs2` |
| API хуки | `PluginAPI.cs` | `metamod-source` |

---

## 19. Известные edge cases

1. **Hotreload**: При горячей перезагрузке восстановить всех игроков в очереди
2. **Смена карты**: Пересоздать Arenas объект, очистить очереди и челленджи
3. **Дисконнект во время матча**: Завершить раунд, засчитать победу оппоненту
4. **Бот в челлендже**: Автопринятие через 1 секунду
5. **Один игрок на сервере**: Арена с состоянием NoOpponent
6. **AFK во время матча**: Завершить раунд, переместить в наблюдатели
