#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>


typedef uint8_t bool;
#define true 1
#define false 0


typedef struct {
     
    uint8_t boot_jump_instruction[3];
    uint8_t oem_indentifier;
    uint16_t bytes_per_sector;
    uint8_t sectors_per_cluster;
    uint16_t reserved_sectors;
    uint8_t fat_count;
    uint16_t dir_entries_count;
    uint16_t total_sectors;
    uint8_t media_descriptor_typr;
    uint16_t sectors_per_fat;
    uint16_t sectors_per_track;
    uint16_t heads;
    uint32_t hidden_sectors;
    uint32_t large_sector_count;

    uint8_t drive_number;
    uint8_t _reserved;
    uint8_t signature;
    uint32_t volume_id;
    uint8_t volume_label[11];
    uint8_t system_id[8];

} __attribute__((packed)) BootSector;


typedef struct {

    uint8_t name[11];
    uint8_t attributes;
    uint8_t  _reserved;
    uint8_t created_time_tenths;
    uint16_t created_time;
    uint16_t created_date;
    uint16_t accessed_date;
    uint16_t first_cluster_high;
    uint16_t modified_time;
    uint16_t modified_date;
    uint16_t first_cluster_low;
    uint32_t size;

} __attribute__((packed)) DirectoryEntry;


BootSector g_boot_sector;
uint8_t* g_fat = NULL;
DirectoryEntry* g_root_directory = NULL;


bool read_boot_sector(FILE* disk) {

    return fread(&g_boot_sector, sizeof(g_boot_sector), 1, disk) > 0;

}


bool read_sectors(FILE* disk, uint32_t lba, uint32_t count, void* buffer_out) {

    bool ok = true;
    ok = ok && (fseek(disk, lba * g_boot_sector.bytes_per_sector, SEEK_SET));
    ok = ok && (fread(buffer_out, g_boot_sector.bytes_per_sector, count, disk) == count);
    return ok;

}


bool read_fat(FILE* disk) {

    g_fat = (uint8_t*) malloc(g_boot_sector.sectors_per_fat * g_boot_sector.bytes_per_sector);
    return read_sectors(disk, g_boot_sector.reserved_sectors, g_boot_sector.sectors_per_fat, g_fat);

}


bool read_root_directory(FILE* disk) {

    uint32_t lba = g_boot_sector.reserved_sectors + g_boot_sector.sectors_per_fat * g_boot_sector.fat_count;
    uint32_t size = sizeof(DirectoryEntry) * g_boot_sector.dir_entries_count;
    uint32_t sectors = (size / g_boot_sector.bytes_per_sector);
    if (size % g_boot_sector.bytes_per_sector > 0)
        sectors++;
    g_root_directory = (DirectoryEntry*) malloc(sectors * g_boot_sector.bytes_per_sector);
    return read_sectors(disk, lba, sectors, g_root_directory);

}


DirectoryEntry* find_file(const char* name) {

    for (uint32_t i = 0; i < g_boot_sector.dir_entries_count; i++) {
        if (memcmp(name, g_root_directory[i].name, 11) == 0) {
            return &g_root_directory[i];
        }

    }

    return NULL;
}


int main(int argc, char** argv) {

    if (argc < 3) {
        printf("Syntax: %s <disk_image> <file name>\n", argv[0]);
        return -1;
    }

    FILE* disk = fopen(argv[1], "rb");
    if (!disk) {
        fprintf(stderr, "Cannot open disk image %s!\n", argv[1]);
        return -1;
    }

    if (!read_boot_sector(disk)) {
        fprintf(stderr, "Could not read boot sector!\n");
        return -2;
    }

    if (!read_fat(disk)) {
        fprintf(stderr, "Could not read FAT!\n");
        free(g_fat);
        return -3;
    }
    
    if (!read_root_directory(disk)) {
        fprintf(stderr, "Could not read root!\n");
        free(g_fat);
        free(g_root_directory);
        return -4;
    }

    DirectoryEntry* fileEntry = find_file(argv[2]);
    if (!fileEntry) {
        fprintf(stderr, "Could not read the file %s!\n", argv[2]);
        free(g_fat);
        free(g_root_directory);
        return -5;
    }

    free(g_fat);
    free(g_root_directory);
    
    return 0;
}