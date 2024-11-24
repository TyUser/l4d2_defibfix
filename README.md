# [L4D2] defibfix
Сайт: https://forums.alliedmods.net/showthread.php?t=118723

Назначение:
1) Sourcemod плагин игры Left 4 Dead 2 режима Coop 4+ игроков.
2) Дефибриллятор корректно спавнит игроков.

Сборка под debian 10 i386:
* mkdir -p alliedmodders
* cd alliedmodders
* git clone --depth=1 --branch=1.10-dev --recursive https://github.com/alliedmodders/sourcemod sourcemod
* git clone --depth=1 --branch=l4d2 https://github.com/alliedmodders/hl2sdk hl2sdk-l4d2
* git clone --depth=1 --branch=1.10-dev https://github.com/alliedmodders/metamod-source mmsource-1.10
* git clone --recursive https://github.com/TyUser/l4d2_defibfix
* cd l4d2_defibfix
* make -f l4d2_make
