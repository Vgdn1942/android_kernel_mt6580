This is 3.10.x MT6580 kernel source ported to be used on DEXP Ixion P350.

## Known information
| Subsystem | Driver name | Availability | Working |
|-----------|-------------|--------------|---------|
| LCM driver | `nt35521_hd720_dsi_vdo_rixin` | Yes | Yes |
| Touch panel | `GT9XX (i2c 1-005D)` | Yes | Yes |
| GPU | `Mali-400 MP` | Yes | Yes |
| Camera #1 | `imx219_mipi_raw` | No | No |
| Camera #2 | `gc2755_mipi_raw` | Yes | No |
| Accelerometer | `MC3XXX (i2c 2-004c)` | No | ? |
| ALS/PS | `stk3x1x (i2c 2-0048)` | No | ? |
| Charger | `BQ24158 (i2c 1-006A)` | No | ? |
| Flash | `Samsung F722MB` | Yes | Yes |
| Lens #1 | `DW9714AF (i2c 0-0018)` | No | ? |
| Lens #2 | `BU6424AF (i2c 0-0019)` | No | ? |
| RAM | `1 GB LPDDR3_1066` | - | - |
| Sound | `mtsndcard` | - | - |
| Other | `kd_camera_hw (i2c 0-007f)` | - | - |
