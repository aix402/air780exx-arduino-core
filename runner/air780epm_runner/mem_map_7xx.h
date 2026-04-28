#ifndef AIR780EPM_RUNNER_MEM_MAP_7XX_H
#define AIR780EPM_RUNNER_MEM_MAP_7XX_H

/*
 * AIR780EPM Arduino runner flash map override for EC718PM.
 *
 * Goal:
 * - pin the runner to the CSDK package AP limit used by AIR780EPM
 * - keep the 448 KB FOTA window available
 * - keep filesystem and KV regions unchanged
 *
 * Keep this override in the runner. If it is removed, the CSDK falls back to
 * feature-macro-dependent defaults, which is less explicit for Arduino builds.
 */

#ifndef AP_FLASH_LOAD_SIZE
#define AP_FLASH_LOAD_SIZE              (0x2c5000) /* 2836 KB */
#endif

#ifndef FULL_OTA_SAVE_ADDR
#define FULL_OTA_SAVE_ADDR              (0x0)
#endif

#define AP_FLASH_LOAD_UNZIP_SIZE        (AP_FLASH_LOAD_SIZE + 0x20000)

#define FLASH_FOTA_REGION_START         (0x347000)
#define FLASH_FOTA_REGION_LEN           (0x70000)  /* 448 KB total, 352 KB usable after hib backup */
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
#define AP_PKGIMG_LIMIT_SIZE            (0x2c5000)
#endif

#endif
