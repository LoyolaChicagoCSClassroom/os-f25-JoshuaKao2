#include "fat.h"
#include "ide.h"
#include "rprintf.h"
#include <stdint.h>
#include <stddef.h>

static void *memcpy_local(void *dest, const void *src, size_t n) {
    uint8_t *d = (uint8_t *)dest;
    const uint8_t *s = (const uint8_t *)src;
    while (n--) *d++ = *s++;
    return dest;
}

static void *memset_local(void *s, int c, size_t n) {
    uint8_t *p = (uint8_t *)s;
    while (n--) *p++ = (uint8_t)c;
    return s;
}

static int memcmp_local(const void *s1, const void *s2, size_t n) {
    const uint8_t *p1 = (const uint8_t *)s1;
    const uint8_t *p2 = (const uint8_t *)s2;
    while (n--) {
        if (*p1 != *p2) return *p1 - *p2;
        p1++;
        p2++;
    }
    return 0;
}


struct boot_sector g_boot_sector;
static uint8_t g_fat_table[65536];
uint32_t g_partition_lba_offset = 2048;
static uint8_t g_is_initialized = 0;


static struct file g_file_handles[8];
static uint8_t g_next_file_handle = 0;

extern int vga_putc(int c);


static void filename_to_fat(const char *filename, char *fat_name, char *fat_ext) {
    int i, j;

    for (i = 0; i < 8; i++) fat_name[i] = ' ';
    for (i = 0; i < 3; i++) fat_ext[i] = ' ';

    for (i = 0; i < 8 && filename[i] && filename[i] != '.'; i++) {
        fat_name[i] = filename[i];
        if (fat_name[i] >= 'a' && fat_name[i] <= 'z') {
            fat_name[i] -= 32;
        }
    }

    while (filename[i] && filename[i] != '.') i++;
    if (filename[i] == '.') i++;


    for (j = 0; j < 3 && filename[i]; j++, i++) {
        fat_ext[j] = filename[i];
        if (fat_ext[j] >= 'a' && fat_ext[j] <= 'z') {
            fat_ext[j] -= 32;
        }
    }
}

int fatInit(void) {
    uint8_t sector_buf[512];
    esp_printf(vga_putc, "Initializing FAT filesystem...\n");


    if (ata_lba_read(g_partition_lba_offset, sector_buf, 1) != 0) {
        esp_printf(vga_putc, "Failed to read boot sector\n");
        return -1;
    }

    memcpy_local(&g_boot_sector, sector_buf, sizeof(struct boot_sector));

    esp_printf(vga_putc, "Bytes per sector: %d\n", g_boot_sector.bytes_per_sector);
    esp_printf(vga_putc, "Sectors per cluster: %d\n", g_boot_sector.num_sectors_per_cluster);
    esp_printf(vga_putc, "Reserved sectors: %d\n", g_boot_sector.num_reserved_sectors);
    esp_printf(vga_putc, "Number of FATs: %d\n", g_boot_sector.num_fat_tables);
    esp_printf(vga_putc, "Root entries: %d\n", g_boot_sector.num_root_dir_entries);
    esp_printf(vga_putc, "Sectors per FAT: %d\n", g_boot_sector.num_sectors_per_fat);
    esp_printf(vga_putc, "Reading FAT table (%d sectors)...\n", g_boot_sector.num_sectors_per_fat);
    uint32_t fat_lba = g_partition_lba_offset + g_boot_sector.num_reserved_sectors;

    uint32_t sectors_to_read = g_boot_sector.num_sectors_per_fat;
    if (sectors_to_read > 128) sectors_to_read = 128;

    if (ata_lba_read(fat_lba, g_fat_table, sectors_to_read) != 0) {
        esp_printf(vga_putc, "Failed to read FAT table\n");
        return -1;
    }
    g_is_initialized = 1;
    g_next_file_handle = 0;
    esp_printf(vga_putc, "FAT filesystem initialized successfully!\n\n");
    return 0;
}


struct file* fatOpen(const char *filename) {
    if (!g_is_initialized) {
        esp_printf(vga_putc, "FAT not initialized\n");
        return NULL;
    }
    if (g_next_file_handle >= 8) {
        esp_printf(vga_putc, "Too many open files\n");
        return NULL;
    }
    char fat_name[8], fat_ext[3];
    filename_to_fat(filename, fat_name, fat_ext);

    esp_printf(vga_putc, "Opening file: %s\n", filename);


    uint32_t root_dir_lba = g_partition_lba_offset +
                            g_boot_sector.num_reserved_sectors +
                            (g_boot_sector.num_fat_tables * g_boot_sector.num_sectors_per_fat);


    uint32_t root_dir_sectors = (g_boot_sector.num_root_dir_entries * 32 + 511) / 512;

    static uint8_t rde_buffer[32768];

    if (ata_lba_read(root_dir_lba, rde_buffer, root_dir_sectors) != 0) {
        esp_printf(vga_putc, "Failed to read root directory\n");
        return NULL;
    }
    struct root_directory_entry *rde_tbl = (struct root_directory_entry *)rde_buffer;


    for (uint32_t i = 0; i < g_boot_sector.num_root_dir_entries; i++) {

        if (rde_tbl[i].file_name[0] == 0x00) {
            break;  
        }

        
        if (rde_tbl[i].file_name[0] == 0xE5) {
            continue;
        }

        
        if (rde_tbl[i].attribute & (FILE_ATTRIBUTE_SUBDIRECTORY | 0x08)) {
            continue;
        }

        
        if (memcmp_local(rde_tbl[i].file_name, fat_name, 8) == 0 &&
            memcmp_local(rde_tbl[i].file_extension, fat_ext, 3) == 0) {

        
            struct file *f = &g_file_handles[g_next_file_handle++];
            memcpy_local(&f->rde, &rde_tbl[i], sizeof(struct root_directory_entry));
            f->start_cluster = rde_tbl[i].cluster;
            f->next = NULL;
            f->prev = NULL;

            esp_printf(vga_putc, "File opened successfully!\n");
            esp_printf(vga_putc, "File size: %d bytes\n", f->rde.file_size);
            esp_printf(vga_putc, "First cluster: %d\n\n", f->start_cluster);

            return f;
        }
    }
    esp_printf(vga_putc, "File not found\n");
    return NULL;
}


int fatRead(struct file *file, void *buffer, uint32_t size) {
    if (!file || !g_is_initialized) {
        return -1;
    }

    if (size > file->rde.file_size) {
        size = file->rde.file_size;
    }
    uint8_t *buf = (uint8_t *)buffer;
    uint32_t bytes_read = 0;
    uint32_t current_cluster = file->start_cluster;


    uint32_t root_dir_sectors = (g_boot_sector.num_root_dir_entries * 32 + 511) / 512;
    uint32_t first_data_sector = g_boot_sector.num_reserved_sectors + (g_boot_sector.num_fat_tables * g_boot_sector.num_sectors_per_fat) +
                                 root_dir_sectors;

    static uint8_t cluster_buf[CLUSTER_SIZE];

    while (size > 0 && current_cluster < 0xFFF8) {

        uint32_t cluster_lba = g_partition_lba_offset + first_data_sector + (current_cluster - 2) * g_boot_sector.num_sectors_per_cluster;

        if (ata_lba_read(cluster_lba, cluster_buf, g_boot_sector.num_sectors_per_cluster) != 0) {
            return -1;
        }

        uint32_t bytes_to_copy = (size < CLUSTER_SIZE) ? size : CLUSTER_SIZE;
        if (bytes_to_copy > (file->rde.file_size - bytes_read)) {
            bytes_to_copy = file->rde.file_size - bytes_read;
        }
        memcpy_local(buf + bytes_read, cluster_buf, bytes_to_copy);

        bytes_read += bytes_to_copy;
        size -= bytes_to_copy;


        if (size > 0) {
            uint16_t *fat16 = (uint16_t *)g_fat_table;
            current_cluster = fat16[current_cluster];
        }
    }

    return bytes_read;
}
