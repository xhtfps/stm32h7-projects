# STM32H7 工程集（STM32H743）

STM32H743 的通用工程模板、LTDC 液晶驱动与谐波测量项目。默认目标为 `STM32H743IITx`、480 MHz、外部 25 MHz 晶振、SDRAM 映射在 `0xC0000000`。

## 1. 目录一览

| 目录 | 内容 |
| --- | --- |
| `工程模板_裸机/` | 通用裸机模板：`App/` 业务层 + `Core/` 初始化 + `Drivers/`（CMSIS、HAL、User BSP），Keil 入口 `MDK-ARM/H7_Generic_Template.uvprojx` |
| `工程模板_freertos/` | 在裸机模板基础上接入 FreeRTOS |
| `工程模板_lvgl/` | 在裸机模板基础上接入 LVGL + LTDC 显示 |
| `5.LTDC液晶驱动-基于AT070TN83/` | 7 寸 AT070TN83 屏的 LTDC/DMA2D + 触摸驱动，改屏时参照此目录 |
| `谐波测量（结项）/` | AD7606 八通道采样 + FFT 谐波分析 + 4x4 键盘/UI（已结项） |

## 2. 软硬件环境

| 项 | 要求 |
| --- | --- |
| 开发板 | STM32H743 系列（工程默认 H743IIT6），板载 SDRAM + LTDC 接口 |
| 屏幕 | AT070TN83（800×480，RGB888 并口） |
| IDE | Keil MDK-ARM 5.36+，器件包 `Keil.STM32H7xx_DFP` |
| 调试器 | J-Link / ST-Link（SWD） |
| 串口 | USART1，115200 |

## 3. 快速开始

1. Keil 打开 `工程模板_裸机/MDK-ARM/H7_Generic_Template.uvprojx`。
2. `Rebuild all target files`，首次全编译。
3. 按实际板卡核对 `Core/Src/main.c` 中的时钟树、SDRAM 参数与外设时钟。
4. `Flash → Download` 烧录，复位后应看到 LED 心跳与串口启动打印。

默认初始化链：

```text
复位 → SystemInit → MPU → I-Cache/D-Cache → HAL_Init
     → 480 MHz 时钟 → App_Init → App_Loop
```

新增业务：业务代码放 `App/`，硬件封装放 `Drivers/User/`，不要直接在 HAL 驱动里写业务。

## 4. 关键注意点（H7 特有）

- **Cache 一致性**：D-Cache 打开后，DMA/SDRAM 缓冲区必须放在合适 SRAM 区域并做 Clean/Invalidate，否则会出现“数据偶尔错”。
- **SDRAM**：没有外扩 SDRAM 时，在 `Core/Inc/main.h` 关闭对应宏，并从 FMC 初始化中移除。
- **LTDC 换屏**：分辨率、时序、像素格式、GPIO 都要改；`Drivers/User/lcd_*.c` 是 AT070TN83 的参数，换屏需整体替换。
- **4x4 矩阵键盘**：行 `PB13/PB15/PD13/PE2`，列 `PB12/PB14/PB2/PD11`；方向反了改 `matrix_keypad.h` 里 `KEYPAD_ROW_REVERSE` / `KEYPAD_COL_REVERSE` / `KEYPAD_FIRST_GROUP_IS_ROWS`。

## 5. 常见问题

- **下载后无显示**：先确认 LTDC 时钟（`LTDC` 像素时钟）与 SDRAM 是否初始化成功，可用串口打印 `HAL_LTDC_Init` 返回值定位。
- **HardFault**：多为 MPU 配置或 Cache 关闭顺序问题，先关 D-Cache 验证。
- **LVGL 花屏**：显存对齐与 stride 不匹配，检查 `lv_conf.h` 的颜色格式和缓冲区大小。

## 6. 维护说明

- 排除清单见 [`EXCLUDED_MANIFEST.csv`](EXCLUDED_MANIFEST.csv)。
- `谐波测量（结项）` 的业务代码（AD7606、FFT、UI）**没有**并入通用模板，需要时从该目录整块取用。
- 第三方库与屏幕手册版权归原作者，仅作学习参考。
