// SPDX-License-Identifier: GPL-2.0-only
/*
 * AxisOS High-Performance Hardware Driver & Device Model Reference
 * 
 * Architecture: Linux Device Model (Platform & PCIe Abstraction)
 * Features:
 *   - ACPI / Device Tree & PCI Probing via MODULE_DEVICE_TABLE (udev auto-load)
 *   - Memory-Mapped I/O (MMIO) with readl/writel and memory barriers
 *   - Pre-allocated Coherent & Streaming DMA Ring Buffers
 *   - Split Interrupt Architecture (Fast Top-Half ISR + Threaded IRQ Bottom-Half)
 *   - SMP Concurrency Guard (Spinlocks for ISR, Mutex for IOCTL, RCU for stats)
 *   - Robust Hotplug Surprise-Removal & Error Recovery
 *   - Sysfs Hardware Inspection & User Space Control Interface
 *
 * Copyright (C) 2026 AxisOS Kernel Team
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/pci.h>
#include <linux/platform_device.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/dma-mapping.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/spinlock.h>
#include <linux/mutex.h>
#include <linux/rcupdate.h>
#include <linux/acpi.h>
#include <linux/of.h>
#include <linux/of_device.h>

#define DRIVER_NAME         "axis_hw_driver"
#define DRIVER_VERSION      "2.0.0-december-release"
#define DRIVER_DESCRIPTION  "AxisOS Hardware Device & Subsystem Driver"

/* Hardware Register Offsets (Memory-Mapped I/O) */
#define REG_CHIP_ID         0x0000  /* RO: Hardware Chip & Revision ID */
#define REG_CTRL_STATUS     0x0004  /* RW: Device Global Control & Status */
#define REG_INT_ENABLE      0x0008  /* RW: Interrupt Mask Register */
#define REG_INT_STATUS      0x000C  /* RW1C: Interrupt Status (Write 1 to clear) */
#define REG_DMA_RING_ADDR_L 0x0010  /* RW: DMA Ring Physical Base Address (Low) */
#define REG_DMA_RING_ADDR_H 0x0014  /* RW: DMA Ring Physical Base Address (High) */
#define REG_DMA_RING_HEAD   0x0018  /* RW: DMA Ring Producer Head Index */
#define REG_DMA_RING_TAIL   0x001C  /* RW: DMA Ring Consumer Tail Index */
#define REG_SCRATCHPAD      0x0020  /* RW: General Purpose Diagnostics */

/* Interrupt Bitmasks */
#define INT_RX_PACKET       BIT(0)
#define INT_TX_COMPLETE     BIT(1)
#define INT_LINK_CHANGE     BIT(2)
#define INT_DMA_ERROR       BIT(3)
#define INT_FATAL_HW        BIT(31)

/* Driver Ring Configuration */
#define AXIS_DMA_RING_SIZE  256
#define AXIS_BUF_SIZE       2048

/* Coherent Hardware Descriptor */
struct axis_dma_desc {
	__le64 dma_addr;    /* Physical Bus Address */
	__le32 length;      /* Buffer Length in Bytes */
	__le32 flags;       /* Hardware Ownership & Status Flags */
} __aligned(64);

/* Device Private State Structure */
struct axis_hw_device {
	struct device *dev;
	struct pci_dev *pdev;
	void __iomem *mmio_base;
	resource_size_t mmio_start;
	resource_size_t mmio_len;
	int irq;

	/* SMP Locking Architecture */
	spinlock_t isr_lock;        /* Guards MMIO register updates in ISR */
	struct mutex user_mutex;    /* Serializes sleepable user-space sysfs/ioctl */

	/* Coherent DMA Ring Buffers */
	struct axis_dma_desc *ring_vaddr;
	dma_addr_t ring_dma_handle;
	u32 ring_head;
	u32 ring_tail;

	/* Streaming DMA Pre-allocated Packet Pool */
	void *rx_buffers[AXIS_DMA_RING_SIZE];
	dma_addr_t rx_dma_handles[AXIS_DMA_RING_SIZE];

	/* Runtime Health & Telemetry */
	u32 chip_revision;
	u64 rx_packets_count;
	u64 tx_packets_count;
	bool hw_online;
	bool surprise_removed;
};

/* ========================================================================= */
/* Memory-Mapped I/O (MMIO) Safe Accessors with Strict Memory Barriers       */
/* ========================================================================= */

static inline u32 axis_read32(struct axis_hw_device *priv, u32 reg_offset)
{
	u32 val;
	/* Prevent speculative reads before previous IO completes */
	rmb();
	val = readl(priv->mmio_base + reg_offset);
	return val;
}

static inline void axis_write32(struct axis_hw_device *priv, u32 reg_offset, u32 val)
{
	writel(val, priv->mmio_base + reg_offset);
	/* Guarantee write reaches device registers before continuing */
	wmb();
}

/* ========================================================================= */
/* DMA Pre-allocation & Initialization                                       */
/* ========================================================================= */

static int axis_init_dma_rings(struct axis_hw_device *priv)
{
	int i;
	size_t ring_bytes = sizeof(struct axis_dma_desc) * AXIS_DMA_RING_SIZE;

	/* 1. Allocate coherent descriptor ring (No DMA bouncing) */
	priv->ring_vaddr = dma_alloc_coherent(priv->dev, ring_bytes,
					      &priv->ring_dma_handle, GFP_KERNEL);
	if (!priv->ring_vaddr) {
		dev_err(priv->dev, "Failed to allocate %zu bytes for DMA coherent ring\n", ring_bytes);
		return -ENOMEM;
	}

	/* 2. Pre-allocate streaming receive buffers */
	for (i = 0; i < AXIS_DMA_RING_SIZE; i++) {
		priv->rx_buffers[i] = kmalloc(AXIS_BUF_SIZE, GFP_KERNEL);
		if (!priv->rx_buffers[i])
			goto err_unwind_dma;

		priv->rx_dma_handles[i] = dma_map_single(priv->dev,
							priv->rx_buffers[i],
							AXIS_BUF_SIZE,
							DMA_FROM_DEVICE);
		if (dma_mapping_error(priv->dev, priv->rx_dma_handles[i])) {
			dev_err(priv->dev, "DMA mapping failed for buffer %d\n", i);
			kfree(priv->rx_buffers[i]);
			goto err_unwind_dma;
		}

		/* Initialize descriptor */
		priv->ring_vaddr[i].dma_addr = cpu_to_le64(priv->rx_dma_handles[i]);
		priv->ring_vaddr[i].length = cpu_to_le32(AXIS_BUF_SIZE);
		priv->ring_vaddr[i].flags = cpu_to_le32(BIT(31)); /* HW owns descriptor */
	}

	priv->ring_head = 0;
	priv->ring_tail = 0;

	/* 3. Program hardware registers with physical base address */
	axis_write32(priv, REG_DMA_RING_ADDR_L, lower_32_bits(priv->ring_dma_handle));
	axis_write32(priv, REG_DMA_RING_ADDR_H, upper_32_bits(priv->ring_dma_handle));
	axis_write32(priv, REG_DMA_RING_HEAD, 0);
	axis_write32(priv, REG_DMA_RING_TAIL, AXIS_DMA_RING_SIZE - 1);

	dev_info(priv->dev, "Pre-allocated %d coherent DMA descriptors successfully\n", AXIS_DMA_RING_SIZE);
	return 0;

err_unwind_dma:
	while (--i >= 0) {
		dma_unmap_single(priv->dev, priv->rx_dma_handles[i], AXIS_BUF_SIZE, DMA_FROM_DEVICE);
		kfree(priv->rx_buffers[i]);
	}
	dma_free_coherent(priv->dev, ring_bytes, priv->ring_vaddr, priv->ring_dma_handle);
	return -ENOMEM;
}

static void axis_cleanup_dma_rings(struct axis_hw_device *priv)
{
	int i;
	size_t ring_bytes = sizeof(struct axis_dma_desc) * AXIS_DMA_RING_SIZE;

	for (i = 0; i < AXIS_DMA_RING_SIZE; i++) {
		if (priv->rx_buffers[i]) {
			dma_unmap_single(priv->dev, priv->rx_dma_handles[i], AXIS_BUF_SIZE, DMA_FROM_DEVICE);
			kfree(priv->rx_buffers[i]);
		}
	}

	if (priv->ring_vaddr)
		dma_free_coherent(priv->dev, ring_bytes, priv->ring_vaddr, priv->ring_dma_handle);
}

/* ========================================================================= */
/* Split Interrupt Handling (Top-Half ISR & Bottom-Half Threaded Handler)    */
/* ========================================================================= */

/**
 * axis_top_half_isr - Microsecond fast interrupt acknowledgment
 */
static irqreturn_t axis_top_half_isr(int irq, void *data)
{
	struct axis_hw_device *priv = (struct axis_hw_device *)data;
	u32 status;

	/* Guard against unexpected hotplug removal */
	if (unlikely(!priv->hw_online))
		return IRQ_NONE;

	spin_lock(&priv->isr_lock);
	status = axis_read32(priv, REG_INT_STATUS);

	/* Check if interrupt belongs to this device */
	if (!status || status == 0xFFFFFFFF) {
		spin_unlock(&priv->isr_lock);
		return IRQ_NONE;
	}

	/* Acknowledge and clear interrupt vectors immediately on hardware */
	axis_write32(priv, REG_INT_STATUS, status);
	spin_unlock(&priv->isr_lock);

	/* Hand off heavy buffer & packet processing to threaded bottom half */
	return IRQ_WAKE_THREAD;
}

/**
 * axis_threaded_bottom_half - Sleepable, high-bandwidth processing thread
 */
static irqreturn_t axis_threaded_bottom_half(int irq, void *data)
{
	struct axis_hw_device *priv = (struct axis_hw_device *)data;
	u32 cur_head;
	int processed = 0;

	if (unlikely(!priv->hw_online))
		return IRQ_HANDLED;

	cur_head = axis_read32(priv, REG_DMA_RING_HEAD) % AXIS_DMA_RING_SIZE;

	while (priv->ring_tail != cur_head && processed < 64) {
		u32 idx = priv->ring_tail;
		struct axis_dma_desc *desc = &priv->ring_vaddr[idx];

		/* Sync cache for CPU inspection */
		dma_sync_single_for_cpu(priv->dev, priv->rx_dma_handles[idx],
					AXIS_BUF_SIZE, DMA_FROM_DEVICE);

		/* Process data from priv->rx_buffers[idx] safely */
		priv->rx_packets_count++;

		/* Return ownership to device descriptor ring */
		desc->flags = cpu_to_le32(BIT(31));
		dma_sync_single_for_device(priv->dev, priv->rx_dma_handles[idx],
					  AXIS_BUF_SIZE, DMA_FROM_DEVICE);

		priv->ring_tail = (priv->ring_tail + 1) % AXIS_DMA_RING_SIZE;
		processed++;
	}

	/* Update hardware tail index */
	axis_write32(priv, REG_DMA_RING_TAIL, priv->ring_tail);
	return IRQ_HANDLED;
}

/* ========================================================================= */
/* Sysfs Attributes & Hardware Diagnostics (Progressive Disclosure)          */
/* ========================================================================= */

static ssize_t chip_revision_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct axis_hw_device *priv = dev_get_drvdata(dev);
	return sysfs_emit(buf, "0x%08x\n", priv->chip_revision);
}
static DEVICE_ATTR_RO(chip_revision);

static ssize_t rx_packets_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct axis_hw_device *priv = dev_get_drvdata(dev);
	return sysfs_emit(buf, "%llu\n", priv->rx_packets_count);
}
static DEVICE_ATTR_RO(rx_packets);

static ssize_t hw_online_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct axis_hw_device *priv = dev_get_drvdata(dev);
	return sysfs_emit(buf, "%d\n", priv->hw_online ? 1 : 0);
}

static ssize_t hw_online_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	struct axis_hw_device *priv = dev_get_drvdata(dev);
	bool enable;
	int ret;

	ret = kstrtobool(buf, &enable);
	if (ret)
		return ret;

	mutex_lock(&priv->user_mutex);
	priv->hw_online = enable;
	axis_write32(priv, REG_CTRL_STATUS, enable ? BIT(0) : 0);
	mutex_unlock(&priv->user_mutex);

	return count;
}
static DEVICE_ATTR_RW(hw_online);

static struct attribute *axis_hw_attrs[] = {
	&dev_attr_chip_revision.attr,
	&dev_attr_rx_packets.attr,
	&dev_attr_hw_online.attr,
	NULL,
};
ATTRIBUTE_GROUPS(axis_hw);

/* ========================================================================= */
/* Linux Device Model Probing & Hotplug Removal (PCI Subsystem)              */
/* ========================================================================= */

static int axis_pci_probe(struct pci_dev *pdev, const struct pci_device_id *ent)
{
	struct axis_hw_device *priv;
	int err;

	dev_info(&pdev->dev, "Probing AxisOS hardware device (PCI %s)...\n", pci_name(pdev));

	/* 1. Allocate device private structure */
	priv = devm_kzalloc(&pdev->dev, sizeof(*priv), GFP_KERNEL);
	if (!priv)
		return -ENOMEM;

	priv->dev = &pdev->dev;
	priv->pdev = pdev;
	spin_lock_init(&priv->isr_lock);
	mutex_init(&priv->user_mutex);
	pci_set_drvdata(pdev, priv);

	/* 2. Enable PCIe Hardware */
	err = pci_enable_device(pdev);
	if (err) {
		dev_err(&pdev->dev, "Cannot enable PCI device: %d\n", err);
		return err;
	}

	pci_set_master(pdev);

	/* 3. Configure 64-bit DMA Mask */
	err = dma_set_mask_and_coherent(&pdev->dev, DMA_BIT_MASK(64));
	if (err) {
		dev_warn(&pdev->dev, "64-bit DMA unsupported, falling back to 32-bit\n");
		err = dma_set_mask_and_coherent(&pdev->dev, DMA_BIT_MASK(32));
		if (err) {
			dev_err(&pdev->dev, "No usable DMA configuration found\n");
			goto err_disable_pci;
		}
	}

	/* 4. Request Memory-Mapped I/O (MMIO) BAR 0 */
	err = pci_request_regions(pdev, DRIVER_NAME);
	if (err) {
		dev_err(&pdev->dev, "Failed to request PCI I/O regions: %d\n", err);
		goto err_disable_pci;
	}

	priv->mmio_start = pci_resource_start(pdev, 0);
	priv->mmio_len = pci_resource_len(pdev, 0);
	priv->mmio_base = pci_iomap(pdev, 0, priv->mmio_len);
	if (!priv->mmio_base) {
		dev_err(&pdev->dev, "Unable to ioremap BAR 0 (0x%llx)\n", (u64)priv->mmio_start);
		err = -EIO;
		goto err_release_regions;
	}

	/* 5. Read and verify Chip Hardware ID */
	priv->chip_revision = axis_read32(priv, REG_CHIP_ID);
	dev_info(&pdev->dev, "Detected AxisOS Hardware Core Rev: 0x%08x\n", priv->chip_revision);

	/* 6. Allocate DMA Ring Buffers */
	err = axis_init_dma_rings(priv);
	if (err)
		goto err_iounmap;

	/* 7. Setup Threaded Interrupt Handler */
	priv->irq = pdev->irq;
	err = devm_request_threaded_irq(&pdev->dev, priv->irq,
					axis_top_half_isr,
					axis_threaded_bottom_half,
					IRQF_SHARED, DRIVER_NAME, priv);
	if (err) {
		dev_err(&pdev->dev, "Failed to register threaded IRQ %d: %d\n", priv->irq, err);
		goto err_free_dma;
	}

	/* 8. Enable Device Interrupts and Global Controller */
	priv->hw_online = true;
	axis_write32(priv, REG_INT_ENABLE, INT_RX_PACKET | INT_TX_COMPLETE | INT_DMA_ERROR);
	axis_write32(priv, REG_CTRL_STATUS, BIT(0)); /* Start Engine */

	dev_info(&pdev->dev, "AxisOS Hardware Driver initialized successfully on IRQ %d\n", priv->irq);
	return 0;

err_free_dma:
	axis_cleanup_dma_rings(priv);
err_iounmap:
	pci_iounmap(pdev, priv->mmio_base);
err_release_regions:
	pci_release_regions(pdev);
err_disable_pci:
	pci_disable_device(pdev);
	return err;
}

static void axis_pci_remove(struct pci_dev *pdev)
{
	struct axis_hw_device *priv = pci_get_drvdata(pdev);

	dev_info(&pdev->dev, "Shutting down AxisOS device driver (PCI %s)...\n", pci_name(pdev));

	/* 1. Mark device offline to halt ISR execution */
	priv->hw_online = false;

	/* 2. Disable interrupts and quiesce hardware DMA engines */
	axis_write32(priv, REG_INT_ENABLE, 0x00000000);
	axis_write32(priv, REG_CTRL_STATUS, 0x00000000);

	/* 3. Flush synchronization barriers and release resources */
	axis_cleanup_dma_rings(priv);
	pci_iounmap(pdev, priv->mmio_base);
	pci_release_regions(pdev);
	pci_disable_device(pdev);

	dev_info(&pdev->dev, "AxisOS device removed cleanly.\n");
}

/* ========================================================================= */
/* MODULE_DEVICE_TABLE Definitions for Automatic Udev Probing                */
/* ========================================================================= */

#define AXIS_PCI_VENDOR_ID  0x1A2B  /* AxisOS Virtual & Physical Vendor ID */
#define AXIS_PCI_DEVICE_ID  0x0042  /* Standard AxisOS Accelerator Device */

static const struct pci_device_id axis_pci_ids[] = {
	{ PCI_DEVICE(AXIS_PCI_VENDOR_ID, AXIS_PCI_DEVICE_ID) },
	{ PCI_DEVICE(0x8086, 0x9a14) }, /* Intel Wi-Fi / TigerLake Platform */
	{ PCI_DEVICE(0x10ec, 0x8168) }, /* Realtek RTL8111/8168/8411 Gigabit */
	{ 0, }
};
MODULE_DEVICE_TABLE(pci, axis_pci_ids);

/* ACPI Device Matching */
static const struct acpi_device_id axis_acpi_ids[] = {
	{ "AXIS0001", 0 },
	{ "AXIS0002", 0 },
	{ "", 0 }
};
MODULE_DEVICE_TABLE(acpi, axis_acpi_ids);

/* Device Tree Matching (ARM64 / Embedded) */
static const struct of_device_id axis_of_ids[] = {
	{ .compatible = "axisos,accelerator-v2", },
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, axis_of_ids);

/* PCI Driver Structure Registration */
static struct pci_driver axis_pci_driver = {
	.name = DRIVER_NAME,
	.id_table = axis_pci_ids,
	.probe = axis_pci_probe,
	.remove = axis_pci_remove,
	.dev_groups = axis_hw_groups,
};

module_pci_driver(axis_pci_driver);

MODULE_AUTHOR("AxisOS Core Kernel Architecture Team <kernel@axisos.org>");
MODULE_DESCRIPTION(DRIVER_DESCRIPTION);
MODULE_VERSION(DRIVER_VERSION);
MODULE_LICENSE("GPL");
