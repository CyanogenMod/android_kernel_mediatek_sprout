/*
* Copyright (C) 2011-2014 MediaTek Inc.
*
* This program is free software: you can redistribute it and/or modify it under the terms of the
* GNU General Public License version 2 as published by the Free Software Foundation.
*
* This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
* without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
* See the GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License along with this program.
* If not, see <http://www.gnu.org/licenses/>.
*/
/*******************************************************************************
 *
 * Filename:
 * ---------
 *   AudDrv_Ana.c
 *
 * Project:
 * --------
 *   MT6583  Audio Driver ana Register setting
 *
 * Description:
 * ------------
 *   Audio register
 *
 * Author:
 * -------
 * Chipeng Chang
 *
 *------------------------------------------------------------------------------
 * $Revision: #1 $
 * $Modtime:$
 * $Log:$
 *
 *
 *******************************************************************************/


/*****************************************************************************
 *                     C O M P I L E R   F L A G S
 *****************************************************************************/


/*****************************************************************************
 *                E X T E R N A L   R E F E R E N C E S
 *****************************************************************************/

#include "AudDrv_Common.h"
#include "AudDrv_Ana.h"
#include "AudDrv_Clk.h"
#ifdef CONFIG_THUNDERSONIC_ENGINE_GPL
#include "../../thundersonic_engine/thundersonic_defs.h"
#endif

// define this to use wrapper to control
#define AUDIO_USING_WRAP_DRIVER
#ifdef AUDIO_USING_WRAP_DRIVER
#include <mach/mt_pmic_wrap.h>
#endif

/*****************************************************************************
 *                         D A T A   T Y P E S
 *****************************************************************************/

void Ana_Set_Reg(uint32 offset, uint32 value, uint32 mask)
{
    // set pmic register or analog CONTROL_IFACE_PATH
    int ret = 0;
#ifdef AUDIO_USING_WRAP_DRIVER
    uint32 Reg_Value = Ana_Get_Reg(offset);
	Reg_Value &= (~mask);
#ifdef CONFIG_THUNDERSONIC_ENGINE_GPL
	if(((offset == AUDTOP_CON5) && lockhp ) ||
		((offset == AUDTOP_CON7) && lockhs)) {
		return;
	}
	else {
#endif
		Reg_Value |= (value & mask);
#ifdef CONFIG_THUNDERSONIC_ENGINE_GPL
	}
#endif
    ret = pwrap_write(offset, Reg_Value);
    Reg_Value = Ana_Get_Reg(offset);
    if ((Reg_Value & mask) != (value & mask))
    {
    pr_err("Ana_Set_Reg offset= 0x%x , value = 0x%x mask = 0x%x ret = %d Reg_Value = 0x%x\n", offset, value, mask, ret, Reg_Value);
    }
#endif
}

uint32 Ana_Get_Reg(uint32 offset)
{
    // get pmic register
    int ret = 0;
    uint32 Rdata = 0;
#ifdef AUDIO_USING_WRAP_DRIVER
    ret = pwrap_read(offset, &Rdata);
#endif
    //PRINTK_ANA_REG ("Ana_Get_Reg offset= 0x%x  Rdata = 0x%x ret = %d\n",offset,Rdata,ret);
    return Rdata;
}

void Ana_Log_Print(void)
{
    AudDrv_ANA_Clk_On();
    pr_debug("ABB_AFE_CON0    = 0x%x\n", Ana_Get_Reg(ABB_AFE_CON0));
    pr_debug("ABB_AFE_CON1    = 0x%x\n", Ana_Get_Reg(ABB_AFE_CON1));
    pr_debug("ABB_AFE_CON2    = 0x%x\n", Ana_Get_Reg(ABB_AFE_CON2));
    pr_debug("ABB_AFE_CON3    = 0x%x\n", Ana_Get_Reg(ABB_AFE_CON3));
    pr_debug("ABB_AFE_CON4    = 0x%x\n", Ana_Get_Reg(ABB_AFE_CON4));
    pr_debug("ABB_AFE_CON5  = 0x%x\n", Ana_Get_Reg(ABB_AFE_CON5));
    pr_debug("ABB_AFE_CON6  = 0x%x\n", Ana_Get_Reg(ABB_AFE_CON6));
    pr_debug("ABB_AFE_CON7  = 0x%x\n", Ana_Get_Reg(ABB_AFE_CON7));
    pr_debug("ABB_AFE_CON8  = 0x%x\n", Ana_Get_Reg(ABB_AFE_CON8));
    pr_debug("ABB_AFE_CON9  = 0x%x\n", Ana_Get_Reg(ABB_AFE_CON9));
    pr_debug("ABB_AFE_CON10  = 0x%x\n", Ana_Get_Reg(ABB_AFE_CON10));
    pr_debug("ABB_AFE_CON11  = 0x%x\n", Ana_Get_Reg(ABB_AFE_CON11));

    pr_debug("ABB_AFE_STA0  = 0x%x\n", Ana_Get_Reg(ABB_AFE_STA0));
    pr_debug("ABB_AFE_STA1  = 0x%x\n", Ana_Get_Reg(ABB_AFE_STA1));
    pr_debug("ABB_AFE_STA2  = 0x%x\n", Ana_Get_Reg(ABB_AFE_STA2));

    pr_debug("ABB_AFE_UP8X_FIFO_CFG0    = 0x%x\n", Ana_Get_Reg(ABB_AFE_UP8X_FIFO_CFG0));
    pr_debug("ABB_AFE_UP8X_FIFO_LOG_MON0    = 0x%x\n", Ana_Get_Reg(ABB_AFE_UP8X_FIFO_LOG_MON0));
    pr_debug("ABB_AFE_UP8X_FIFO_LOG_MON1    = 0x%x\n", Ana_Get_Reg(ABB_AFE_UP8X_FIFO_LOG_MON1));
    pr_debug("ABB_AFE_PMIC_NEWIF_CFG0    = 0x%x\n", Ana_Get_Reg(ABB_AFE_PMIC_NEWIF_CFG0));
    pr_debug("ABB_AFE_PMIC_NEWIF_CFG1    = 0x%x\n", Ana_Get_Reg(ABB_AFE_PMIC_NEWIF_CFG1));
    pr_debug("ABB_AFE_PMIC_NEWIF_CFG2  = 0x%x\n", Ana_Get_Reg(ABB_AFE_PMIC_NEWIF_CFG2));
    pr_debug("ABB_AFE_PMIC_NEWIF_CFG3  = 0x%x\n", Ana_Get_Reg(ABB_AFE_PMIC_NEWIF_CFG3));
    pr_debug("ABB_AFE_TOP_CON0  = 0x%x\n", Ana_Get_Reg(ABB_AFE_TOP_CON0));
    pr_debug("ABB_AFE_MON_DEBUG0  = 0x%x\n", Ana_Get_Reg(ABB_AFE_MON_DEBUG0));


    pr_debug("SPK_CON0  = 0x%x\n", Ana_Get_Reg(SPK_CON0));
    pr_debug("SPK_CON1  = 0x%x\n", Ana_Get_Reg(SPK_CON1));
    pr_debug("SPK_CON2  = 0x%x\n", Ana_Get_Reg(SPK_CON2));
    pr_debug("SPK_CON6  = 0x%x\n", Ana_Get_Reg(SPK_CON6));
    pr_debug("SPK_CON7  = 0x%x\n", Ana_Get_Reg(SPK_CON7));
    pr_debug("SPK_CON8  = 0x%x\n", Ana_Get_Reg(SPK_CON8));
    pr_debug("SPK_CON9  = 0x%x\n", Ana_Get_Reg(SPK_CON9));
    pr_debug("SPK_CON10  = 0x%x\n", Ana_Get_Reg(SPK_CON10));
    pr_debug("SPK_CON11  = 0x%x\n", Ana_Get_Reg(SPK_CON11));
    pr_debug("SPK_CON12  = 0x%x\n", Ana_Get_Reg(SPK_CON12));

    pr_debug("CID    = 0x%x\n", Ana_Get_Reg(CID));
    pr_debug("TOP_CKPDN0  = 0x%x\n", Ana_Get_Reg(TOP_CKPDN0));
    pr_debug("TOP_CKPDN0_SET  = 0x%x\n", Ana_Get_Reg(TOP_CKPDN0_SET));
    pr_debug("TOP_CKPDN0_CLR  = 0x%x\n", Ana_Get_Reg(TOP_CKPDN0_CLR));
    pr_debug("TOP_CKPDN1  = 0x%x\n", Ana_Get_Reg(TOP_CKPDN1));
    pr_debug("TOP_CKPDN1_SET  = 0x%x\n", Ana_Get_Reg(TOP_CKPDN1_SET));
    pr_debug("TOP_CKPDN1_CLR  = 0x%x\n", Ana_Get_Reg(TOP_CKPDN1_CLR));
    pr_debug("TOP_CKPDN2  = 0x%x\n", Ana_Get_Reg(TOP_CKPDN2));
    pr_debug("TOP_CKPDN2_SET  = 0x%x\n", Ana_Get_Reg(TOP_CKPDN2_SET));
    pr_debug("TOP_CKPDN2_CLR  = 0x%x\n", Ana_Get_Reg(TOP_CKPDN2_CLR));
    pr_debug("TOP_CKCON1  = 0x%x\n", Ana_Get_Reg(TOP_CKCON1));

    pr_debug("AUDTOP_CON0  = 0x%x\n", Ana_Get_Reg(AUDTOP_CON0));
    pr_debug("AUDTOP_CON1  = 0x%x\n", Ana_Get_Reg(AUDTOP_CON1));
    pr_debug("AUDTOP_CON2  = 0x%x\n", Ana_Get_Reg(AUDTOP_CON2));
    pr_debug("AUDTOP_CON3  = 0x%x\n", Ana_Get_Reg(AUDTOP_CON3));
    pr_debug("AUDTOP_CON4  = 0x%x\n", Ana_Get_Reg(AUDTOP_CON4));
    pr_debug("AUDTOP_CON5  = 0x%x\n", Ana_Get_Reg(AUDTOP_CON5));
    pr_debug("AUDTOP_CON6  = 0x%x\n", Ana_Get_Reg(AUDTOP_CON6));
    pr_debug("AUDTOP_CON7  = 0x%x\n", Ana_Get_Reg(AUDTOP_CON7));
    pr_debug("AUDTOP_CON8  = 0x%x\n", Ana_Get_Reg(AUDTOP_CON8));
    pr_debug("AUDTOP_CON9  = 0x%x\n", Ana_Get_Reg(AUDTOP_CON9));
    AudDrv_ANA_Clk_Off();
    pr_debug("-Ana_Log_Print \n");
}


// export symbols for other module using
EXPORT_SYMBOL(Ana_Log_Print);
EXPORT_SYMBOL(Ana_Set_Reg);
EXPORT_SYMBOL(Ana_Get_Reg);


