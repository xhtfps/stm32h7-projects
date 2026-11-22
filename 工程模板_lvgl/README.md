# STM32H7 通用工程模板

本模板由以下三份工程资料归并而成：

- `5.LTDC液晶驱动-基于AT070TN83`：提供 H743 启动流程、480 MHz 时钟、MPU/D-Cache、FMC/SDRAM、LTDC/DMA2D 和 LCD/BSP 驱动。
- `LCD`：提供独立 LCD 驱动代码，保留在原工作区中，作为后续显示模块移植参考。
- `软件/谐波测量（结项）`：验证了 SPI、UART、定时器、DMA、AD7606、FFT 和 UI 等业务扩展方式；业务代码没有并入通用模板。

## 模板结构

```text
H7通用工程模板/
├─ App/
│  ├─ Inc/app.h              # 应用层公共接口
│  └─ Src/app.c              # 用户业务入口
├─ Core/
│  ├─ Inc/                   # main、HAL 配置、中断声明
│  └─ Src/                   # 启动后的系统初始化与中断实现
├─ Drivers/
│  ├─ CMSIS/
│  ├─ STM32H7xx_HAL_Driver/
│  └─ User/                  # LED、串口、SDRAM、LTDC/LCD 等可选 BSP
└─ MDK-ARM/
   ├─ H7_Generic_Template.uvprojx
   └─ startup_stm32h743xx.s
```

## 默认初始化链

```text
复位 → SystemInit → MPU → I-Cache/D-Cache → HAL_Init
     → 480 MHz 系统时钟 → App_Init → App_Loop
```

默认假定目标为 `STM32H743IITx`，外部晶振为 25 MHz，系统主频为 480 MHz，外部 SDRAM 地址为 `0xC0000000`。若硬件没有 SDRAM 或 LCD，可在 `Core/Inc/main.h` 中关闭对应宏，并从 `main.c` 的外设时钟选择中移除不需要的时钟。

## Keil 使用

1. 使用 Keil MDK 打开 `MDK-ARM/H7_Generic_Template.uvprojx`。
2. 根据实际板卡修改 `Core/Src/main.c` 的时钟、SDRAM 和外设时钟配置。
3. 在 `App/Src/app.c` 中调用需要的 BSP/驱动初始化函数。
4. 新增业务模块时，放入 `App/`；硬件封装放入 `Drivers/User/`，避免在底层驱动中直接处理业务或 UI。

模板已经加入 GPIO、DMA、MDMA、DMA2D、FMC/SDRAM、LTDC、SPI、I2C、TIM、UART、Cortex、PWR、RCC 等 HAL 源文件。未使用的外设仍建议在 `stm32h7xx_hal_conf.h` 和 Keil 工程中按项目裁剪。

## 移植注意事项

- `Drivers/User/lcd_*.c` 来源于 AT070TN83 示例，属于可选显示 BSP；更换屏幕时应替换时序、分辨率、像素格式和 GPIO 配置。
- H7 的 D-Cache 与 DMA/SDRAM 一起使用时，必须处理 Cache 一致性；DMA 缓冲区应按实际场景放入合适的 SRAM 区域并执行 Clean/Invalidate。
- LTDC 双层模式下，layer0/layer1 各占用一帧 800x480 RGB565 显存；LVGL 绘图缓冲和内存池已放到两层显存之后，避免覆盖前景层。
- `AD7606.c`、`FFT.c`、`User_UI.c`、`BSP_4x4KEY.c` 等谐波测量业务模块未复制进模板，请按应用需要放入 `App/` 或独立业务模块目录。
- 当前环境没有 Keil 编译器，模板已完成文件、路径和工程分组整理，但最终下载前仍需在本机 Keil MDK 中执行一次 Rebuild。

## 4x4 矩阵键盘与 LVGL

已加入 `Drivers/User/` 下的 `matrix_keypad.c/.h`，并通过 LVGL keypad indev 接入默认 group。LVGL 页面会显示最近一次按键的原始行列、逻辑行列和映射后的 LVGL key，不再绕过 LVGL 直接绘制校准界面。当前引脚解释为：

```text
行：PB13、PB15、PD13、PE2
列：PB12、PB14、PB2、PD11
```

程序启动后屏幕显示按键诊断信息。`2/4/6/8` 导航，`A` 确认，`B` 返回/退格，`C/D/*/#` 映射为 ESC/DEL/HOME/END，其余数字以普通 LVGL key 发送。如果上下或左右方向仍相反，修改 `Drivers/User/Inc/matrix_keypad.h` 中的 `KEYPAD_ROTATE_180`、`KEYPAD_ROW_REVERSE` 或 `KEYPAD_COL_REVERSE`；如果前后四组实际是列/行相反，修改 `KEYPAD_FIRST_GROUP_IS_ROWS`。
