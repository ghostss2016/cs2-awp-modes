# Интернет-анализ режимов AWP для CS2

Проверено 25.09.2026. Звёзды и активность GitHub использованы как проверяемый прокси популярности исходников; открытых данных о реальном онлайне конкретных серверов в найденных репозиториях нет, поэтому утверждать «самый большой онлайн» по одному GitHub нельзя.

| Режим | Найденная основа | Что можно взять | Решение |
|---|---|---|---|
| AWP FFA/Deathmatch | [NockyCZ/CS2-Deathmatch](https://github.com/NockyCZ/CS2-Deathmatch), 173 коммита, 22 форка | мгновенный респавн, FFA, spawn protection, выбор оружия, MultiCFG | лучший кандидат для отдельного FFA-профиля |
| Only-AWP DM | [Only_awp.md](https://github.com/NockyCZ/CS2-Deathmatch/blob/main/Custom%20Modes%20Examples/Only_awp.md) | готовая allow-list AWP, броня, flash, сообщение режима | взять как конфигурационный профиль, не копировать плагин целиком |
| Multi-1v1 / ladder | [rockCityMath/CS2-Multi-1v1](https://github.com/rockCityMath/CS2-Multi-1v1), 30 звёзд | лестница арен, победитель поднимается, проигравший опускается | база для текущего AWP Arena |
| Расширенная арена | [K4-Arenas wiki](https://github-wiki-see.page/m/K4ryuu/K4-Arenas/wiki) | очереди, 2v2/3v3, меню предпочтений, БД, API раундов | ориентир архитектуры, реализация в нативном MetaMod |
| Ротация раундов | [splewis/csgo-multi-1v1](https://github.com/splewis/csgo-multi-1v1) | AWP/pistol/rifle типы раундов и ELO | идеи для `awp_rotation`, без переноса SourceMod-кода |
| AWP Retake | [NeuTroNBZh/CS2-RETAKE](https://github.com/NeuTroNBZh/CS2-RETAKE/) | бомба, выбор оружия, ограничение AWP, спавны по сайтам | отдельный профиль после AWP Arena |
| AWP Retake (SwiftlyS2) | [a2Labs-cc/SwiftlyS2-Retakes](https://github.com/a2Labs-cc/SwiftlyS2-Retakes) | AWP toggle, предпочтения loadout/spawn, cookies | полезно для требований к сохранению настроек |
| Готовый AWP серверный комплект | [ALegitCookie/cs2-dedicatedserver-awp_arena](https://github.com/ALegitCookie/cs2-dedicatedserver-awp_arena) | связка Metamod + CSS + WeaponRestrict + server.cfg | только参考 конфигурации: комплект рассчитан на чистый сервер и не подходит для прямого наложения на флот |
| Ограничитель оружия | [CS2Plugins/WeaponRestrict](https://github.com/CS2Plugins/WeaponRestrict) | квоты AWP, map overrides, VIP bypass | оставить как внешнюю совместимость, в нативном режиме использовать собственный allow-list |

## Выбранный порядок реализации

1. `awp_arena` — уже начат в этом репозитории: AWP-only ladder с allow-list раундов.
2. `awp_rotation` — AWP/Scout/Deagle/Knife через тот же нативный движок и конфиг.
3. `awp_ffa` — отдельный FFA-респавн и защита спавна, после тестов на 04.
4. `awp_retake` — только после отдельной схемы bombsite spawn/config и проверки совместимости с картами.
5. `awp_scoutz`/движение — отдельная карта и серверные параметры, чтобы не менять физику основного режима.
