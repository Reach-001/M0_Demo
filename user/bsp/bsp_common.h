/**
 * @file    bsp_common.h
 * @brief   BSP 公共定义 — 错误码、通用宏
 * @author  Reach
 * @version v1.0
 *
 * 本文件提供所有 BSP 模块共用的类型定义和宏。
 * 各 bsp_xxx.h 如需返回错误码，#include "bsp_common.h" 即可使用。
 *
 * 使用示例：
 * @code
 *   bsp_status_t ret = some_bsp_function(param);
 *   if (ret != BSP_OK) {
 *       printf("BSP error: %d\r\n", ret);
 *   }
 * @endcode
 */

#ifndef _BSP_COMMON_H_
#define _BSP_COMMON_H_

#include "board_config.h"

/**
 * @brief   BSP 统一错误码枚举
 * @note    新增模块若需要特定错误码，在此追加即可
 */
typedef enum
{
    BSP_OK          = 0,    ///< 操作成功
    BSP_ERR_PARAM   = -1,   ///< 参数非法（空指针、越界等）
    BSP_ERR_BUSY    = -2,   ///< 设备忙（上次操作未完成）
    BSP_ERR_TIMEOUT = -3,   ///< 操作超时
    BSP_ERR_HW      = -4,   ///< 硬件错误（通信失败、外设无响应）
    BSP_ERR_STATE   = -5,   ///< 状态错误（未初始化就调用）
} bsp_status_t;

/**
 * @brief   参数校验宏 — 若条件为假则返回错误码
 * @param   expr    断言条件
 * @param   ret     失败时返回的错误码
 *
 * 使用示例：
 * @code
 *   bsp_status_t bsp_xxx_do(uint8 id) {
 *       BSP_ASSERT_RETURN(id < XXX_NUM, BSP_ERR_PARAM);
 *       // ... 正常逻辑
 *       return BSP_OK;
 *   }
 * @endcode
 */
#define BSP_ASSERT_RETURN(expr, ret)   do { if (!(expr)) return (ret); } while(0)

#endif /* _BSP_COMMON_H_ */
