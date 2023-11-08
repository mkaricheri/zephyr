/*
 * Copyright (c) 2016 Linaro Limited
 *               2016 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/storage/flash_map.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <stdio.h>

#define TEST_PARTITION          scratch_partition
#define IMAGE1_PARTITION        image1_partition
#define IMAGE2_PARTITION        image2_partition

#define TEST_PARTITION_OFFSET	FIXED_PARTITION_OFFSET(TEST_PARTITION)
#define TEST_PARTITION_DEVICE	FIXED_PARTITION_DEVICE(TEST_PARTITION)
#define TEST_PARTITION_SIZE    FIXED_PARTITION_SIZE(TEST_PARTITION)

#define IMAGE1_PARTITION_OFFSET	FIXED_PARTITION_OFFSET(IMAGE1_PARTITION)
#define IMAGE1_PARTITION_DEVICE	FIXED_PARTITION_DEVICE(IMAGE1_PARTITION)
#define IMAGE1_PARTITION_SIZE    FIXED_PARTITION_SIZE(IMAGE1_PARTITION)

#define IMAGE2_PARTITION_OFFSET	FIXED_PARTITION_OFFSET(IMAGE2_PARTITION)
#define IMAGE2_PARTITION_DEVICE	FIXED_PARTITION_DEVICE(IMAGE2_PARTITION)
#define IMAGE2_PARTITION_SIZE    FIXED_PARTITION_SIZE(IMAGE2_PARTITION)

#define FLASH_PAGE_SIZE   32
#define TEST_DATA_WORD_0  0x1122
#define TEST_DATA_WORD_1  0xaabb
#define TEST_DATA_WORD_2  0xabcd
#define TEST_DATA_WORD_3  0x1234
/* halfway through the partition */
#define FLASH_TEST_OFFSET2 0x1e0004
#define FLASH_TEST_PAGE_IDX 15

int main(void)
{

	const struct device *flash_dev = TEST_PARTITION_DEVICE;
        uint32_t buffer[8];
        uint32_t buffer_1[8];
	uint32_t i, j, offset, pages, count=0;

        printf("Available partitions on SoC Flash\n");
        printf("image1_partition        @%x, size %d\n", IMAGE1_PARTITION_OFFSET, IMAGE1_PARTITION_SIZE);
        printf("image2_partition        @%x, size %d\n", IMAGE2_PARTITION_OFFSET, IMAGE2_PARTITION_SIZE);
        printf("scratch_partition       @%x, size %d\n", TEST_PARTITION_OFFSET, TEST_PARTITION_SIZE);
	printf("TM32H7 Flash Testing using scratch_partition\n");
	printf("============================================\n");

	if (!device_is_ready(flash_dev)) {
		printf("Flash device not ready\n");
		return 0;
	}

	printf("\nTest 1: Flash erase page at 0x%x, size %d\n",
                TEST_PARTITION_OFFSET,
                TEST_PARTITION_SIZE);
	if (flash_erase(flash_dev, TEST_PARTITION_OFFSET, TEST_PARTITION_SIZE) != 0) {
		printf("   Flash erase failed!\n");
	} else {
		printf("   Flash erase succeeded!\n");
	}

	printf("\nTest 2: Flash write (word array 1)\n");
	offset = TEST_PARTITION_OFFSET;
        pages = TEST_PARTITION_SIZE / FLASH_PAGE_SIZE;
        /* STM32 flash supports 32byte program size. So page size is 32 */
        for (i = 0; i < FLASH_PAGE_SIZE / 4; i++) {
             if ((i % 4) == 0) {
                 buffer[i] = 0xaa0055ff;
             } else if ((i % 4) == 1) {
                 buffer[i] = 0x0055ff00;
             } else if ((i % 4) == 2) {
                 buffer[i] = 0x55ff00aa;
             } else if ((i % 4) == 3) {
                 buffer[i] = 0xff00aa55;
             }
        }

        for (i = 0; i < pages; i++) {
                if ((i % 100) == 0) {
                        printf("Writing page %d\n", i);
                }
                if (flash_write(flash_dev, offset + (i * FLASH_PAGE_SIZE), &buffer[0],
		                sizeof(buffer)) != 0) {
		        printf("   Flash write failed!\n");
		        return 0;
	        }
        }

        for (j = 0; j < pages; j++) {
                if ((j % 100) == 0) {
	                printf("   Attempted to read 0x%x\n", offset + (j * FLASH_PAGE_SIZE));
                }
	        if (flash_read(flash_dev, offset + (j * FLASH_PAGE_SIZE), &buffer_1[0],
	                        sizeof(buffer_1)) != 0) {
		                printf("   Flash read failed!\n");
		        return 0;
	        }
                for (i = 0; i < FLASH_PAGE_SIZE / 4; i++) {
                        if (buffer[i] != buffer_1[i]) {
			        count++;
		        }
                }
	}
        if (count) {
                printf("%d words read patterns wrong\n", count);
        } else {
                printf("%d pages read correctly\n", pages);
        }

#if defined(CONFIG_FLASH_PAGE_LAYOUT)
	struct flash_pages_info info;
	int rc;

	rc = flash_get_page_info_by_offs(flash_dev, FLASH_TEST_OFFSET2, &info);

	printf("\nTest 3: Page layout API\n");

	if (!rc) {
		printf("   Offset  0x%08x:\n", FLASH_TEST_OFFSET2);
		printf("     belongs to the page %u of start offset 0x%08lx\n",
		       info.index, (unsigned long) info.start_offset);
		printf("     and the size of 0x%08x B.\n", info.size);
	} else {
		printf("   Error: flash_get_page_info_by_offs returns %d\n",
		       rc);
	}

	rc = flash_get_page_info_by_idx(flash_dev, FLASH_TEST_PAGE_IDX, &info);

	if (!rc) {
		printf("   Page of number %u has start offset 0x%08lx\n",
		       FLASH_TEST_PAGE_IDX,
		       (unsigned long) info.start_offset);
		printf("     and size of 0x%08x B.\n", info.size);
		if (info.index == FLASH_TEST_PAGE_IDX) {
			printf("     Page index resolved properly\n");
		} else {
			printf("     ERROR: Page index resolved to %u\n",
			       info.index);
		}

	} else {
		printf("   Error: flash_get_page_info_by_idx returns %d\n", rc);
	}

	printf("   SoC flash consists of %u pages.\n",
	       flash_get_page_count(flash_dev));

#endif


	printf("\nTest 4: Write block size API\n");
	printf("   write-block-size = %u\n",
	       flash_get_write_block_size(flash_dev));
	return 0;

}
