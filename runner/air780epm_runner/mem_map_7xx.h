#ifndef AIR780EPM_RUNNER_MEM_MAP_7XX_H
#define AIR780EPM_RUNNER_MEM_MAP_7XX_H

/*
 * AIR780EPM Arduino runner flash map override for EC718PM.
 *
 * Goal:
 * - keep filesystem and KV regions unchanged for the next storage phase
 * - shrink the FOTA window from 448 KB to 256 KB
 * - grow the AP package/image window by 192 KB so TLS CA and MQTTS sketches fit
 *
 * This file is picked up automatically by external/luatos-soc-2024/csdk.lua
 * via __USER_MAP_CONF_FILE__.
 */

#ifndef AP_FLASH_LOAD_SIZE
#define AP_FLASH_LOAD_SIZE              (0x2f5000) /* 3028 KB */
#endif

#ifndef FULL_OTA_SAVE_ADDR
#define FULL_OTA_SAVE_ADDR              (0x0)
#endif

#define AP_FLASH_LOAD_UNZIP_SIZE        (AP_FLASH_LOAD_SIZE + 0x20000)

#define FLASH_FOTA_REGION_START         (0x377000)
#define FLASH_FOTA_REGION_LEN           (0x40000)  /* 256 KB total, 160 KB usable after hib backup */
#define FLASH_FOTA_REGION_END           (0x3b7000)

#define FLASH_FS_REGION_START           (0x3b7000)
#define FLASH_FS_REGION_END             (0x3e1000)
#define FLASH_FS_REGION_SIZE            (FLASH_FS_REGION_END - FLASH_FS_REGION_START) /* 168 KB */

#define FLASH_FDB_REGION_START          (0x3e1000) /* 64 KB */
#define FLASH_FDB_REGION_END            (0x3f1000)

#define FLASH_HIB_BACKUP_EXIST          (1)
#define FLASH_MEM_BACKUP_ADDR           (AP_FLASH_XIP_ADDR + FLASH_MEM_BACKUP_NONXIP_ADDR)
#define FLASH_MEM_BACKUP_NONXIP_ADDR    (FLASH_FOTA_REGION_END - FLASH_MEM_BACKUP_SIZE)
#define FLASH_MEM_BACKUP_SIZE           (0x18000) /* 96 KB */
#define FLASH_MEM_BLOCK_SIZE            (0x6000)
#define FLASH_MEM_BLOCK_CNT             (0x4)

#ifndef AP_PKGIMG_LIMIT_SIZE
#define AP_PKGIMG_LIMIT_SIZE            (0x2f5000)
#endif

#endif
