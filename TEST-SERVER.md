# Test Server

## IMPORTANT NOTES
1. **Container ID меняется после перезапуска!** Всегда определяй контейнер перед поиском или записью данных:
   ```bash
   sshpass -p '2722025' ssh -p 2222 root@85.88.179.233 "docker ps --format '{{.ID}} {{.Names}}' | grep 21029d2c"
   ```
2. **При краше сервера** - смотри crash dumps в `/home/container/game/csgo/addons/accelerator_local/dumps/`

## Game Server
- **IP:** 85.88.179.233:27017
- **Type:** CS2 Arena 1v1

## SSH Access
- **Host:** 85.88.179.233
- **Port:** 2222
- **User:** root
- **Password:** 2722025

## Docker
- **Container ID:** d3c4731140bc (меняется часто - определяй каждый раз!)
- **Container Name:** 21029d2c-4a07-46c3-b131-e99dbd7b3193
- **Image:** ghcr.io/ghostss2016/cs2-server:latest

## Plugin Path
```
/home/container/game/csgo/addons/cs2-awp-modes/cs2-awp-modes.so
```

## Deploy Commands
```bash
# 1. FIRST: Get current container ID
sshpass -p '2722025' ssh -p 2222 root@85.88.179.233 "docker ps --format '{{.ID}}' | head -1"
# Save this ID as CONTAINER_ID variable

# 2. Copy plugin to server
sshpass -p '2722025' scp -P 2222 /home/code/cs2-build/cs2-awp-modes/build/cs2-awp-modes/linux-x86_64/cs2-awp-modes.so root@85.88.179.233:/tmp/cs2-awp-modes.so

# 3. Copy into container and restart (replace CONTAINER_ID with actual ID)
sshpass -p '2722025' ssh -p 2222 root@85.88.179.233 "docker cp /tmp/cs2-awp-modes.so CONTAINER_ID:/home/container/game/csgo/addons/cs2-awp-modes/cs2-awp-modes.so && docker restart CONTAINER_ID"

# 4. Check logs
sshpass -p '2722025' ssh -p 2222 root@85.88.179.233 "docker logs CONTAINER_ID 2>&1 | grep 'CS2AWPModes'"

# Check crash dumps (IMPORTANT: Always check when server crashes!)
sshpass -p '2722025' ssh -p 2222 root@85.88.179.233 "docker exec CONTAINER_ID ls -la /home/container/game/csgo/addons/accelerator_local/dumps/"

# Copy crash dump to local
sshpass -p '2722025' ssh -p 2222 root@85.88.179.233 "docker cp CONTAINER_ID:/home/container/game/csgo/addons/accelerator_local/dumps/ /tmp/"
sshpass -p '2722025' scp -r -P 2222 root@85.88.179.233:/tmp/dumps /home/code/cs2-build/crash-dumps/
```

## Crash Dumps Location
When server crashes, crash dumps are located at:
```
/home/container/game/csgo/addons/accelerator_local/dumps/
```
