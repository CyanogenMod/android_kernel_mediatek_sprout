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
/*****************************************************************************
 *
 * Filename:
 * ---------
 *   ov5648mipi_Sensor.c
 *
 * Project:
 * --------
 *   ALPS
 *
 * Description:
 * ------------
 *   Source code of Sensor driver
 *
 *
 *------------------------------------------------------------------------------
 * Upper this line, this part is controlled by CC/CQ. DO NOT MODIFY!!
 *============================================================================
 ****************************************************************************/

#include <linux/videodev2.h>
#include <linux/i2c.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/fs.h>
#include <asm/atomic.h>
#include <asm/system.h>
#include <linux/slab.h>     //kmalloc/kfree in kernel 3.10

#include "kd_camera_hw.h"
#include "kd_imgsensor.h"
#include "kd_imgsensor_define.h"
#include "kd_imgsensor_errcode.h"

#include "ov5648mipi_Sensor.h"
#include "ov5648mipi_Camera_Sensor_para.h"
#include "ov5648mipi_CameraCustomized.h"
#include <linux/xlog.h>

//#define OV5648MIPI_DRIVER_TRACE
#define OV5648MIPI_DEBUG
#ifdef OV5648MIPI_DEBUG
#define PFX "OV5648MIPI"
#define SENSORDB(fmt, arg...)    xlog_printk(ANDROID_LOG_DEBUG   , PFX, "[%s] " fmt, __FUNCTION__, ##arg)
#else
#define SENSORDB(x,...)
#endif

typedef enum
{
    OV5648MIPI_SENSOR_MODE_INIT,
    OV5648MIPI_SENSOR_MODE_PREVIEW,
    OV5648MIPI_SENSOR_MODE_CAPTURE,
    OV5648MIPI_SENSOR_MODE_VIDEO
} OV5648MIPI_SENSOR_MODE;

/* SENSOR PRIVATE STRUCT */
typedef struct OV5648MIPI_sensor_STRUCT
{
    MSDK_SENSOR_CONFIG_STRUCT cfg_data;
    sensor_data_struct eng; /* engineer mode */
    MSDK_SENSOR_ENG_INFO_STRUCT eng_info;
    kal_uint8 mirror;

    OV5648MIPI_SENSOR_MODE ov5648mipi_sensor_mode;

    kal_bool video_mode;
    kal_bool NightMode;
    kal_uint16 normal_fps; /* video normal mode max fps */
    kal_uint16 night_fps; /* video night mode max fps */
    kal_uint16 FixedFps;
    kal_uint16 shutter;
    kal_uint16 gain;
    kal_uint32 pclk;

    kal_uint32 pvPclk;
    kal_uint32 videoPclk;
    kal_uint32 capPclk;

    kal_uint16 frame_length;
    kal_uint16 line_length;

    kal_uint16 dummy_pixel;
    kal_uint16 dummy_line;

    kal_uint32 maxExposureLines;
} OV5648MIPI_sensor_struct;

static OV5648MIPI_sensor_struct OV5648MIPI_sensor =
{
    .eng =
    {
        .reg = CAMERA_SENSOR_REG_DEFAULT_VALUE,
        .cct = CAMERA_SENSOR_CCT_DEFAULT_VALUE,
    },
    .eng_info =
    {
        .SensorId = 128,
        .SensorType = CMOS_SENSOR,
        .SensorOutputDataFormat = OV5648MIPI_COLOR_FORMAT,
    },
    .ov5648mipi_sensor_mode = OV5648MIPI_SENSOR_MODE_INIT,
    .shutter = 0x3D0,
    .gain = 0x100,
    .pclk = OV5648MIPI_PREVIEW_CLK,
    .pvPclk = OV5648MIPI_PREVIEW_CLK,
    .capPclk = OV5648MIPI_CAPTURE_CLK,
    .videoPclk = OV5648MIPI_VIDEO_CLK,
    .frame_length = OV5648MIPI_PV_PERIOD_LINE_NUMS,
    .line_length = OV5648MIPI_PV_PERIOD_PIXEL_NUMS,
    .dummy_pixel = 0,
    .dummy_line = 0,
};

static DEFINE_SPINLOCK(ov5648mipi_drv_lock);

static MSDK_SCENARIO_ID_ENUM mCurrentScenarioId = MSDK_SCENARIO_ID_CAMERA_PREVIEW;
static kal_bool OV5648MIPIAutoFlickerMode = KAL_FALSE;
kal_bool OV5648MIPIDuringTestPattern = KAL_FALSE;
#define OV5648MIPI_TEST_PATTERN_CHECKSUM (0xc24ec08d)

extern int iReadRegI2C(u8 *a_pSendData , u16 a_sizeSendData, u8 * a_pRecvData, u16 a_sizeRecvData, u16 i2cId);
extern int iWriteRegI2C(u8 *a_pSendData , u16 a_sizeSendData, u16 i2cId);
//void OV5648MIPISetMaxFrameRate(UINT16 u2FrameRate);


kal_uint16 OV5648MIPI_read_cmos_sensor(kal_uint32 addr)
{
    kal_uint16 get_byte=0;

    char puSendCmd[2] = {(char)(addr >> 8), (char)(addr & 0xFF) };
    iReadRegI2C(puSendCmd, 2, (u8*)&get_byte, 1, OV5648MIPI_WRITE_ID);

    return get_byte;
}

void OV5648MIPI_write_cmos_sensor(kal_uint32 addr, kal_uint32 para)
{
    char puSendCmd[3] = {(char)(addr >> 8), (char)(addr & 0xFF), (char)(para & 0xFF)};
    iWriteRegI2C(puSendCmd, 3, OV5648MIPI_WRITE_ID);
}



//OTP Code Start
/*
#ifdef OV5648MIPI_USE_OTP
//index: index of otp group.(1, 2)
//return:     ERROR: group index have invalid data
//         TRUE: group index has valid data
kal_uint16 OV5648MIPI_check_otp_wb(kal_uint16 index)
{
    kal_uint16 i;
    kal_uint16 rg = 0, bg = 0;

    //clear otp buffer
    for(i = 0; i < 16; i++)
    {
        OV5648MIPI_write_cmos_sensor(0x3d00 + i, 0x00);
    }

    if(index == 2)  //Group 2
    {
        //read otp bank 1
        OV5648MIPI_write_cmos_sensor(0x3d84,0xc0);
        OV5648MIPI_write_cmos_sensor(0x3d85,0x10); //OTP start address, bank 0
        OV5648MIPI_write_cmos_sensor(0x3d86,0x1f); //OTP end address
        OV5648MIPI_write_cmos_sensor(0x3d81,0x01); //OTP Read Enable

        msleep(50); // delay 50ms

        rg = (OV5648MIPI_read_cmos_sensor(0x3d07) << 8) + OV5648MIPI_read_cmos_sensor(0x3d08);
        bg = (OV5648MIPI_read_cmos_sensor(0x3d09) << 8) + OV5648MIPI_read_cmos_sensor(0x3d0a);
    }
    else if(index == 1) //Group 1
    {
        //read otp bank 0
        OV5648MIPI_write_cmos_sensor(0x3d84,0xc0);
        OV5648MIPI_write_cmos_sensor(0x3d85,0x00); //OTP start address, bank 0
        OV5648MIPI_write_cmos_sensor(0x3d86,0x0f); //OTP end address
        OV5648MIPI_write_cmos_sensor(0x3d81,0x01); //OTP Read Enable

        msleep(50); // delay 50ms

        rg = (OV5648MIPI_read_cmos_sensor(0x3d0a) << 8) + OV5648MIPI_read_cmos_sensor(0x3d0b);
        bg = (OV5648MIPI_read_cmos_sensor(0x3d0c) << 8) + OV5648MIPI_read_cmos_sensor(0x3d0d);
    }
    else
    {
        SENSORDB("invalid index!");
        return FALSE;
    }

    //disable otp read
    OV5648MIPI_write_cmos_sensor(0x3d81,0x00);

    //clear otp buffer
    for(i=0; i<16; i++)
    {
        OV5648MIPI_write_cmos_sensor(0x3d00 + i, 0x00);
    }

    if((rg == 0)||(bg == 0))
    {
        return FALSE;
    }
    else
    {
        return TRUE;
    }
}

//index:index of otp group.(1,2,3)
//return:    0.
kal_uint16 OV5648MIPI_read_otp_wb(kal_uint16 index, struct OV5648MIPI_otp_struct *otp_ptr)
{
    kal_uint16 i;

    //clear otp buffer
    for(i = 0; i < 16; i++)
    {
        OV5648MIPI_write_cmos_sensor(0x3d00 + i, 0x00);
    }

    if(index == 2)  //Group 2
    {
        //read otp bank 1
        OV5648MIPI_write_cmos_sensor(0x3d84,0xc0);
        OV5648MIPI_write_cmos_sensor(0x3d85,0x10); //OTP start address, bank 0
        OV5648MIPI_write_cmos_sensor(0x3d86,0x1f); //OTP end address
        OV5648MIPI_write_cmos_sensor(0x3d81,0x01); //OTP Read Enable

        msleep(50); // delay 50ms

        (*otp_ptr).rg_ratio = (OV5648MIPI_read_cmos_sensor(0x3d07) << 8) + OV5648MIPI_read_cmos_sensor(0x3d08);
        (*otp_ptr).bg_ratio = (OV5648MIPI_read_cmos_sensor(0x3d09) << 8) + OV5648MIPI_read_cmos_sensor(0x3d0a);
        (*otp_ptr).gb_gr_ratio = (OV5648MIPI_read_cmos_sensor(0x3d0b) << 8) + OV5648MIPI_read_cmos_sensor(0x3d0c);
    }
    else if(index == 1) //Group 1
    {
        //read otp bank 0
        OV5648MIPI_write_cmos_sensor(0x3d84,0xc0);
        OV5648MIPI_write_cmos_sensor(0x3d85,0x00); //OTP start address, bank 0
        OV5648MIPI_write_cmos_sensor(0x3d86,0x0f); //OTP end address
        OV5648MIPI_write_cmos_sensor(0x3d81,0x01); //OTP Read Enable

        msleep(50); // delay 50ms

        (*otp_ptr).rg_ratio = (OV5648MIPI_read_cmos_sensor(0x3d0a) << 8) + OV5648MIPI_read_cmos_sensor(0x3d0b);
        (*otp_ptr).bg_ratio = (OV5648MIPI_read_cmos_sensor(0x3d0c) << 8) + OV5648MIPI_read_cmos_sensor(0x3d0d);
        (*otp_ptr).gb_gr_ratio = (OV5648MIPI_read_cmos_sensor(0x3d0e) << 8) + OV5648MIPI_read_cmos_sensor(0x3d0f);
    }
    else
    {
        SENSORDB("invalid index!");
        return FALSE;
    }

    //disable otp read
    OV5648MIPI_write_cmos_sensor(0x3d81, 0x00);

    //clear otp buffer
    for(i = 0; i < 16; i++)
    {
        OV5648MIPI_write_cmos_sensor(0x3d00 + i, 0x00);
    }

    SENSORDB("Read wb otp data success!");
    SENSORDB("rg_ratio = %d, bg_ratio = %d, gb_gr_ratio = %d", (*otp_ptr).rg_ratio, (*otp_ptr).bg_ratio, (*otp_ptr).gb_gr_ratio);

    return TRUE;
}

//R_gain: red gain of sensor AWB, 0x400 = 1
//G_gain: green gain of sensor AWB, 0x400 = 1
//B_gain: blue gain of sensor AWB, 0x400 = 1
//reutrn 0
kal_uint16 OV5648MIPI_update_wb_gain(kal_uint32 R_gain, kal_uint32 G_gain, kal_uint32 B_gain)
{
    SENSORDB("R_gain[%x] G_gain[%x] B_gain[%x]", R_gain, G_gain, B_gain);

    if(R_gain > 0x400)
    {
        OV5648MIPI_write_cmos_sensor(0x5186, R_gain >> 8);
        OV5648MIPI_write_cmos_sensor(0x5187, (R_gain & 0x00FF));
    }

    if(G_gain > 0x400)
    {
        OV5648MIPI_write_cmos_sensor(0x5188, G_gain >> 8);
        OV5648MIPI_write_cmos_sensor(0x5189, (G_gain & 0x00FF));
    }

    if(B_gain > 0x400)
    {
        OV5648MIPI_write_cmos_sensor(0x518a, B_gain >> 8);
        OV5648MIPI_write_cmos_sensor(0x518b, (B_gain & 0x00FF));
    }

    return 0;
}

//R/G and B/G ratio of typical camera module is defined here
kal_uint32 RG_Ratio_Typical = RG_Typical;
kal_uint32 BG_Ratio_Typical = BG_Typical;
kal_uint32 GB_GR_Ratio_Typical = GB_GR_Typical;


//call this function after OV5648 initialization
//return value:    0: Update success
//                  1: No OTP
kal_uint16 update_otp(void)
{
    struct OV5648MIPI_otp_struct current_otp;
    kal_uint32 i, otp_index;
    kal_uint32 R_gain, B_gain, G_gain, G_gain_R,G_gain_B;
    kal_uint32 rg, bg;

    // R/G and B/G of current camera module is read out from sensor OTP
    // check first wb OTP with valid OTP
    // Have two groups otp data, Group 1 & Group 2, read group 2 OTP data first
    for(i = 2; i >= 1; i--)
    {
        if(OV5648MIPI_check_otp_wb(i))
        {
            otp_index = i;
            break;
        }
    }
    if(i == 0)
    {
        //no valid wb OTP data
         SENSORDB("no valid wb OTP data!");
        return 1;
    }

    OV5648MIPI_read_otp_wb(otp_index, &current_otp);

    rg=current_otp.rg_ratio;
    bg=current_otp.bg_ratio;

    //calculate G gain
    //0x400 =1xgain
    if(bg < BG_Ratio_Typical)
    {
        if(rg < RG_Ratio_Typical)
        {
            //current_opt.bg_ratio < BG_Ratio_Typical &&
            //cuttent_otp.rg < RG_Ratio_Typical
            G_gain = 0x400;
            B_gain = 0x400 * BG_Ratio_Typical /bg;
            R_gain = 0x400 * RG_Ratio_Typical /rg;
        }
        else
        {
            //current_otp.bg_ratio < BG_Ratio_Typical &&
               //current_otp.rg_ratio >= RG_Ratio_Typical
               R_gain = 0x400;
            G_gain = 0x400 * rg / RG_Ratio_Typical;
            B_gain = G_gain * BG_Ratio_Typical /bg;
        }
    }
    else
    {
        if(rg < RG_Ratio_Typical)
        {
            //current_otp.bg_ratio >= BG_Ratio_Typical &&
               //current_otp.rg_ratio < RG_Ratio_Typical
               B_gain = 0x400;
            G_gain = 0x400 * bg/ BG_Ratio_Typical;
            R_gain = G_gain * RG_Ratio_Typical / rg;
        }
        else
        {
            //current_otp.bg_ratio >= BG_Ratio_Typical &&
            //current_otp.rg_ratio >= RG_Ratio_Typical
            G_gain_B = 0x400 * bg / BG_Ratio_Typical;
            G_gain_R = 0x400 * rg / RG_Ratio_Typical;

            if(G_gain_B > G_gain_R)
            {
                B_gain = 0x400;
                G_gain = G_gain_B;
                R_gain = G_gain * RG_Ratio_Typical / rg;
            }
            else
            {
                R_gain = 0x400;
                G_gain = G_gain_R;
                B_gain = G_gain * BG_Ratio_Typical / bg;
            }
        }
    }

    //write sensor wb gain to register
    OV5648MIPI_update_wb_gain(R_gain, G_gain, B_gain);
    //success
    return 0;
}

#endif
*/
//OTP Code End
#define OV5648_WB_OTP

#ifdef OV5648_WB_OTP

static int module_id;
static int RG_Ratio_Typical = 0x1A8;//0x19f;
static int BG_Ratio_Typical = 0x165;//0x163;

struct otp_struct {
	kal_uint16 module_integrator_id;
	kal_uint16 lens_id;
	kal_uint16 rg_ratio;
	kal_uint16 bg_ratio;
	kal_uint16 user_data[2];
	kal_uint16 light_rg;
	kal_uint16 light_bg;
};

/* index: index of otp group. (1, 2)
** return:
** 	0, group index is empty
** 	1, group index has invalid data
**	2, group index has valid data
*/
static kal_uint16 check_otp(kal_uint16 index)
{
	kal_uint16 flag, i;
	kal_uint16 rg, bg;

	if (index == 1)
	{
		// read otp --Bank 0
		OV5648MIPI_write_cmos_sensor(0x3d84, 0xc0);
		OV5648MIPI_write_cmos_sensor(0x3d85, 0x00);
		OV5648MIPI_write_cmos_sensor(0x3d86, 0x0f);
		OV5648MIPI_write_cmos_sensor(0x3d81, 0x01);
		mdelay(5);
		flag = OV5648MIPI_read_cmos_sensor(0x3d05);
		rg = OV5648MIPI_read_cmos_sensor(0x3d07);
		bg = OV5648MIPI_read_cmos_sensor(0x3d08);
	}
	else if (index == 2)
	{
		// read otp --Bank 0
		OV5648MIPI_write_cmos_sensor(0x3d84, 0xc0);
		OV5648MIPI_write_cmos_sensor(0x3d85, 0x00);
		OV5648MIPI_write_cmos_sensor(0x3d86, 0x0f);
		OV5648MIPI_write_cmos_sensor(0x3d81, 0x01);
		mdelay(5);
		flag = OV5648MIPI_read_cmos_sensor(0x3d0e);
		// read otp --Bank 1
		OV5648MIPI_write_cmos_sensor(0x3d84, 0xc0);
		OV5648MIPI_write_cmos_sensor(0x3d85, 0x10);
		OV5648MIPI_write_cmos_sensor(0x3d86, 0x1f);
		OV5648MIPI_write_cmos_sensor(0x3d81, 0x01);
		mdelay(5);
		rg = OV5648MIPI_read_cmos_sensor(0x3d00);
		bg = OV5648MIPI_read_cmos_sensor(0x3d01);
	}
	flag = flag & 0x80;

	// clear otp buffer
	for (i = 0; i < 16; i++) {
		OV5648MIPI_write_cmos_sensor(0x3d00 + i, 0x00);
	}

	SENSORDB("check_otp() = 0x%x, 0x%x, 0x%x\r\n", flag, rg, bg);

	if (flag) {
		return 1;
	}
	else
	{
		if (0 == rg && 0 == bg)
		{
			return 0;
		}
		else
		{
			return 2;
		}
	}
}

/* index: index of otp group. (1, 2)
** return:	0,
*/
static kal_uint16 read_otp(kal_uint16 index, struct otp_struct *otp_ptr)
{
	kal_uint16 i, temp;

	// read otp into buffer
	if (index == 1)
	{
		// read otp --Bank 0
		OV5648MIPI_write_cmos_sensor(0x3d84, 0xc0);
		OV5648MIPI_write_cmos_sensor(0x3d85, 0x00);
		OV5648MIPI_write_cmos_sensor(0x3d86, 0x0f);
		OV5648MIPI_write_cmos_sensor(0x3d81, 0x01);
		mdelay(5);
		(*otp_ptr).module_integrator_id = (OV5648MIPI_read_cmos_sensor(0x3d05) & 0x7f);
		(*otp_ptr).lens_id = OV5648MIPI_read_cmos_sensor(0x3d06);
		temp = OV5648MIPI_read_cmos_sensor(0x3d0b);
		(*otp_ptr).rg_ratio = (OV5648MIPI_read_cmos_sensor(0x3d07)<<2) + ((temp>>6) & 0x03);
		(*otp_ptr).bg_ratio = (OV5648MIPI_read_cmos_sensor(0x3d08)<<2) + ((temp>>4) & 0x03);
		(*otp_ptr).light_rg = (OV5648MIPI_read_cmos_sensor(0x3d0c)<<2) + ((temp>>2) & 0x03);
		(*otp_ptr).light_bg = (OV5648MIPI_read_cmos_sensor(0x3d0d)<<2) + (temp & 0x03);
		(*otp_ptr).user_data[0] = OV5648MIPI_read_cmos_sensor(0x3d09);
		(*otp_ptr).user_data[1] = OV5648MIPI_read_cmos_sensor(0x3d0a);
	}
	else if (index == 2)
	{
		// read otp --Bank 0
		OV5648MIPI_write_cmos_sensor(0x3d84, 0xc0);
		OV5648MIPI_write_cmos_sensor(0x3d85, 0x00);
		OV5648MIPI_write_cmos_sensor(0x3d86, 0x0f);
		OV5648MIPI_write_cmos_sensor(0x3d81, 0x01);
		mdelay(5);
		(*otp_ptr).module_integrator_id = (OV5648MIPI_read_cmos_sensor(0x3d0e) & 0x7f);
		(*otp_ptr).lens_id = OV5648MIPI_read_cmos_sensor(0x3d0f);
		// read otp --Bank 1
		OV5648MIPI_write_cmos_sensor(0x3d84, 0xc0);
		OV5648MIPI_write_cmos_sensor(0x3d85, 0x10);
		OV5648MIPI_write_cmos_sensor(0x3d86, 0x1f);
		OV5648MIPI_write_cmos_sensor(0x3d81, 0x01);
		mdelay(5);
		temp = OV5648MIPI_read_cmos_sensor(0x3d04);
		(*otp_ptr).rg_ratio = (OV5648MIPI_read_cmos_sensor(0x3d00)<<2) + ((temp>>6) & 0x03);
		(*otp_ptr).bg_ratio = (OV5648MIPI_read_cmos_sensor(0x3d01)<<2) + ((temp>>4) & 0x03);
		(*otp_ptr).light_rg = (OV5648MIPI_read_cmos_sensor(0x3d05)<<2) + ((temp>>2) & 0x03);
		(*otp_ptr).light_bg = (OV5648MIPI_read_cmos_sensor(0x3d06)<<2) + (temp & 0x03);
		(*otp_ptr).user_data[0] = OV5648MIPI_read_cmos_sensor(0x3d02);
		(*otp_ptr).user_data[1] = OV5648MIPI_read_cmos_sensor(0x3d03);
	}
	module_id = (*otp_ptr).module_integrator_id;

	// clear otp buffer
	for (i = 0; i < 16; i++) {
		OV5648MIPI_write_cmos_sensor(0x3d00 + i, 0x00);
	}

	return 0;
}

/* R_gain, sensor red gain of AWB, 0x400 =1
** G_gain, sensor green gain of AWB, 0x400 =1
** B_gain, sensor blue gain of AWB, 0x400 =1
** return 0;
*/

static kal_uint32 update_awb_gain(kal_uint32 R_gain, kal_uint32 G_gain, kal_uint32 B_gain)
{
	if (R_gain>0x400) {
		OV5648MIPI_write_cmos_sensor(0x5186, R_gain>>8);
		OV5648MIPI_write_cmos_sensor(0x5187, R_gain & 0x00ff);
	}

	if (G_gain>0x400) {
		OV5648MIPI_write_cmos_sensor(0x5188, G_gain>>8);
		OV5648MIPI_write_cmos_sensor(0x5189, G_gain & 0x00ff);
	}

	if (B_gain>0x400) {
		OV5648MIPI_write_cmos_sensor(0x518a, B_gain>>8);
		OV5648MIPI_write_cmos_sensor(0x518b, B_gain & 0x00ff);
	}
	return 0;
}


/* call this function after OV5648 initialization
** return:
** 		0, update success
**	       1, no OTP
*/
static kal_uint32 update_otp(void)
{
	struct otp_struct current_otp;
	kal_uint8 i;
	kal_uint8 otp_index;
	kal_uint8 temp;
	kal_uint32 R_gain, G_gain, B_gain, G_gain_R, G_gain_B;
	kal_uint32 rg,bg;

	// R/G and B/G of current camera module is read out from sensor OTP
	// check first OTP with valid data
	for( i = 1; i <= 2; i++) {
		temp = check_otp(i);
		if (temp == 2) {
			otp_index = i;
			break;
		}
	}

	if (i > 2) {
		// no valid wb OTP data
		return 1;
	}

	read_otp(otp_index, &current_otp);

	if(current_otp.light_rg==0) {
		// no light source information in OTP
		rg = current_otp.rg_ratio;
	}
	else {
		// light source information found in OTP
		rg = current_otp.rg_ratio * (current_otp.light_rg + 512) / 1024;
	}

	if(current_otp.light_bg==0) {
		// no light source information in OTP
		bg = current_otp.bg_ratio;
	}
	else {
		// light source information found in OTP
		bg = current_otp.bg_ratio * (current_otp.light_bg + 512) / 1024;
	}

	//calculate G gain
	//0x400 = 1x gain
	if(bg < BG_Ratio_Typical) {
		if (rg< RG_Ratio_Typical) {
			// current_otp.bg_ratio < BG_Ratio_typical &&?
			// current_otp.rg_ratio < RG_Ratio_typical
			G_gain = 0x400;
			B_gain = 0x400 * BG_Ratio_Typical / bg;
			R_gain = 0x400 * RG_Ratio_Typical / rg;
		}
		else {
			// current_otp.bg_ratio < BG_Ratio_typical &&?
			// current_otp.rg_ratio >= RG_Ratio_typical
			R_gain = 0x400;
			G_gain = 0x400 * rg / RG_Ratio_Typical;
			B_gain = G_gain * BG_Ratio_Typical /bg;
		}
	}
	else {
		if (rg < RG_Ratio_Typical) {
			// current_otp.bg_ratio >= BG_Ratio_typical &&?
			// current_otp.rg_ratio < RG_Ratio_typical
			B_gain = 0x400;
			G_gain = 0x400 * bg / BG_Ratio_Typical;
			R_gain = G_gain * RG_Ratio_Typical / rg;
		}
		else {
			// current_otp.bg_ratio >= BG_Ratio_typical &&?
			// current_otp.rg_ratio >= RG_Ratio_typical
			G_gain_B = 0x400 * bg / BG_Ratio_Typical;
			G_gain_R = 0x400 * rg / RG_Ratio_Typical;

			if(G_gain_B > G_gain_R ) {
				B_gain = 0x400;
				G_gain = G_gain_B;
				R_gain = G_gain * RG_Ratio_Typical /rg;
			}
			else {
				R_gain = 0x400;
				G_gain = G_gain_R;
				B_gain = G_gain * BG_Ratio_Typical / bg;
			}
		}
	}
	//before OTP update
	SENSORDB("reg[0x5186] = %x\r\n", OV5648MIPI_read_cmos_sensor(0x5186));
	SENSORDB("reg[0x5187] = %x\r\n", OV5648MIPI_read_cmos_sensor(0x5187));
	SENSORDB("reg[0x5188] = %x\r\n", OV5648MIPI_read_cmos_sensor(0x5188));
	SENSORDB("reg[0x5189] = %x\r\n", OV5648MIPI_read_cmos_sensor(0x5189));
	SENSORDB("reg[0x518a] = %x\r\n", OV5648MIPI_read_cmos_sensor(0x518a));
	SENSORDB("reg[0x518b] = %x\r\n", OV5648MIPI_read_cmos_sensor(0x518b));
	SENSORDB("RG_Ratio_Typical = %x\r\n", RG_Ratio_Typical);
	SENSORDB("BG_Ratio_Typical = %x\r\n", BG_Ratio_Typical);
	SENSORDB("RG = %x\r\n", rg);//rg
	SENSORDB("BG = %x\r\n", bg);//bg


	update_awb_gain(R_gain, G_gain, B_gain);
    //after OTP update
	SENSORDB("reg[0x5186] = %x\r\n", OV5648MIPI_read_cmos_sensor(0x5186));
	SENSORDB("reg[0x5187] = %x\r\n", OV5648MIPI_read_cmos_sensor(0x5187));
	SENSORDB("reg[0x5188] = %x\r\n", OV5648MIPI_read_cmos_sensor(0x5188));
	SENSORDB("reg[0x5189] = %x\r\n", OV5648MIPI_read_cmos_sensor(0x5189));
	SENSORDB("reg[0x518a] = %x\r\n", OV5648MIPI_read_cmos_sensor(0x518a));
	SENSORDB("reg[0x518b] = %x\r\n", OV5648MIPI_read_cmos_sensor(0x518b));

	return 0;
}

#endif




static void OV5648MIPI_Write_Shutter(kal_uint16 iShutter)
{
    kal_uint32 min_framelength = 0, max_shutter=0;
    kal_uint32 extra_lines = 0;
    kal_uint32 frame_length = 0;
    kal_uint32 line_length = 0;
    unsigned long flags;


    #ifdef OV5648MIPI_DRIVER_TRACE
        SENSORDB("iShutter =  %d", iShutter);
    #endif

    if(OV5648MIPIAutoFlickerMode)
    {
        if (OV5648MIPI_SENSOR_MODE_PREVIEW  == OV5648MIPI_sensor.ov5648mipi_sensor_mode)
        {
            line_length = OV5648MIPI_PV_PERIOD_PIXEL_NUMS + OV5648MIPI_sensor.dummy_pixel;
            max_shutter = OV5648MIPI_PV_PERIOD_LINE_NUMS + OV5648MIPI_sensor.dummy_line;
        }
        else if (OV5648MIPI_SENSOR_MODE_VIDEO  == OV5648MIPI_sensor.ov5648mipi_sensor_mode)
        {
            line_length = OV5648MIPI_PV_PERIOD_PIXEL_NUMS + OV5648MIPI_sensor.dummy_pixel;
            max_shutter = OV5648MIPI_PV_PERIOD_LINE_NUMS + OV5648MIPI_sensor.dummy_line;
        }
        else
        {
            line_length = OV5648MIPI_FULL_PERIOD_PIXEL_NUMS + OV5648MIPI_sensor.dummy_pixel;
            max_shutter = OV5648MIPI_FULL_PERIOD_LINE_NUMS + OV5648MIPI_sensor.dummy_line;
        }

        switch(mCurrentScenarioId)
        {
            case MSDK_SCENARIO_ID_CAMERA_ZSD:
            case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
                SENSORDB("AutoFlickerMode!!! MSDK_SCENARIO_ID_CAMERA_ZSD  0!!\n");
                min_framelength = max_shutter;// capture max_fps 24,no need calculate
                break;
            case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
                if(OV5648MIPI_sensor.FixedFps == 30)
                {
                    min_framelength = (OV5648MIPI_sensor.videoPclk) /(OV5648MIPI_PV_PERIOD_PIXEL_NUMS + OV5648MIPI_sensor.dummy_pixel)/285*10 ;
                }
                else if(OV5648MIPI_sensor.FixedFps == 15)
                {
                    min_framelength = (OV5648MIPI_sensor.videoPclk) /(OV5648MIPI_PV_PERIOD_PIXEL_NUMS + OV5648MIPI_sensor.dummy_pixel)/146*10 ;
                }
                else
                {
                    min_framelength = max_shutter;
                }
                break;
            default:
                min_framelength = (OV5648MIPI_sensor.pvPclk) /(OV5648MIPI_PV_PERIOD_PIXEL_NUMS + OV5648MIPI_sensor.dummy_pixel)/285*10 ;
                break;
        }

        SENSORDB("AutoFlickerMode!!! min_framelength for AutoFlickerMode = %d (0x%x)\n", min_framelength, min_framelength);
        SENSORDB("max framerate(10 base) autofilker = %d\n",(OV5648MIPI_sensor.pvPclk)*10 /line_length/min_framelength);

        if (iShutter < 3)
            iShutter = 3;

        if (iShutter > max_shutter-4)
            extra_lines = iShutter - max_shutter + 4;
        else
            extra_lines = 0;

        if (OV5648MIPI_SENSOR_MODE_PREVIEW  == OV5648MIPI_sensor.ov5648mipi_sensor_mode)
        {
            frame_length = OV5648MIPI_PV_PERIOD_LINE_NUMS + OV5648MIPI_sensor.dummy_line + extra_lines ;
        }
        else if (OV5648MIPI_SENSOR_MODE_VIDEO  == OV5648MIPI_sensor.ov5648mipi_sensor_mode)
        {
            frame_length = OV5648MIPI_PV_PERIOD_LINE_NUMS + OV5648MIPI_sensor.dummy_line + extra_lines ;
        }
        else
        {
            frame_length = OV5648MIPI_FULL_PERIOD_LINE_NUMS + OV5648MIPI_sensor.dummy_line + extra_lines ;
        }
        SENSORDB("frame_length 0= %d\n",frame_length);


        if (frame_length < min_framelength)
        {
            switch(mCurrentScenarioId)
            {
            case MSDK_SCENARIO_ID_CAMERA_ZSD:
            case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
                extra_lines = min_framelength- (OV5648MIPI_FULL_PERIOD_LINE_NUMS+ OV5648MIPI_sensor.dummy_line);
                break;
            case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
                extra_lines = min_framelength- (OV5648MIPI_PV_PERIOD_LINE_NUMS+ OV5648MIPI_sensor.dummy_line);
                break;
            default:
                extra_lines = min_framelength- (OV5648MIPI_PV_PERIOD_LINE_NUMS+ OV5648MIPI_sensor.dummy_line);
                break;
            }
            frame_length = min_framelength;
        }

        SENSORDB("frame_length 1= %d\n", frame_length);


        //Set total frame length
        OV5648MIPI_write_cmos_sensor(0x380e, (frame_length >> 8) & 0xFF);
        OV5648MIPI_write_cmos_sensor(0x380f, frame_length & 0xFF);

        spin_lock_irqsave(&ov5648mipi_drv_lock,flags);
        OV5648MIPI_sensor.maxExposureLines = frame_length;
        OV5648MIPI_sensor.line_length = line_length;
        OV5648MIPI_sensor.frame_length = frame_length;
        spin_unlock_irqrestore(&ov5648mipi_drv_lock,flags);

        //Set shutter (Coarse integration time, uint: lines.)
        OV5648MIPI_write_cmos_sensor(0x3500, (iShutter>>12) & 0x0F);
        OV5648MIPI_write_cmos_sensor(0x3501, (iShutter>>4) & 0xFF);
        OV5648MIPI_write_cmos_sensor(0x3502, (iShutter<<4) & 0xF0);  /* Don't use the fraction part. */

        SENSORDB("frame_length 2= %d\n",frame_length);
        SENSORDB("shutter=%d, extra_lines=%d, line_length=%d, frame_length=%d\n", iShutter, extra_lines, line_length, frame_length);
    }
    else
    {
        if (OV5648MIPI_SENSOR_MODE_PREVIEW  == OV5648MIPI_sensor.ov5648mipi_sensor_mode)
        {
            line_length = OV5648MIPI_PV_PERIOD_PIXEL_NUMS + OV5648MIPI_sensor.dummy_pixel;
            max_shutter = OV5648MIPI_PV_PERIOD_LINE_NUMS + OV5648MIPI_sensor.dummy_line;
        }
        else if (OV5648MIPI_SENSOR_MODE_VIDEO  == OV5648MIPI_sensor.ov5648mipi_sensor_mode)
        {
            line_length = OV5648MIPI_PV_PERIOD_PIXEL_NUMS + OV5648MIPI_sensor.dummy_pixel;
            max_shutter = OV5648MIPI_PV_PERIOD_LINE_NUMS + OV5648MIPI_sensor.dummy_line;
        }
        else
        {
            line_length = OV5648MIPI_FULL_PERIOD_PIXEL_NUMS + OV5648MIPI_sensor.dummy_pixel;
            max_shutter = OV5648MIPI_FULL_PERIOD_LINE_NUMS + OV5648MIPI_sensor.dummy_line;
        }


        if (iShutter < 3)
            iShutter = 3;

        if (iShutter > max_shutter-4)
            extra_lines = iShutter - max_shutter + 4;
        else
            extra_lines = 0;

        if (OV5648MIPI_SENSOR_MODE_PREVIEW  == OV5648MIPI_sensor.ov5648mipi_sensor_mode)
        {
            frame_length = OV5648MIPI_PV_PERIOD_LINE_NUMS + OV5648MIPI_sensor.dummy_line + extra_lines ;
        }
        else if (OV5648MIPI_SENSOR_MODE_VIDEO  == OV5648MIPI_sensor.ov5648mipi_sensor_mode)
        {
            frame_length = OV5648MIPI_PV_PERIOD_LINE_NUMS + OV5648MIPI_sensor.dummy_line + extra_lines ;
        }
        else
        {
            frame_length = OV5648MIPI_FULL_PERIOD_LINE_NUMS + OV5648MIPI_sensor.dummy_line + extra_lines ;
        }
        SENSORDB("frame_length 0= %d\n",frame_length);

        //Set total frame length
        OV5648MIPI_write_cmos_sensor(0x380e, (frame_length >> 8) & 0xFF);
        OV5648MIPI_write_cmos_sensor(0x380f, frame_length & 0xFF);


        spin_lock_irqsave(&ov5648mipi_drv_lock,flags);
        OV5648MIPI_sensor.maxExposureLines = frame_length;
        OV5648MIPI_sensor.line_length = line_length;
        OV5648MIPI_sensor.frame_length = frame_length;
        spin_unlock_irqrestore(&ov5648mipi_drv_lock,flags);

        //Set shutter (Coarse integration time, uint: lines.)
        OV5648MIPI_write_cmos_sensor(0x3500, (iShutter>>12) & 0x0F);
        OV5648MIPI_write_cmos_sensor(0x3501, (iShutter>>4) & 0xFF);
        OV5648MIPI_write_cmos_sensor(0x3502, (iShutter<<4) & 0xF0);  /* Don't use the fraction part. */

        SENSORDB("frame_length 2= %d\n",frame_length);
        SENSORDB("shutter=%d, extra_lines=%d, line_length=%d, frame_length=%d\n", iShutter, extra_lines, line_length, frame_length);
    }
}   /*  OV5648MIPI_Write_Shutter  */

static void OV5648MIPI_Set_Dummy(const kal_uint16 iDummyPixels, const kal_uint16 iDummyLines)
{
    kal_uint16 line_length, frame_length;

    #ifdef OV5648MIPI_DRIVER_TRACE
        SENSORDB("iDummyPixels = %d, iDummyLines = %d ", iDummyPixels, iDummyLines);
    #endif

    if (OV5648MIPI_SENSOR_MODE_PREVIEW == OV5648MIPI_sensor.ov5648mipi_sensor_mode){
        line_length = OV5648MIPI_PV_PERIOD_PIXEL_NUMS + iDummyPixels;
        frame_length = OV5648MIPI_PV_PERIOD_LINE_NUMS + iDummyLines;
    }
    else if (OV5648MIPI_SENSOR_MODE_VIDEO  == OV5648MIPI_sensor.ov5648mipi_sensor_mode)
    {
        line_length = OV5648MIPI_PV_PERIOD_PIXEL_NUMS + iDummyPixels;
        frame_length = OV5648MIPI_PV_PERIOD_LINE_NUMS + iDummyLines;
    }
    else
    {
        line_length = OV5648MIPI_FULL_PERIOD_PIXEL_NUMS + iDummyPixels;
        frame_length = OV5648MIPI_FULL_PERIOD_LINE_NUMS + iDummyLines;
    }

    OV5648MIPI_sensor.dummy_pixel = iDummyPixels;
    OV5648MIPI_sensor.dummy_line = iDummyLines;
    OV5648MIPI_sensor.line_length = line_length;
    OV5648MIPI_sensor.frame_length = frame_length;

    OV5648MIPI_write_cmos_sensor(0x380c, line_length >> 8);
    OV5648MIPI_write_cmos_sensor(0x380d, line_length & 0xFF);
    OV5648MIPI_write_cmos_sensor(0x380e, frame_length >> 8);
    OV5648MIPI_write_cmos_sensor(0x380f, frame_length & 0xFF);

}   /*  OV5648MIPI_Set_Dummy  */

/*
void OV5648MIPISetMaxFrameRate(UINT16 u2FrameRate)
{
    kal_int16 dummy_line;
    kal_uint16 frame_length = OV5648MIPI_sensor.frame_length;
    unsigned long flags;

    #ifdef OV5648MIPI_DRIVER_TRACE
        SENSORDB("u2FrameRate = %d ", u2FrameRate);
    #endif

    frame_length= (10 * OV5648MIPI_sensor.pclk) / u2FrameRate / OV5648MIPI_sensor.line_length;

    spin_lock_irqsave(&ov5648mipi_drv_lock, flags);
    OV5648MIPI_sensor.frame_length = frame_length;
    spin_unlock_irqrestore(&ov5648mipi_drv_lock, flags);

    if(mCurrentScenarioId == MSDK_SCENARIO_ID_CAMERA_ZSD)
        dummy_line = frame_length - OV5648MIPI_FULL_PERIOD_LINE_NUMS;
    else
        dummy_line = frame_length - OV5648MIPI_PV_PERIOD_LINE_NUMS;

    if (dummy_line < 0) dummy_line = 0;

    OV5648MIPI_Set_Dummy(OV5648MIPI_sensor.dummy_pixel, dummy_line);
}
*/

/*************************************************************************
* FUNCTION
*   OV5648MIPI_SetShutter
*
* DESCRIPTION
*   This function set e-shutter of OV5648MIPI to change exposure time.
*
* PARAMETERS
*   iShutter : exposured lines
*
* RETURNS
*   None
*
* GLOBALS AFFECTED
*
*************************************************************************/
void set_OV5648MIPI_shutter(kal_uint16 iShutter)
{
    unsigned long flags;

    spin_lock_irqsave(&ov5648mipi_drv_lock, flags);
    OV5648MIPI_sensor.shutter = iShutter;
    spin_unlock_irqrestore(&ov5648mipi_drv_lock, flags);

    OV5648MIPI_Write_Shutter(iShutter);
}   /*  Set_OV5648MIPI_Shutter */

#if 0
static kal_uint16 OV5648MIPI_Reg2Gain(const kal_uint8 iReg)
{
    kal_uint16 iGain ;
    /* Range: 1x to 32x */
    iGain = (iReg >> 4) * BASEGAIN + (iReg & 0xF) * BASEGAIN / 16;
    return iGain ;
}
#endif

 kal_uint16 OV5648MIPI_Gain2Reg(const kal_uint16 iGain)
{
    kal_uint16 iReg = 0x0000;

    iReg = ((iGain / BASEGAIN) << 4) + ((iGain % BASEGAIN) * 16 / BASEGAIN);
    iReg = iReg & 0xFFFF;
    return (kal_uint16)iReg;
}


/*************************************************************************
* FUNCTION
*   OV5648MIPI_SetGain
*
* DESCRIPTION
*   This function is to set global gain to sensor.
*
* PARAMETERS
*   iGain : sensor global gain(base: 0x40)
*
* RETURNS
*   the actually gain set to sensor.
*
* GLOBALS AFFECTED
*
*************************************************************************/
kal_uint16 OV5648MIPI_SetGain(kal_uint16 iGain)
{
    kal_uint16 iRegGain;

    OV5648MIPI_sensor.gain = iGain;

    /* 0x350A[0:1], 0x350B[0:7] AGC real gain */
    /* [0:3] = N meams N /16 X  */
    /* [4:9] = M meams M X       */
    /* Total gain = M + N /16 X   */

    //
    if(iGain < BASEGAIN || iGain > 32 * BASEGAIN){
        SENSORDB("Error gain setting");

        if(iGain < BASEGAIN) iGain = BASEGAIN;
        if(iGain > 32 * BASEGAIN) iGain = 32 * BASEGAIN;
    }

    iRegGain = OV5648MIPI_Gain2Reg(iGain);

    #ifdef OV5648MIPI_DRIVER_TRACE
        SENSORDB("iGain = %d , iRegGain = 0x%x ", iGain, iRegGain);
    #endif

    OV5648MIPI_write_cmos_sensor(0x350a, iRegGain >> 8);
    OV5648MIPI_write_cmos_sensor(0x350b, iRegGain & 0xFF);

    return iGain;
}   /*  OV5648MIPI_SetGain  */


void OV5648MIPI_Set_Mirror_Flip(kal_uint8 image_mirror, kal_uint8 image_flip)
{
    kal_uint8 HV;
    // SENSORDB("image_mirror = %d,flip = %d", image_mirror, image_flip);
    /********************************************************
       *
       *   0x3820[2] ISP Vertical flip
       *   0x3820[1] Sensor Vertical flip
       *
       *   0x3821[2] ISP Horizontal mirror
       *   0x3821[1] Sensor Horizontal mirror
       *
       *   ISP and Sensor flip or mirror register bit should be the same!!
       *
       ********************************************************/
    HV = image_mirror | (image_flip << 1);
    SENSORDB("image_mirror = %d,flip = %d HV = %d", image_mirror, image_flip, HV);
    switch (HV)
    {
        case IMAGE_NORMAL:
            OV5648MIPI_write_cmos_sensor(0x3820,((OV5648MIPI_read_cmos_sensor(0x3820) & 0xF9) | 0x00));
            OV5648MIPI_write_cmos_sensor(0x3821,((OV5648MIPI_read_cmos_sensor(0x3821) & 0xF9) | 0x00));
            break;
        case IMAGE_H_MIRROR:
            OV5648MIPI_write_cmos_sensor(0x3820,((OV5648MIPI_read_cmos_sensor(0x3820) & 0xF9) | 0x00));
            OV5648MIPI_write_cmos_sensor(0x3821,((OV5648MIPI_read_cmos_sensor(0x3821) & 0xF9) | 0x06));
            break;
        case IMAGE_V_MIRROR:
            OV5648MIPI_write_cmos_sensor(0x3820,((OV5648MIPI_read_cmos_sensor(0x3820) & 0xF9) | 0x06));
            OV5648MIPI_write_cmos_sensor(0x3821,((OV5648MIPI_read_cmos_sensor(0x3821) & 0xF9) | 0x00));
            break;
        case IMAGE_HV_MIRROR:
            OV5648MIPI_write_cmos_sensor(0x3820,((OV5648MIPI_read_cmos_sensor(0x3820) & 0xF9) | 0x06));
            OV5648MIPI_write_cmos_sensor(0x3821,((OV5648MIPI_read_cmos_sensor(0x3821) & 0xF9) | 0x06));
            break;
        default:
            SENSORDB("Error image_mirror setting");
    }

}


/*************************************************************************
* FUNCTION
*   OV5648MIPI_NightMode
*
* DESCRIPTION
*   This function night mode of OV5648MIPI.
*
* PARAMETERS
*   bEnable: KAL_TRUE -> enable night mode, otherwise, disable night mode
*
* RETURNS
*   None
*
* GLOBALS AFFECTED
*
*************************************************************************/
void OV5648MIPI_night_mode(kal_bool enable)
{
/*No Need to implement this function*/
}   /*  OV5648MIPI_night_mode  */


/* write camera_para to sensor register */
static void OV5648MIPI_camera_para_to_sensor(void)
{
    kal_uint32 i;

    SENSORDB("OV5648MIPI_camera_para_to_sensor\n");

    for (i = 0; 0xFFFFFFFF != OV5648MIPI_sensor.eng.reg[i].Addr; i++)
    {
        OV5648MIPI_write_cmos_sensor(OV5648MIPI_sensor.eng.reg[i].Addr, OV5648MIPI_sensor.eng.reg[i].Para);
    }
    for (i = OV5648MIPI_FACTORY_START_ADDR; 0xFFFFFFFF != OV5648MIPI_sensor.eng.reg[i].Addr; i++)
    {
        OV5648MIPI_write_cmos_sensor(OV5648MIPI_sensor.eng.reg[i].Addr, OV5648MIPI_sensor.eng.reg[i].Para);
    }
    OV5648MIPI_SetGain(OV5648MIPI_sensor.gain); /* update gain */
}

/* update camera_para from sensor register */
static void OV5648MIPI_sensor_to_camera_para(void)
{
    kal_uint32 i,temp_data;

    SENSORDB("OV5648MIPI_sensor_to_camera_para\n");

    for (i = 0; 0xFFFFFFFF != OV5648MIPI_sensor.eng.reg[i].Addr; i++)
    {
        temp_data =OV5648MIPI_read_cmos_sensor(OV5648MIPI_sensor.eng.reg[i].Addr);

        spin_lock(&ov5648mipi_drv_lock);
        OV5648MIPI_sensor.eng.reg[i].Para = temp_data;
        spin_unlock(&ov5648mipi_drv_lock);
    }
    for (i = OV5648MIPI_FACTORY_START_ADDR; 0xFFFFFFFF != OV5648MIPI_sensor.eng.reg[i].Addr; i++)
    {
        temp_data =OV5648MIPI_read_cmos_sensor(OV5648MIPI_sensor.eng.reg[i].Addr);

        spin_lock(&ov5648mipi_drv_lock);
        OV5648MIPI_sensor.eng.reg[i].Para = temp_data;
        spin_unlock(&ov5648mipi_drv_lock);
    }
}

/* ------------------------ Engineer mode ------------------------ */
inline static void OV5648MIPI_get_sensor_group_count(kal_int32 *sensor_count_ptr)
{

    SENSORDB("OV5648MIPI_get_sensor_group_count\n");

    *sensor_count_ptr = OV5648MIPI_GROUP_TOTAL_NUMS;
}

inline static void OV5648MIPI_get_sensor_group_info(MSDK_SENSOR_GROUP_INFO_STRUCT *para)
{

    SENSORDB("OV5648MIPI_get_sensor_group_info\n");

    switch (para->GroupIdx)
    {
    case OV5648MIPI_PRE_GAIN:
        sprintf(para->GroupNamePtr, "CCT");
        para->ItemCount = 5;
        break;
    case OV5648MIPI_CMMCLK_CURRENT:
        sprintf(para->GroupNamePtr, "CMMCLK Current");
        para->ItemCount = 1;
        break;
    case OV5648MIPI_FRAME_RATE_LIMITATION:
        sprintf(para->GroupNamePtr, "Frame Rate Limitation");
        para->ItemCount = 2;
        break;
    case OV5648MIPI_REGISTER_EDITOR:
        sprintf(para->GroupNamePtr, "Register Editor");
        para->ItemCount = 2;
        break;
    default:
        ASSERT(0);
  }
}

inline static void OV5648MIPI_get_sensor_item_info(MSDK_SENSOR_ITEM_INFO_STRUCT *para)
{

    const static kal_char *cct_item_name[] = {"SENSOR_BASEGAIN", "Pregain-R", "Pregain-Gr", "Pregain-Gb", "Pregain-B"};
    const static kal_char *editer_item_name[] = {"REG addr", "REG value"};

    SENSORDB("OV5648MIPI_get_sensor_item_info");

    switch (para->GroupIdx)
    {
    case OV5648MIPI_PRE_GAIN:
        switch (para->ItemIdx)
        {
        case OV5648MIPI_SENSOR_BASEGAIN:
        case OV5648MIPI_PRE_GAIN_R_INDEX:
        case OV5648MIPI_PRE_GAIN_Gr_INDEX:
        case OV5648MIPI_PRE_GAIN_Gb_INDEX:
        case OV5648MIPI_PRE_GAIN_B_INDEX:
            break;
        default:
            ASSERT(0);
        }
        sprintf(para->ItemNamePtr, cct_item_name[para->ItemIdx - OV5648MIPI_SENSOR_BASEGAIN]);
        para->ItemValue = OV5648MIPI_sensor.eng.cct[para->ItemIdx].Para * 1000 / BASEGAIN;
        para->IsTrueFalse = para->IsReadOnly = para->IsNeedRestart = KAL_FALSE;
        para->Min = OV5648MIPI_MIN_ANALOG_GAIN * 1000;
        para->Max = OV5648MIPI_MAX_ANALOG_GAIN * 1000;
        break;
    case OV5648MIPI_CMMCLK_CURRENT:
        switch (para->ItemIdx)
        {
        case 0:
            sprintf(para->ItemNamePtr, "Drv Cur[2,4,6,8]mA");
            switch (OV5648MIPI_sensor.eng.reg[OV5648MIPI_CMMCLK_CURRENT_INDEX].Para)
            {
            case ISP_DRIVING_2MA:
                para->ItemValue = 2;
                break;
            case ISP_DRIVING_4MA:
                para->ItemValue = 4;
                break;
            case ISP_DRIVING_6MA:
                para->ItemValue = 6;
                break;
            case ISP_DRIVING_8MA:
                para->ItemValue = 8;
                break;
            default:
                ASSERT(0);
            }
            para->IsTrueFalse = para->IsReadOnly = KAL_FALSE;
            para->IsNeedRestart = KAL_TRUE;
            para->Min = 2;
            para->Max = 8;
            break;
        default:
            ASSERT(0);
        }
        break;
    case OV5648MIPI_FRAME_RATE_LIMITATION:
        switch (para->ItemIdx)
        {
        case 0:
            sprintf(para->ItemNamePtr, "Max Exposure Lines");
            para->ItemValue = 5998;
            break;
        case 1:
            sprintf(para->ItemNamePtr, "Min Frame Rate");
            para->ItemValue = 5;
            break;
        default:
            ASSERT(0);
        }
        para->IsTrueFalse = para->IsNeedRestart = KAL_FALSE;
        para->IsReadOnly = KAL_TRUE;
        para->Min = para->Max = 0;
        break;
    case OV5648MIPI_REGISTER_EDITOR:
        switch (para->ItemIdx)
        {
        case 0:
        case 1:
            sprintf(para->ItemNamePtr, editer_item_name[para->ItemIdx]);
            para->ItemValue = 0;
            para->IsTrueFalse = para->IsReadOnly = para->IsNeedRestart = KAL_FALSE;
            para->Min = 0;
            para->Max = (para->ItemIdx == 0 ? 0xFFFF : 0xFF);
            break;
        default:
            ASSERT(0);
        }
        break;
    default:
        ASSERT(0);
  }
}

inline static kal_bool OV5648MIPI_set_sensor_item_info(MSDK_SENSOR_ITEM_INFO_STRUCT *para)
{
    kal_uint16 temp_para;

    SENSORDB("OV5648MIPI_set_sensor_item_info\n");

    switch (para->GroupIdx)
    {
    case OV5648MIPI_PRE_GAIN:
        switch (para->ItemIdx)
        {
        case OV5648MIPI_SENSOR_BASEGAIN:
        case OV5648MIPI_PRE_GAIN_R_INDEX:
        case OV5648MIPI_PRE_GAIN_Gr_INDEX:
        case OV5648MIPI_PRE_GAIN_Gb_INDEX:
        case OV5648MIPI_PRE_GAIN_B_INDEX:
            spin_lock(&ov5648mipi_drv_lock);
            OV5648MIPI_sensor.eng.cct[para->ItemIdx].Para = para->ItemValue * BASEGAIN / 1000;
            spin_unlock(&ov5648mipi_drv_lock);
            OV5648MIPI_SetGain(OV5648MIPI_sensor.gain); /* update gain */
            break;
        default:
            ASSERT(0);
        }
        break;
    case OV5648MIPI_CMMCLK_CURRENT:
        switch (para->ItemIdx)
        {
        case 0:
            switch (para->ItemValue)
            {
            case 2:
                temp_para = ISP_DRIVING_2MA;
                break;
            case 3:
            case 4:
                temp_para = ISP_DRIVING_4MA;
                break;
            case 5:
            case 6:
                temp_para = ISP_DRIVING_6MA;
                break;
            default:
                temp_para = ISP_DRIVING_8MA;
                break;
            }
            spin_lock(&ov5648mipi_drv_lock);
            //OV5648MIPI_set_isp_driving_current(temp_para);
            OV5648MIPI_sensor.eng.reg[OV5648MIPI_CMMCLK_CURRENT_INDEX].Para = temp_para;
            spin_unlock(&ov5648mipi_drv_lock);
            break;
        default:
            ASSERT(0);
        }
        break;
    case OV5648MIPI_FRAME_RATE_LIMITATION:
        ASSERT(0);
        break;
    case OV5648MIPI_REGISTER_EDITOR:
        switch (para->ItemIdx)
        {
        static kal_uint32 fac_sensor_reg;
        case 0:
            if (para->ItemValue < 0 || para->ItemValue > 0xFFFF) return KAL_FALSE;
            fac_sensor_reg = para->ItemValue;
            break;
        case 1:
            if (para->ItemValue < 0 || para->ItemValue > 0xFF) return KAL_FALSE;
            OV5648MIPI_write_cmos_sensor(fac_sensor_reg, para->ItemValue);
            break;
        default:
            ASSERT(0);
        }
        break;
    default:
        ASSERT(0);
    }

    return KAL_TRUE;
}

static void OV5648MIPI_Sensor_Init(void)
{
#ifdef OV5648MIPI_DRIVER_TRACE
   SENSORDB("Enter!");
#endif

   /*****************************************************************************
    0x3037  SC_CMMN_PLL_CTR13
        SC_CMMN_PLL_CTR13[4] pll_root_div 0: bypass 1: /2
        SC_CMMN_PLL_CTR13[3:0] pll_prediv 1, 2, 3, 4, 6, 8

    0x3036  SC_CMMN_PLL_MULTIPLIER
        SC_CMMN_PLL_MULTIPLIER[7:0] PLL_multiplier(4~252) This can be any integer during 4~127 and only even integer during 128 ~ 252

    0x3035  SC_CMMN_PLL_CTR1
        SC_CMMN_PLL_CTR1[7:4] system_pll_div
        SC_CMMN_PLL_CTR1[3:0] scale_divider_mipi

    0x3034  SC_CMMN_PLL_CTR0
        SC_CMMN_PLL_CTR0[6:4] pll_charge_pump
        SC_CMMN_PLL_CTR0[3:0] mipi_bit_mode

    0x3106  SRB CTRL
        SRB CTRL[3:2] PLL_clock_divider
        SRB CTRL[1] rst_arb
        SRB CTRL[0] sclk_arb

    pll_prediv_map[] = {2, 2, 4, 6, 8, 3, 12, 5, 16, 2, 2, 2, 2, 2, 2, 2};
    system_pll_div_map[] = {16, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};
    pll_root_div_map[] = {1, 2};
    mipi_bit_mode_map[] = {2, 2, 2, 2, 2, 2, 2, 2, 4, 2, 5, 2, 2, 2, 2, 2};
    PLL_clock_divider_map[] = {1, 2, 4, 1};

    VCO = XVCLK * 2 / pll_prediv_map[pll_prediv] * PLL_multiplier;
    sysclk = VCO * 2 / system_pll_div_map[system_pll_div] / pll_root_div_map[pll_root_div] / mipi_bit_mode_map[mipi_bit_mode] / PLL_clock_divider_map[PLL_clock_divider]

    Change Register

    VCO = XVCLK * 2 / pll_prediv_map[0x3037[3:0]] * 0x3036[7:0];
    sysclk = VCO * 2 / system_pll_div_map[0x3035[7:4]] / pll_root_div_map[0x3037[4]] / mipi_bit_mode_map[0x3034[3:0]] / PLL_clock_divider_map[0x3106[3:2]]

    XVCLK = 24 MHZ
    0x3106 0x05
    0x3037 0x03
    0x3036 0x69
    0x3035 0x21
    0x3034 0x1A

    VCO = 24 * 2 / 6 * 105
    sysclk = VCO * 2 / 2 / 1 / 5 / 2
    sysclk = 84 MHZ
    */

    //@@ global setting
    OV5648MIPI_write_cmos_sensor(0x0100, 0x00); // Software Standy
    OV5648MIPI_write_cmos_sensor(0x0103, 0x01); // Software Reset

    mDELAY(5);

    OV5648MIPI_write_cmos_sensor(0x3001, 0x00); // D[7:0] set to input
    OV5648MIPI_write_cmos_sensor(0x3002, 0x00); // D[11:8] set to input
    OV5648MIPI_write_cmos_sensor(0x3011, 0x02); // Drive strength 2x

    OV5648MIPI_write_cmos_sensor(0x3018, 0x4c); // MIPI 2 lane
    OV5648MIPI_write_cmos_sensor(0x3022, 0x00);

    OV5648MIPI_write_cmos_sensor(0x3034, 0x1a); // 10-bit mode
    OV5648MIPI_write_cmos_sensor(0x3035, 0x21); // PLL
    OV5648MIPI_write_cmos_sensor(0x3036, 0x69); // PLL

    OV5648MIPI_write_cmos_sensor(0x3037, 0x03); // PLL

    OV5648MIPI_write_cmos_sensor(0x3038, 0x00); // PLL
    OV5648MIPI_write_cmos_sensor(0x3039, 0x00); // PLL
    OV5648MIPI_write_cmos_sensor(0x303a, 0x00); // PLLS
    OV5648MIPI_write_cmos_sensor(0x303b, 0x19); // PLLS
    OV5648MIPI_write_cmos_sensor(0x303c, 0x11); // PLLS
    OV5648MIPI_write_cmos_sensor(0x303d, 0x30); // PLLS
    OV5648MIPI_write_cmos_sensor(0x3105, 0x11);
    OV5648MIPI_write_cmos_sensor(0x3106, 0x05); // PLL
    OV5648MIPI_write_cmos_sensor(0x3304, 0x28);
    OV5648MIPI_write_cmos_sensor(0x3305, 0x41);
    OV5648MIPI_write_cmos_sensor(0x3306, 0x30);
    OV5648MIPI_write_cmos_sensor(0x3308, 0x00);
    OV5648MIPI_write_cmos_sensor(0x3309, 0xc8);
    OV5648MIPI_write_cmos_sensor(0x330a, 0x01);
    OV5648MIPI_write_cmos_sensor(0x330b, 0x90);
    OV5648MIPI_write_cmos_sensor(0x330c, 0x02);
    OV5648MIPI_write_cmos_sensor(0x330d, 0x58);
    OV5648MIPI_write_cmos_sensor(0x330e, 0x03);
    OV5648MIPI_write_cmos_sensor(0x330f, 0x20);
    OV5648MIPI_write_cmos_sensor(0x3300, 0x00);

    // exposure time
    //OV5648MIPI_write_cmos_sensor(0x3500, 0x00); // exposure [19:16]
    //OV5648MIPI_write_cmos_sensor(0x3501, 0x3d); // exposure [15:8]
    //OV5648MIPI_write_cmos_sensor(0x3502, 0x00); // exposure [7:0], exposure = 0x3d0 = 976

    OV5648MIPI_write_cmos_sensor(0x3503, 0x07); // gain has no delay, manual agc/aec

    // gain
    //OV5648MIPI_write_cmos_sensor(0x350a, 0x00); // gain[9:8]
    //OV5648MIPI_write_cmos_sensor(0x350b, 0x40); // gain[7:0], gain = 4x

    OV5648MIPI_write_cmos_sensor(0x3601, 0x33); // analog control
    OV5648MIPI_write_cmos_sensor(0x3602, 0x00); // analog control
    OV5648MIPI_write_cmos_sensor(0x3611, 0x0e); // analog control
    OV5648MIPI_write_cmos_sensor(0x3612, 0x2b); // analog control
    OV5648MIPI_write_cmos_sensor(0x3614, 0x50); // analog control
    OV5648MIPI_write_cmos_sensor(0x3620, 0x33); // analog control
    OV5648MIPI_write_cmos_sensor(0x3622, 0x00); // analog control
    OV5648MIPI_write_cmos_sensor(0x3630, 0xad); // analog control
    OV5648MIPI_write_cmos_sensor(0x3631, 0x00); // analog control
    OV5648MIPI_write_cmos_sensor(0x3632, 0x94); // analog control
    OV5648MIPI_write_cmos_sensor(0x3633, 0x17); // analog control
    OV5648MIPI_write_cmos_sensor(0x3634, 0x14); // analog control

    // fix EV3 issue
    OV5648MIPI_write_cmos_sensor(0x3704, 0xc0); // analog control

    OV5648MIPI_write_cmos_sensor(0x3705, 0x2a); // analog control
    OV5648MIPI_write_cmos_sensor(0x3708, 0x66); // analog control
    OV5648MIPI_write_cmos_sensor(0x3709, 0x52); // analog control
    OV5648MIPI_write_cmos_sensor(0x370b, 0x23); // analog control
    OV5648MIPI_write_cmos_sensor(0x370c, 0xc3); // analog control
    OV5648MIPI_write_cmos_sensor(0x370d, 0x00); // analog control
    OV5648MIPI_write_cmos_sensor(0x370e, 0x00); // analog control
    OV5648MIPI_write_cmos_sensor(0x371c, 0x07); // analog control
    OV5648MIPI_write_cmos_sensor(0x3739, 0xd2); // analog control
    OV5648MIPI_write_cmos_sensor(0x373c, 0x00);

    OV5648MIPI_write_cmos_sensor(0x3800, 0x00); // xstart = 0
    OV5648MIPI_write_cmos_sensor(0x3801, 0x00); // xstart
    OV5648MIPI_write_cmos_sensor(0x3802, 0x00); // ystart = 0
    OV5648MIPI_write_cmos_sensor(0x3803, 0x00); // ystart
    OV5648MIPI_write_cmos_sensor(0x3804, 0x0a); // xend = 2623
    OV5648MIPI_write_cmos_sensor(0x3805, 0x3f); // yend
    OV5648MIPI_write_cmos_sensor(0x3806, 0x07); // yend = 1955
    OV5648MIPI_write_cmos_sensor(0x3807, 0xa3); // yend
    OV5648MIPI_write_cmos_sensor(0x3808, 0x05); // x output size = 1296
    OV5648MIPI_write_cmos_sensor(0x3809, 0x10); // x output size
    OV5648MIPI_write_cmos_sensor(0x380a, 0x03); // y output size = 972
    OV5648MIPI_write_cmos_sensor(0x380b, 0xcc); // y output size
    OV5648MIPI_write_cmos_sensor(0x380c, 0x0b); // hts = 2816
    OV5648MIPI_write_cmos_sensor(0x380d, 0x00); // hts
    OV5648MIPI_write_cmos_sensor(0x380e, 0x03); // vts = 992
    OV5648MIPI_write_cmos_sensor(0x380f, 0xe0); // vts

    OV5648MIPI_write_cmos_sensor(0x3810, 0x00); // isp x win = 8
    OV5648MIPI_write_cmos_sensor(0x3811, 0x08); // isp x win
    OV5648MIPI_write_cmos_sensor(0x3812, 0x00); // isp y win = 4
    OV5648MIPI_write_cmos_sensor(0x3813, 0x04); // isp y win
    OV5648MIPI_write_cmos_sensor(0x3814, 0x31); // x inc
    OV5648MIPI_write_cmos_sensor(0x3815, 0x31); // y inc
    OV5648MIPI_write_cmos_sensor(0x3817, 0x00); // hsync start

    // Horizontal binning
    OV5648MIPI_write_cmos_sensor(0x3820, 0x08); // flip off, v bin off
    OV5648MIPI_write_cmos_sensor(0x3821, 0x07); // mirror on, h bin on

    OV5648MIPI_write_cmos_sensor(0x3826, 0x03);
    OV5648MIPI_write_cmos_sensor(0x3829, 0x00);
    OV5648MIPI_write_cmos_sensor(0x382b, 0x0b);
    OV5648MIPI_write_cmos_sensor(0x3830, 0x00);
    OV5648MIPI_write_cmos_sensor(0x3836, 0x00);
    OV5648MIPI_write_cmos_sensor(0x3837, 0x00);
    OV5648MIPI_write_cmos_sensor(0x3838, 0x00);
    OV5648MIPI_write_cmos_sensor(0x3839, 0x04);
    OV5648MIPI_write_cmos_sensor(0x383a, 0x00);
    OV5648MIPI_write_cmos_sensor(0x383b, 0x01);

    OV5648MIPI_write_cmos_sensor(0x3b00, 0x00); // strobe off
    OV5648MIPI_write_cmos_sensor(0x3b02, 0x08); // shutter delay
    OV5648MIPI_write_cmos_sensor(0x3b03, 0x00); // shutter delay
    OV5648MIPI_write_cmos_sensor(0x3b04, 0x04); // frex_exp
    OV5648MIPI_write_cmos_sensor(0x3b05, 0x00); // frex_exp
    OV5648MIPI_write_cmos_sensor(0x3b06, 0x04);
    OV5648MIPI_write_cmos_sensor(0x3b07, 0x08); // frex inv
    OV5648MIPI_write_cmos_sensor(0x3b08, 0x00); // frex exp req
    OV5648MIPI_write_cmos_sensor(0x3b09, 0x02); // frex end option
    OV5648MIPI_write_cmos_sensor(0x3b0a, 0x04); // frex rst length
    OV5648MIPI_write_cmos_sensor(0x3b0b, 0x00); // frex strobe width
    OV5648MIPI_write_cmos_sensor(0x3b0c, 0x3d); // frex strobe width
    OV5648MIPI_write_cmos_sensor(0x3f01, 0x0d);
    OV5648MIPI_write_cmos_sensor(0x3f0f, 0xf5);

    OV5648MIPI_write_cmos_sensor(0x4000, 0x89); // blc enable
    OV5648MIPI_write_cmos_sensor(0x4001, 0x02); // blc start line
    OV5648MIPI_write_cmos_sensor(0x4002, 0x45); // blc auto, reset frame number = 5
    OV5648MIPI_write_cmos_sensor(0x4004, 0x02); // black line number
    OV5648MIPI_write_cmos_sensor(0x4005, 0x18); // blc normal freeze
    OV5648MIPI_write_cmos_sensor(0x4006, 0x08);
    OV5648MIPI_write_cmos_sensor(0x4007, 0x10);
    OV5648MIPI_write_cmos_sensor(0x4008, 0x00);
    OV5648MIPI_write_cmos_sensor(0x4300, 0xf8);
    OV5648MIPI_write_cmos_sensor(0x4303, 0xff);
    OV5648MIPI_write_cmos_sensor(0x4304, 0x00);
    OV5648MIPI_write_cmos_sensor(0x4307, 0xff);
    OV5648MIPI_write_cmos_sensor(0x4520, 0x00);
    OV5648MIPI_write_cmos_sensor(0x4521, 0x00);
    OV5648MIPI_write_cmos_sensor(0x4511, 0x22);

    //update DPC settings
    OV5648MIPI_write_cmos_sensor(0x5780, 0xfc);
    OV5648MIPI_write_cmos_sensor(0x5781, 0x1f);
    OV5648MIPI_write_cmos_sensor(0x5782, 0x03);
    OV5648MIPI_write_cmos_sensor(0x5786, 0x20);
    OV5648MIPI_write_cmos_sensor(0x5787, 0x40);
    OV5648MIPI_write_cmos_sensor(0x5788, 0x08);
    OV5648MIPI_write_cmos_sensor(0x5789, 0x08);
    OV5648MIPI_write_cmos_sensor(0x578a, 0x02);
    OV5648MIPI_write_cmos_sensor(0x578b, 0x01);
    OV5648MIPI_write_cmos_sensor(0x578c, 0x01);
    OV5648MIPI_write_cmos_sensor(0x578d, 0x0c);
    OV5648MIPI_write_cmos_sensor(0x578e, 0x02);
    OV5648MIPI_write_cmos_sensor(0x578f, 0x01);
    OV5648MIPI_write_cmos_sensor(0x5790, 0x01);

    OV5648MIPI_write_cmos_sensor(0x4800, 0x14); // MIPI line sync enable

    OV5648MIPI_write_cmos_sensor(0x481f, 0x3c); // MIPI clk prepare min
    OV5648MIPI_write_cmos_sensor(0x4826, 0x00); // MIPI hs prepare min
    OV5648MIPI_write_cmos_sensor(0x4837, 0x18); // MIPI global timing
    OV5648MIPI_write_cmos_sensor(0x4b00, 0x06);
    OV5648MIPI_write_cmos_sensor(0x4b01, 0x0a);
    OV5648MIPI_write_cmos_sensor(0x5000, 0xff); // bpc on, wpc on
    OV5648MIPI_write_cmos_sensor(0x5001, 0x00); // awb disable
    OV5648MIPI_write_cmos_sensor(0x5002, 0x41); // win enable, awb gain enable
    OV5648MIPI_write_cmos_sensor(0x5003, 0x0a); // buf en, bin auto en
    OV5648MIPI_write_cmos_sensor(0x5004, 0x00); // size man off
    OV5648MIPI_write_cmos_sensor(0x5043, 0x00);
    OV5648MIPI_write_cmos_sensor(0x5013, 0x00);
    OV5648MIPI_write_cmos_sensor(0x501f, 0x03); // ISP output data
    OV5648MIPI_write_cmos_sensor(0x503d, 0x00); // test pattern off
    OV5648MIPI_write_cmos_sensor(0x5180, 0x08); // manual wb gain on
    OV5648MIPI_write_cmos_sensor(0x5a00, 0x08);
    OV5648MIPI_write_cmos_sensor(0x5b00, 0x01);
    OV5648MIPI_write_cmos_sensor(0x5b01, 0x40);
    OV5648MIPI_write_cmos_sensor(0x5b02, 0x00);
    OV5648MIPI_write_cmos_sensor(0x5b03, 0xf0);

    //OV5648MIPI_write_cmos_sensor(0x350b, 0x80); // gain = 8x
    OV5648MIPI_write_cmos_sensor(0x4837, 0x17); // MIPI global timing

    OV5648MIPI_write_cmos_sensor(0x0100, 0x01); // wake up from software sleep

//OTP Code Start
/*
#ifdef OV5648MIPI_USE_OTP
    //wb otp update
    update_otp();

#endif
*/
//OTP Code End


#ifdef OV5648MIPI_DRIVER_TRACE
    SENSORDB("Exit!");
#endif
}   /*  OV5648MIPI_Sensor_Init  */


static void OV5648MIPI_Preview_Setting(void)
{
#ifdef OV5648MIPI_DRIVER_TRACE
    SENSORDB("Enter!");
#endif


    /********************************************************
       *
       *   1296x972 30fps 2 lane MIPI 420Mbps/lane
       *
       ********************************************************/

    OV5648MIPI_write_cmos_sensor(0x0100, 0x00); //Stream Off

    //OV5648MIPI_write_cmos_sensor(0x3500, 0x00); // exposure [19:16]
    //OV5648MIPI_write_cmos_sensor(0x3501, 0x3d); // exposure
    //OV5648MIPI_write_cmos_sensor(0x3502, 0x00); // exposure
    OV5648MIPI_write_cmos_sensor(0x3708, 0x66);

    OV5648MIPI_write_cmos_sensor(0x3709, 0x52);
    OV5648MIPI_write_cmos_sensor(0x370c, 0xc3);
    OV5648MIPI_write_cmos_sensor(0x3800, 0x00); // x start = 0
    OV5648MIPI_write_cmos_sensor(0x3801, 0x00); // x start
    OV5648MIPI_write_cmos_sensor(0x3802, 0x00); // y start = 0
    OV5648MIPI_write_cmos_sensor(0x3803, 0x00); // y start
    OV5648MIPI_write_cmos_sensor(0x3804, 0x0a); // xend = 2623
    OV5648MIPI_write_cmos_sensor(0x3805, 0x3f); // xend
    OV5648MIPI_write_cmos_sensor(0x3806, 0x07); // yend = 1955
    OV5648MIPI_write_cmos_sensor(0x3807, 0xa3); // yend
    OV5648MIPI_write_cmos_sensor(0x3808, 0x05); // x output size = 1296
    OV5648MIPI_write_cmos_sensor(0x3809, 0x10); // x output size
    OV5648MIPI_write_cmos_sensor(0x380a, 0x03); // y output size = 972
    OV5648MIPI_write_cmos_sensor(0x380b, 0xcc); // y output size


    //OV5648MIPI_write_cmos_sensor(0x380c, 0x0b); // hts = 2816
    //OV5648MIPI_write_cmos_sensor(0x380d, 0x00); // hts
    //OV5648MIPI_write_cmos_sensor(0x380e, 0x03); // vts = 992
    //OV5648MIPI_write_cmos_sensor(0x380f, 0xE0); // vts
    OV5648MIPI_write_cmos_sensor(0x380c, ((OV5648MIPI_PV_PERIOD_PIXEL_NUMS >> 8) & 0xFF)); // hts = 2816
    OV5648MIPI_write_cmos_sensor(0x380d, (OV5648MIPI_PV_PERIOD_PIXEL_NUMS & 0xFF));        // hts
    OV5648MIPI_write_cmos_sensor(0x380e, ((OV5648MIPI_PV_PERIOD_LINE_NUMS >> 8) & 0xFF));  // vts = 992
    OV5648MIPI_write_cmos_sensor(0x380f, (OV5648MIPI_PV_PERIOD_LINE_NUMS & 0xFF));         // vts

    OV5648MIPI_write_cmos_sensor(0x3810, 0x00); // isp x win = 8
    OV5648MIPI_write_cmos_sensor(0x3811, 0x08); // isp x win
    OV5648MIPI_write_cmos_sensor(0x3812, 0x00); // isp y win = 4
    OV5648MIPI_write_cmos_sensor(0x3813, 0x04); // isp y win
    OV5648MIPI_write_cmos_sensor(0x3814, 0x31); // x inc
    OV5648MIPI_write_cmos_sensor(0x3815, 0x31); // y inc
    OV5648MIPI_write_cmos_sensor(0x3817, 0x00); // hsync start


    /********************************************************
       *
       *   0x3820[2] ISP Vertical flip
       *   0x3820[1] Sensor Vertical flip
       *
       *   0x3821[2] ISP Horizontal mirror
       *   0x3821[1] Sensor Horizontal mirror
       *
       *   ISP and Sensor flip or mirror register bit should be the same!!
       *
       ********************************************************/
    OV5648MIPI_write_cmos_sensor(0x3820,((OV5648MIPI_read_cmos_sensor(0x3820) & 0x06) | 0x08));//bin off
    OV5648MIPI_write_cmos_sensor(0x3821,((OV5648MIPI_read_cmos_sensor(0x3821) & 0x06) | 0x01));//bin on


    OV5648MIPI_write_cmos_sensor(0x4004, 0x02); // black line number
    //OV5648MIPI_write_cmos_sensor(0x4005, 0x1a); // blc normal freeze
    OV5648MIPI_write_cmos_sensor(0x4005, 0x18); // blc normal freeze
    //OV5648MIPI_write_cmos_sensor(0x4050, 0x37); // blc normal freeze
    OV5648MIPI_write_cmos_sensor(0x4050, 0x6e); // blc normal freeze
    OV5648MIPI_write_cmos_sensor(0x4051, 0x8f); // blc normal freeze

    //OV5648MIPI_write_cmos_sensor(0x350b, 0x80); // gain = 8x
    OV5648MIPI_write_cmos_sensor(0x4837, 0x17); // MIPI global timing

    OV5648MIPI_write_cmos_sensor(0x0100, 0x01); //Stream On

#ifdef OV5648MIPI_DRIVER_TRACE
    SENSORDB("Exit!");
#endif
}   /*  OV5648MIPI_Preview_Setting  */


static void OV5648MIPI_Capture_Setting(void)
{
#ifdef OV5648MIPI_DRIVER_TRACE
    SENSORDB("Enter!");
#endif

    /********************************************************
       *
       *   2592x1944 15fps 2 lane MIPI 420Mbps/lane
       *
       ********************************************************/

    OV5648MIPI_write_cmos_sensor(0x0100, 0x00); //Stream Off

    //OV5648MIPI_write_cmos_sensor(0x3500, 0x00); // exposure [19:16]
    //OV5648MIPI_write_cmos_sensor(0x3501, 0x7b); // exposure
    //OV5648MIPI_write_cmos_sensor(0x3502, 0x00); // exposure
    OV5648MIPI_write_cmos_sensor(0x3708, 0x63);

    OV5648MIPI_write_cmos_sensor(0x3709, 0x12);
    OV5648MIPI_write_cmos_sensor(0x370c, 0xc0);
    OV5648MIPI_write_cmos_sensor(0x3800, 0x00); // xstart = 0
    OV5648MIPI_write_cmos_sensor(0x3801, 0x00); // xstart
    OV5648MIPI_write_cmos_sensor(0x3802, 0x00); // ystart = 0
    OV5648MIPI_write_cmos_sensor(0x3803, 0x00); // ystart
    OV5648MIPI_write_cmos_sensor(0x3804, 0x0a); // xend = 2623
    OV5648MIPI_write_cmos_sensor(0x3805, 0x3f); // xend
    OV5648MIPI_write_cmos_sensor(0x3806, 0x07); // yend = 1955
    OV5648MIPI_write_cmos_sensor(0x3807, 0xa3); // yend
    OV5648MIPI_write_cmos_sensor(0x3808, 0x0a); // x output size = 2592
    OV5648MIPI_write_cmos_sensor(0x3809, 0x20); // x output size
    OV5648MIPI_write_cmos_sensor(0x380a, 0x07); // y output size = 1944
    OV5648MIPI_write_cmos_sensor(0x380b, 0x98); // y output size

    //OV5648MIPI_write_cmos_sensor(0x380c, 0x0b); // hts = 2816
    //OV5648MIPI_write_cmos_sensor(0x380d, 0x00); // hts
    //OV5648MIPI_write_cmos_sensor(0x380e, 0x07); // vts = 1984
    //OV5648MIPI_write_cmos_sensor(0x380f, 0xc0); // vts
    OV5648MIPI_write_cmos_sensor(0x380c, ((OV5648MIPI_FULL_PERIOD_PIXEL_NUMS >> 8) & 0xFF)); // hts = 2816
    OV5648MIPI_write_cmos_sensor(0x380d, (OV5648MIPI_FULL_PERIOD_PIXEL_NUMS & 0xFF));        // hts
    OV5648MIPI_write_cmos_sensor(0x380e, ((OV5648MIPI_FULL_PERIOD_LINE_NUMS >> 8) & 0xFF));  // vts = 1984
    OV5648MIPI_write_cmos_sensor(0x380f, (OV5648MIPI_FULL_PERIOD_LINE_NUMS & 0xFF));         // vts

    OV5648MIPI_write_cmos_sensor(0x3810, 0x00); // isp x win = 16
    OV5648MIPI_write_cmos_sensor(0x3811, 0x10); // isp x win
    OV5648MIPI_write_cmos_sensor(0x3812, 0x00); // isp y win = 6
    OV5648MIPI_write_cmos_sensor(0x3813, 0x06); // isp y win
    OV5648MIPI_write_cmos_sensor(0x3814, 0x11); // x inc
    OV5648MIPI_write_cmos_sensor(0x3815, 0x11); // y inc
    OV5648MIPI_write_cmos_sensor(0x3817, 0x00); // hsync start


    /********************************************************
       *
       *   0x3820[2] ISP Vertical flip
       *   0x3820[1] Sensor Vertical flip
       *
       *   0x3821[2] ISP Horizontal mirror
       *   0x3821[1] Sensor Horizontal mirror
       *
       *   ISP and Sensor flip or mirror register bit should be the same!!
       *
       ********************************************************/
    OV5648MIPI_write_cmos_sensor(0x3820,((OV5648MIPI_read_cmos_sensor(0x3820) & 0x06) | 0x40));//bin off, bits3 must be 0
    OV5648MIPI_write_cmos_sensor(0x3821,((OV5648MIPI_read_cmos_sensor(0x3821) & 0x06) | 0x00));//bin off


    OV5648MIPI_write_cmos_sensor(0x4004, 0x04); // black line number
    //0x4005[1]: 0 normal freeze 1 blc always update
    OV5648MIPI_write_cmos_sensor(0x4005, 0x18); // blc normal freeze

    //OV5648MIPI_write_cmos_sensor(0x350b, 0x40); // gain = 4x
    OV5648MIPI_write_cmos_sensor(0x4837, 0x17); // MIPI global timing

    OV5648MIPI_write_cmos_sensor(0x0100, 0x01); //Stream On

#ifdef OV5648MIPI_DRIVER_TRACE
    SENSORDB("Exit!");
#endif
}   /*  OV5648MIPI_Capture_Setting  */


/*************************************************************************
* FUNCTION
*   OV5648MIPIOpen
*
* DESCRIPTION
*   This function initialize the registers of CMOS sensor
*
* PARAMETERS
*   None
*
* RETURNS
*   None
*
* GLOBALS AFFECTED
*
*************************************************************************/
extern unsigned int g_para_version;
extern char g_para_model[32];
extern unsigned int gDrvIndex;
UINT32 OV5648MIPIOpen(void)
{
    kal_uint16 sensor_id = 0;

    // check if sensor ID correct
    sensor_id=((OV5648MIPI_read_cmos_sensor(0x300A) << 8) | OV5648MIPI_read_cmos_sensor(0x300B));
    SENSORDB("sensor_id = 0x%x ", sensor_id);

    if (sensor_id != OV5648MIPI_SENSOR_ID){
        return ERROR_SENSOR_CONNECT_FAIL;
    }

    /* initail sequence write in  */
    OV5648MIPI_Sensor_Init();

#ifdef OV5648_WB_OTP
    update_otp();
#endif
	SENSORDB("Module ID: 0x%x ", module_id);  // sunny's module id is 0x01

#ifdef OV5648_WB_OTP
    /*Check OEM Partition status,  0: No oerm partition data*/
    if(g_para_version == 0)
    {
    	/* if you want to distiguish module vendor, do it here through  module_id */
    	if(module_id != 0x01){
        sensor_id = 0xFFFFFFFF;
    	SENSORDB("This is not sunny ov5648: module_id = 0x%x ", module_id);
        return ERROR_SENSOR_CONNECT_FAIL;
        }
    	//sensor_id += module_id;
    	SENSORDB("This is  sunny ov5648, sensorID = 0x%x ", sensor_id);
    	/* if you want to distiguish module vendor, do it here through  module_id */
    }
#endif


    spin_lock(&ov5648mipi_drv_lock);
    OV5648MIPIDuringTestPattern = KAL_FALSE;
    OV5648MIPIAutoFlickerMode = KAL_FALSE;
    OV5648MIPI_sensor.ov5648mipi_sensor_mode = OV5648MIPI_SENSOR_MODE_INIT;
    OV5648MIPI_sensor.shutter = 0x3D0;
    OV5648MIPI_sensor.gain = 0x100;
    OV5648MIPI_sensor.pclk = OV5648MIPI_PREVIEW_CLK;
    OV5648MIPI_sensor.frame_length = OV5648MIPI_PV_PERIOD_LINE_NUMS;
    OV5648MIPI_sensor.line_length = OV5648MIPI_PV_PERIOD_PIXEL_NUMS;
    OV5648MIPI_sensor.dummy_pixel = 0;
    OV5648MIPI_sensor.dummy_line = 0;
    spin_unlock(&ov5648mipi_drv_lock);

    return ERROR_NONE;
}   /*  OV5648MIPIOpen  */


/*************************************************************************
* FUNCTION
*   OV5648GetSensorID
*
* DESCRIPTION
*   This function get the sensor ID
*
* PARAMETERS
*   *sensorID : return the sensor ID
*
* RETURNS
*   None
*
* GLOBALS AFFECTED
*
*************************************************************************/
UINT32 OV5648GetSensorID(UINT32 *sensorID)
{
    // check if sensor ID correct
    *sensorID=((OV5648MIPI_read_cmos_sensor(0x300A) << 8) | OV5648MIPI_read_cmos_sensor(0x300B));

    SENSORDB("Sensor ID: 0x%x ", *sensorID);

    if (*sensorID != OV5648MIPI_SENSOR_ID) {
        // if Sensor ID is not correct, Must set *sensorID to 0xFFFFFFFF
        *sensorID = 0xFFFFFFFF;
        return ERROR_SENSOR_CONNECT_FAIL;
    }


	/* if you want to distiguish module vendor, do it here through	module_id */
#ifdef OV5648_WB_OTP
    OV5648MIPI_Sensor_Init();
	update_otp();
#endif
    /*Check OEM Partition status,  0: No oerm partition data*/
    if(g_para_version == 0)
    {
        #ifdef OV5648_WB_OTP
        if(module_id != 0x01){
            *sensorID = 0xFFFFFFFF;
            SENSORDB("This is not Sunny ov5648: module_id = 0x%x ", module_id);
            return ERROR_SENSOR_CONNECT_FAIL;
        }
        #endif
    }
    else
    {
        SENSORDB("OV5648(index:%x), ParaName=%s\n",gDrvIndex >> 16, g_para_model);
        if(0 == strcmp("BOARD_D3062", g_para_model))/*Only for Model "BOARD_D3062"*/
        {
            if((gDrvIndex >> 16)!= 0x01)
            {
                *sensorID = 0xFFFFFFFF;
                SENSORDB("This is not truly ov5648: module_id = 0x%x ", gDrvIndex >> 16);
                return ERROR_SENSOR_CONNECT_FAIL;
            }
        }
        else{
            /* GPIO_MAIN_CAM_ID_PIN	AC11(GPIO17) 0x80000011*/
            /* Sunny(1), Truly(0)*/
            unsigned int hw_id;
            hw_id=mt_get_gpio_in(GPIO_MAIN_CAM_ID_PIN); // GPIO 19
            mdelay(1);
            SENSORDB("OV5648 hw_id:%x \n",hw_id);
            if(hw_id != 0x01){
                *sensorID = 0xFFFFFFFF;
                SENSORDB("This is not Sunny ov5648: module_id = 0x%x ", hw_id);
                return ERROR_SENSOR_CONNECT_FAIL;
            }
        }
    }
	/* if you want to distiguish module vendor, do it here through	module_id */
    //S*sensorID += module_id;
    SENSORDB("This is  Sunny ov5648, sensorID = 0x%x ", *sensorID);

    return ERROR_NONE;
}

/*************************************************************************
* FUNCTION
*   OV5648MIPIClose
*
* DESCRIPTION
*
*
* PARAMETERS
*   None
*
* RETURNS
*   None
*
* GLOBALS AFFECTED
*
*************************************************************************/
UINT32 OV5648MIPIClose(void)
{

    /*No Need to implement this function*/

    return ERROR_NONE;
}   /*  OV5648MIPIClose  */


/*************************************************************************
* FUNCTION
* OV5648MIPIPreview
*
* DESCRIPTION
*   This function start the sensor preview.
*
* PARAMETERS
*   *image_window : address pointer of pixel numbers in one period of HSYNC
*  *sensor_config_data : address pointer of line numbers in one period of VSYNC
*
* RETURNS
*   None
*
* GLOBALS AFFECTED
*
*************************************************************************/
UINT32 OV5648MIPIPreview(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
                      MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
    SENSORDB("Enter!");

    OV5648MIPI_Preview_Setting();

    spin_lock(&ov5648mipi_drv_lock);
    OV5648MIPI_sensor.ov5648mipi_sensor_mode = OV5648MIPI_SENSOR_MODE_PREVIEW;
    OV5648MIPI_sensor.pvPclk = OV5648MIPI_PREVIEW_CLK;
    OV5648MIPI_sensor.video_mode = KAL_FALSE;
    OV5648MIPI_sensor.line_length = OV5648MIPI_PV_PERIOD_PIXEL_NUMS;
    OV5648MIPI_sensor.frame_length = OV5648MIPI_PV_PERIOD_LINE_NUMS;
    OV5648MIPI_sensor.dummy_pixel = 0;
    OV5648MIPI_sensor.dummy_line = 0;
    spin_unlock(&ov5648mipi_drv_lock);

    SENSORDB("Exit!");

    return ERROR_NONE;
}   /*  OV5648MIPIPreview   */


UINT32 OV5648MIPIVideo(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
                      MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
    SENSORDB("Enter!");

    OV5648MIPI_Preview_Setting();

    spin_lock(&ov5648mipi_drv_lock);
    OV5648MIPI_sensor.ov5648mipi_sensor_mode = OV5648MIPI_SENSOR_MODE_VIDEO;
    OV5648MIPI_sensor.videoPclk = OV5648MIPI_VIDEO_CLK;
    OV5648MIPI_sensor.video_mode = KAL_TRUE;
    OV5648MIPI_sensor.line_length = OV5648MIPI_PV_PERIOD_PIXEL_NUMS;
    OV5648MIPI_sensor.frame_length = OV5648MIPI_PV_PERIOD_LINE_NUMS;
    OV5648MIPI_sensor.dummy_pixel = 0;
    OV5648MIPI_sensor.dummy_line = 0;
    spin_unlock(&ov5648mipi_drv_lock);

    SENSORDB("Exit!");

    return ERROR_NONE;
}   /*  OV5648MIPIPreview   */


UINT32 OV5648MIPIZSDPreview(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
                      MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
    SENSORDB("Enter!");

    OV5648MIPI_Capture_Setting();

    spin_lock(&ov5648mipi_drv_lock);
    OV5648MIPI_sensor.ov5648mipi_sensor_mode = OV5648MIPI_SENSOR_MODE_CAPTURE;
    OV5648MIPI_sensor.capPclk = OV5648MIPI_CAPTURE_CLK;
    OV5648MIPI_sensor.video_mode = KAL_FALSE;
    OV5648MIPI_sensor.line_length = OV5648MIPI_FULL_PERIOD_PIXEL_NUMS;
    OV5648MIPI_sensor.frame_length = OV5648MIPI_FULL_PERIOD_LINE_NUMS;
    OV5648MIPI_sensor.dummy_pixel = 0;
    OV5648MIPI_sensor.dummy_line = 0;
    spin_unlock(&ov5648mipi_drv_lock);

    SENSORDB("Exit!");

    return ERROR_NONE;
}   /*  OV5648MIPIZSDPreview   */

/*************************************************************************
* FUNCTION
*   OV5648MIPICapture
*
* DESCRIPTION
*   This function setup the CMOS sensor in capture MY_OUTPUT mode
*
* PARAMETERS
*
* RETURNS
*   None
*
* GLOBALS AFFECTED
*
*************************************************************************/
UINT32 OV5648MIPICapture(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
                          MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
    SENSORDB("Enter!");

    if(OV5648MIPI_SENSOR_MODE_CAPTURE != OV5648MIPI_sensor.ov5648mipi_sensor_mode)
    {
        OV5648MIPI_Capture_Setting();
    }

    spin_lock(&ov5648mipi_drv_lock);
    OV5648MIPI_sensor.ov5648mipi_sensor_mode = OV5648MIPI_SENSOR_MODE_CAPTURE;
    OV5648MIPI_sensor.capPclk = OV5648MIPI_CAPTURE_CLK;
    OV5648MIPI_sensor.video_mode = KAL_FALSE;
    OV5648MIPIAutoFlickerMode = KAL_FALSE;
    OV5648MIPI_sensor.line_length = OV5648MIPI_FULL_PERIOD_PIXEL_NUMS;
    OV5648MIPI_sensor.frame_length = OV5648MIPI_FULL_PERIOD_LINE_NUMS;
    OV5648MIPI_sensor.dummy_pixel = 0;
    OV5648MIPI_sensor.dummy_line = 0;
    spin_unlock(&ov5648mipi_drv_lock);

    SENSORDB("Exit!");

    return ERROR_NONE;
}   /* OV5648MIPI_Capture() */

UINT32 OV5648MIPIGetResolution(MSDK_SENSOR_RESOLUTION_INFO_STRUCT *pSensorResolution)
{
#ifdef OV5648MIPI_DRIVER_TRACE
    SENSORDB("Enter");
#endif

    pSensorResolution->SensorFullWidth = OV5648MIPI_IMAGE_SENSOR_FULL_WIDTH;
    pSensorResolution->SensorFullHeight = OV5648MIPI_IMAGE_SENSOR_FULL_HEIGHT;

    pSensorResolution->SensorPreviewWidth = OV5648MIPI_IMAGE_SENSOR_PV_WIDTH;
    pSensorResolution->SensorPreviewHeight = OV5648MIPI_IMAGE_SENSOR_PV_HEIGHT;

    pSensorResolution->SensorVideoWidth = OV5648MIPI_IMAGE_SENSOR_PV_WIDTH;
    pSensorResolution->SensorVideoHeight = OV5648MIPI_IMAGE_SENSOR_PV_HEIGHT;

    return ERROR_NONE;
}   /*  OV5648MIPIGetResolution  */

UINT32 OV5648MIPIGetInfo(MSDK_SCENARIO_ID_ENUM ScenarioId,
                      MSDK_SENSOR_INFO_STRUCT *pSensorInfo,
                      MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData)
{
#ifdef OV5648MIPI_DRIVER_TRACE
    SENSORDB("ScenarioId = %d", ScenarioId);
#endif

    switch(ScenarioId)
    {
        case MSDK_SCENARIO_ID_CAMERA_ZSD:
            pSensorInfo->SensorPreviewResolutionX = OV5648MIPI_IMAGE_SENSOR_FULL_WIDTH; /* not use */
            pSensorInfo->SensorPreviewResolutionY = OV5648MIPI_IMAGE_SENSOR_FULL_HEIGHT; /* not use */
            pSensorInfo->SensorCameraPreviewFrameRate = 15; /* not use */
            break;

        default:
            pSensorInfo->SensorPreviewResolutionX = OV5648MIPI_IMAGE_SENSOR_PV_WIDTH; /* not use */
            pSensorInfo->SensorPreviewResolutionY = OV5648MIPI_IMAGE_SENSOR_PV_HEIGHT; /* not use */
            pSensorInfo->SensorCameraPreviewFrameRate = 30; /* not use */
            break;
    }

    pSensorInfo->SensorFullResolutionX = OV5648MIPI_IMAGE_SENSOR_FULL_WIDTH; /* not use */
    pSensorInfo->SensorFullResolutionY = OV5648MIPI_IMAGE_SENSOR_FULL_HEIGHT; /* not use */

    pSensorInfo->SensorVideoFrameRate = 30; /* not use */
    pSensorInfo->SensorStillCaptureFrameRate= 15; /* not use */
    pSensorInfo->SensorWebCamCaptureFrameRate= 15; /* not use */

    pSensorInfo->SensorClockPolarity = SENSOR_CLOCK_POLARITY_LOW;
    pSensorInfo->SensorClockFallingPolarity = SENSOR_CLOCK_POLARITY_LOW; /* not use */
    pSensorInfo->SensorHsyncPolarity = SENSOR_CLOCK_POLARITY_LOW; // inverse with datasheet
    pSensorInfo->SensorVsyncPolarity = SENSOR_CLOCK_POLARITY_LOW;
    pSensorInfo->SensorInterruptDelayLines = 4; /* not use */
    pSensorInfo->SensorResetActiveHigh = FALSE; /* not use */
    pSensorInfo->SensorResetDelayCount = 5; /* not use */

    pSensorInfo->SensroInterfaceType = SENSOR_INTERFACE_TYPE_MIPI;

    pSensorInfo->SensorOutputDataFormat = OV5648MIPI_COLOR_FORMAT;

    pSensorInfo->CaptureDelayFrame = 2;
    pSensorInfo->PreviewDelayFrame = 2;
    pSensorInfo->VideoDelayFrame = 2;

    pSensorInfo->SensorMasterClockSwitch = 0; /* not use */
    pSensorInfo->SensorDrivingCurrent = ISP_DRIVING_2MA;

    pSensorInfo->AEShutDelayFrame = 0;          /* The frame of setting shutter default 0 for TG int */
    pSensorInfo->AESensorGainDelayFrame = 0;    /* The frame of setting sensor gain */
    pSensorInfo->AEISPGainDelayFrame = 2;

    switch (ScenarioId)
    {
        case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
        case MSDK_SCENARIO_ID_CAMERA_ZSD:
            pSensorInfo->SensorClockFreq = 24;
            pSensorInfo->SensorClockDividCount = 3; /* not use */
            pSensorInfo->SensorClockRisingCount = 0;
            pSensorInfo->SensorClockFallingCount = 2; /* not use */
            pSensorInfo->SensorPixelClockCount = 3; /* not use */
            pSensorInfo->SensorDataLatchCount = 2; /* not use */
            pSensorInfo->SensorGrabStartX = OV5648MIPI_FULL_START_X;
            pSensorInfo->SensorGrabStartY = OV5648MIPI_FULL_START_Y;

            pSensorInfo->SensorMIPILaneNumber = SENSOR_MIPI_2_LANE;
            pSensorInfo->MIPIDataLowPwr2HighSpeedTermDelayCount = 0;
            pSensorInfo->MIPIDataLowPwr2HighSpeedSettleDelayCount = 14;
            pSensorInfo->MIPICLKLowPwr2HighSpeedTermDelayCount = 0;
            pSensorInfo->SensorWidthSampling = 0;  // 0 is default 1x
            pSensorInfo->SensorHightSampling = 0;   // 0 is default 1x
            pSensorInfo->SensorPacketECCOrder = 1;

            break;
        default:
            pSensorInfo->SensorClockFreq = 24;
            pSensorInfo->SensorClockDividCount = 3; /* not use */
            pSensorInfo->SensorClockRisingCount = 0;
            pSensorInfo->SensorClockFallingCount = 2; /* not use */
            pSensorInfo->SensorPixelClockCount = 3; /* not use */
            pSensorInfo->SensorDataLatchCount = 2; /* not use */
            pSensorInfo->SensorGrabStartX = OV5648MIPI_PV_START_X;
            pSensorInfo->SensorGrabStartY = OV5648MIPI_PV_START_Y;

            pSensorInfo->SensorMIPILaneNumber = SENSOR_MIPI_2_LANE;
            pSensorInfo->MIPIDataLowPwr2HighSpeedTermDelayCount = 0;
            pSensorInfo->MIPIDataLowPwr2HighSpeedSettleDelayCount = 14;
            pSensorInfo->MIPICLKLowPwr2HighSpeedTermDelayCount = 0;
            pSensorInfo->SensorWidthSampling = 0;  // 0 is default 1x
            pSensorInfo->SensorHightSampling = 0;   // 0 is default 1x
            pSensorInfo->SensorPacketECCOrder = 1;

            break;
    }

    return ERROR_NONE;
}   /*  OV5648MIPIGetInfo  */


UINT32 OV5648MIPIControl(MSDK_SCENARIO_ID_ENUM ScenarioId, MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *pImageWindow,
                      MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData)
{
    SENSORDB("ScenarioId = %d", ScenarioId);

    mCurrentScenarioId =ScenarioId;

    switch (ScenarioId)
    {
        case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
            OV5648MIPIPreview(pImageWindow, pSensorConfigData);
            break;
        case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
            OV5648MIPIVideo(pImageWindow, pSensorConfigData);
            break;
        case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
            OV5648MIPICapture(pImageWindow, pSensorConfigData);
            break;
        case MSDK_SCENARIO_ID_CAMERA_ZSD:
            OV5648MIPIZSDPreview(pImageWindow, pSensorConfigData);
            break;
        default:
            SENSORDB("Error ScenarioId setting");
            return ERROR_INVALID_SCENARIO_ID;
    }

    return ERROR_NONE;
}   /* OV5648MIPIControl() */



UINT32 OV5648MIPISetVideoMode(UINT16 u2FrameRate)
{
    kal_uint32 MIN_Frame_length = 0, frameRate = 0, extralines = 0;

    SENSORDB("u2FrameRate = %d ", u2FrameRate);

    // SetVideoMode Function should fix framerate
    if(u2FrameRate < 5){
        // Dynamic frame rate
        return ERROR_NONE;
    }

    if(OV5648MIPI_sensor.ov5648mipi_sensor_mode == OV5648MIPI_SENSOR_MODE_VIDEO)//video ScenarioId recording
    {
        if(OV5648MIPIAutoFlickerMode == KAL_TRUE)
        {
            if (u2FrameRate==30)
                frameRate= 285;
            else if(u2FrameRate==15)
                frameRate= 146;
            else
                frameRate=u2FrameRate*10;

            MIN_Frame_length = (OV5648MIPI_sensor.videoPclk)/(OV5648MIPI_PV_PERIOD_PIXEL_NUMS + OV5648MIPI_sensor.dummy_pixel)/frameRate*10;
        }
        else
            MIN_Frame_length = (OV5648MIPI_sensor.videoPclk)/(OV5648MIPI_PV_PERIOD_PIXEL_NUMS + OV5648MIPI_sensor.dummy_pixel)/u2FrameRate;

        if((MIN_Frame_length <= OV5648MIPI_PV_PERIOD_LINE_NUMS))
        {
            MIN_Frame_length = OV5648MIPI_PV_PERIOD_LINE_NUMS;
            SENSORDB("[OV5648SetVideoMode]current fps = %d\n", (OV5648MIPI_sensor.videoPclk)  /(OV5648MIPI_PV_PERIOD_PIXEL_NUMS)/OV5648MIPI_PV_PERIOD_LINE_NUMS);
        }
        SENSORDB("[OV5648SetVideoMode]current fps (10 base)= %d\n", (OV5648MIPI_sensor.videoPclk)*10/(OV5648MIPI_PV_PERIOD_PIXEL_NUMS + OV5648MIPI_sensor.dummy_pixel)/MIN_Frame_length);
        extralines = MIN_Frame_length - OV5648MIPI_PV_PERIOD_LINE_NUMS;

        spin_lock(&ov5648mipi_drv_lock);
        OV5648MIPI_sensor.dummy_pixel = 0;//define dummy pixels and lines
        OV5648MIPI_sensor.dummy_line = extralines ;
        spin_unlock(&ov5648mipi_drv_lock);

        OV5648MIPI_Set_Dummy(OV5648MIPI_sensor.dummy_pixel, OV5648MIPI_sensor.dummy_line);
    }
    else if(OV5648MIPI_sensor.ov5648mipi_sensor_mode == OV5648MIPI_SENSOR_MODE_CAPTURE)
    {
        SENSORDB("-------[OV5648SetVideoMode]ZSD???---------\n");
        if(OV5648MIPIAutoFlickerMode == KAL_TRUE)
        {
            if (u2FrameRate==15)
                frameRate= 146;
            else
                frameRate=u2FrameRate*10;

            MIN_Frame_length = (OV5648MIPI_sensor.capPclk) /(OV5648MIPI_FULL_PERIOD_PIXEL_NUMS + OV5648MIPI_sensor.dummy_pixel)/frameRate*10;
        }
        else
            MIN_Frame_length = (OV5648MIPI_sensor.capPclk) /(OV5648MIPI_FULL_PERIOD_PIXEL_NUMS + OV5648MIPI_sensor.dummy_pixel)/u2FrameRate;

        if((MIN_Frame_length <=OV5648MIPI_FULL_PERIOD_LINE_NUMS))
        {
            MIN_Frame_length = OV5648MIPI_FULL_PERIOD_LINE_NUMS;
            SENSORDB("[OV5648SetVideoMode]current fps = %d\n", (OV5648MIPI_sensor.capPclk) /(OV5648MIPI_FULL_PERIOD_PIXEL_NUMS)/OV5648MIPI_FULL_PERIOD_LINE_NUMS);

        }
        SENSORDB("[OV5648SetVideoMode]current fps (10 base)= %d\n", (OV5648MIPI_sensor.capPclk)*10/(OV5648MIPI_FULL_PERIOD_PIXEL_NUMS + OV5648MIPI_sensor.dummy_pixel)/MIN_Frame_length);

        extralines = MIN_Frame_length - OV5648MIPI_FULL_PERIOD_LINE_NUMS;

        spin_lock(&ov5648mipi_drv_lock);
        OV5648MIPI_sensor.dummy_pixel = 0;//define dummy pixels and lines
        OV5648MIPI_sensor.dummy_line= extralines ;
        spin_unlock(&ov5648mipi_drv_lock);

        OV5648MIPI_Set_Dummy(OV5648MIPI_sensor.dummy_pixel, OV5648MIPI_sensor.dummy_line);
    }
    SENSORDB("[OV5648SetVideoMode]MIN_Frame_length=%d, OV5648MIPI_sensor.dummy_line=%d\n", MIN_Frame_length, OV5648MIPI_sensor.dummy_line);

    return ERROR_NONE;
}

UINT32 OV5648MIPISetAutoFlickerMode(kal_bool bEnable, UINT16 u2FrameRate)
{
    SENSORDB("bEnable = %d, u2FrameRate = %d ", bEnable, u2FrameRate);

    if(bEnable){
        spin_lock(&ov5648mipi_drv_lock);
        OV5648MIPIAutoFlickerMode = KAL_TRUE;
        spin_unlock(&ov5648mipi_drv_lock);

    }else{ //Cancel Auto flick
        spin_lock(&ov5648mipi_drv_lock);
        OV5648MIPIAutoFlickerMode = KAL_FALSE;
        spin_unlock(&ov5648mipi_drv_lock);
    }

    return ERROR_NONE;
}


UINT32 OV5648MIPISetMaxFramerateByScenario(MSDK_SCENARIO_ID_ENUM scenarioId, MUINT32 frameRate)
{
    kal_uint32 pclk;
    kal_int16 dummyLine;
    kal_uint16 lineLength,frameHeight;

    SENSORDB("scenarioId = %d, frame rate = %d\n", scenarioId, frameRate);

    switch (scenarioId) {
        case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
            pclk = OV5648MIPI_PREVIEW_CLK;
            lineLength = OV5648MIPI_PV_PERIOD_PIXEL_NUMS;
            frameHeight = (10 * pclk)/frameRate/lineLength;
            dummyLine = frameHeight - OV5648MIPI_PV_PERIOD_LINE_NUMS;
            OV5648MIPI_Set_Dummy(0, dummyLine);
            break;
        case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
            pclk = OV5648MIPI_VIDEO_CLK;
            lineLength = OV5648MIPI_VIDEO_PERIOD_PIXEL_NUMS;
            frameHeight = (10 * pclk)/frameRate/lineLength;
            dummyLine = frameHeight - OV5648MIPI_VIDEO_PERIOD_LINE_NUMS;
            OV5648MIPI_Set_Dummy(0, dummyLine);
            break;
        case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
        case MSDK_SCENARIO_ID_CAMERA_ZSD:
            pclk = OV5648MIPI_CAPTURE_CLK;
            lineLength = OV5648MIPI_FULL_PERIOD_PIXEL_NUMS;
            frameHeight = (10 * pclk)/frameRate/lineLength;
            dummyLine = frameHeight - OV5648MIPI_FULL_PERIOD_LINE_NUMS;
            OV5648MIPI_Set_Dummy(0, dummyLine);
            break;
        case MSDK_SCENARIO_ID_CAMERA_3D_PREVIEW: //added
            break;
        case MSDK_SCENARIO_ID_CAMERA_3D_VIDEO:
            break;
        case MSDK_SCENARIO_ID_CAMERA_3D_CAPTURE: //added
            break;
        default:
            break;
    }
    return ERROR_NONE;
}


UINT32 OV5648MIPIGetDefaultFramerateByScenario(MSDK_SCENARIO_ID_ENUM scenarioId, MUINT32 *pframeRate)
{
    SENSORDB("scenarioId = %d \n", scenarioId);

    switch (scenarioId) {
        case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
        case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
             *pframeRate = 300;
             break;
        case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
        case MSDK_SCENARIO_ID_CAMERA_ZSD:
             *pframeRate = 150;
            break;
        case MSDK_SCENARIO_ID_CAMERA_3D_PREVIEW: //added
        case MSDK_SCENARIO_ID_CAMERA_3D_VIDEO:
        case MSDK_SCENARIO_ID_CAMERA_3D_CAPTURE: //added
             *pframeRate = 300;
            break;
        default:
            break;
    }

    return ERROR_NONE;
}

UINT32 OV5648MIPI_SetTestPatternMode(kal_bool bEnable)
{
    SENSORDB("bEnable: %d", bEnable);

    if(bEnable)
    {
        spin_lock(&ov5648mipi_drv_lock);
        OV5648MIPIDuringTestPattern = KAL_TRUE;
        spin_unlock(&ov5648mipi_drv_lock);

        // 0x503D[8]: 1 enable,  0 disable
        // 0x503D[1:0]; 00 Color bar, 01 Random Data, 10 Square
        OV5648MIPI_write_cmos_sensor(0x503D, 0x80);
    }
    else
    {
        spin_lock(&ov5648mipi_drv_lock);
        OV5648MIPIDuringTestPattern = KAL_FALSE;
        spin_unlock(&ov5648mipi_drv_lock);

        // 0x503D[8]: 1 enable,  0 disable
        // 0x503D[1:0]; 00 Color bar, 01 Random Data, 10 Square
        OV5648MIPI_write_cmos_sensor(0x503D, 0x00);
    }

    return ERROR_NONE;
}


UINT32 OV5648MIPIFeatureControl(MSDK_SENSOR_FEATURE_ENUM FeatureId,
                             UINT8 *pFeaturePara,UINT32 *pFeatureParaLen)
{
    UINT16 *pFeatureReturnPara16=(UINT16 *) pFeaturePara;
    UINT16 *pFeatureData16=(UINT16 *) pFeaturePara;
    UINT32 *pFeatureReturnPara32=(UINT32 *) pFeaturePara;
    UINT32 *pFeatureData32=(UINT32 *) pFeaturePara;
    //UINT32 OV5648MIPISensorRegNumber;
    //UINT32 i;
    //PNVRAM_SENSOR_DATA_STRUCT pSensorDefaultData=(PNVRAM_SENSOR_DATA_STRUCT) pFeaturePara;
    //MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData=(MSDK_SENSOR_CONFIG_STRUCT *) pFeaturePara;
    MSDK_SENSOR_REG_INFO_STRUCT *pSensorRegData=(MSDK_SENSOR_REG_INFO_STRUCT *) pFeaturePara;
    //MSDK_SENSOR_GROUP_INFO_STRUCT *pSensorGroupInfo=(MSDK_SENSOR_GROUP_INFO_STRUCT *) pFeaturePara;
    //MSDK_SENSOR_ITEM_INFO_STRUCT *pSensorItemInfo=(MSDK_SENSOR_ITEM_INFO_STRUCT *) pFeaturePara;
    //MSDK_SENSOR_ENG_INFO_STRUCT *pSensorEngInfo=(MSDK_SENSOR_ENG_INFO_STRUCT *) pFeaturePara;

#ifdef OV5648MIPI_DRIVER_TRACE
//    SENSORDB("FeatureId = %d", FeatureId);
#endif

    switch (FeatureId)
    {
        case SENSOR_FEATURE_GET_RESOLUTION:
            *pFeatureReturnPara16++=OV5648MIPI_IMAGE_SENSOR_FULL_WIDTH;
            *pFeatureReturnPara16=OV5648MIPI_IMAGE_SENSOR_FULL_HEIGHT;
            *pFeatureParaLen=4;
            break;
        case SENSOR_FEATURE_GET_PERIOD:
            switch(mCurrentScenarioId)
            {
                case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
                case MSDK_SCENARIO_ID_CAMERA_ZSD:
                    *pFeatureReturnPara16++ = OV5648MIPI_sensor.line_length;
                    *pFeatureReturnPara16 = OV5648MIPI_sensor.frame_length;
                    *pFeatureParaLen=4;
                    break;
                default:
                    *pFeatureReturnPara16++ = OV5648MIPI_sensor.line_length;
                    *pFeatureReturnPara16 = OV5648MIPI_sensor.frame_length;
                    *pFeatureParaLen=4;
                    break;
            }
            break;
        case SENSOR_FEATURE_GET_PIXEL_CLOCK_FREQ:
            switch(mCurrentScenarioId)
            {
                case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
                case MSDK_SCENARIO_ID_CAMERA_ZSD:
                    *pFeatureReturnPara32 = OV5648MIPI_CAPTURE_CLK;
                    *pFeatureParaLen=4;
                    break;
                default:
                    *pFeatureReturnPara32 = OV5648MIPI_PREVIEW_CLK;
                    *pFeatureParaLen=4;
                    break;
            }
            break;
        case SENSOR_FEATURE_SET_ESHUTTER:
            set_OV5648MIPI_shutter(*pFeatureData16);
            break;
        case SENSOR_FEATURE_SET_NIGHTMODE:
            OV5648MIPI_night_mode((BOOL) *pFeatureData16);
            break;
        case SENSOR_FEATURE_SET_GAIN:
            OV5648MIPI_SetGain((UINT16) *pFeatureData16);
            break;
        case SENSOR_FEATURE_SET_FLASHLIGHT:
            break;
        case SENSOR_FEATURE_SET_ISP_MASTER_CLOCK_FREQ:
            break;
        case SENSOR_FEATURE_SET_REGISTER:
            OV5648MIPI_write_cmos_sensor(pSensorRegData->RegAddr, pSensorRegData->RegData);
            break;
        case SENSOR_FEATURE_GET_REGISTER:
            pSensorRegData->RegData = OV5648MIPI_read_cmos_sensor(pSensorRegData->RegAddr);
            break;
        case SENSOR_FEATURE_SET_CCT_REGISTER:
            memcpy(&OV5648MIPI_sensor.eng.cct, pFeaturePara, sizeof(OV5648MIPI_sensor.eng.cct));
            break;
            break;
        case SENSOR_FEATURE_GET_CCT_REGISTER:
            if (*pFeatureParaLen >= sizeof(OV5648MIPI_sensor.eng.cct) + sizeof(kal_uint32))
            {
              *((kal_uint32 *)pFeaturePara++) = sizeof(OV5648MIPI_sensor.eng.cct);
              memcpy(pFeaturePara, &OV5648MIPI_sensor.eng.cct, sizeof(OV5648MIPI_sensor.eng.cct));
            }
            break;
        case SENSOR_FEATURE_SET_ENG_REGISTER:
            memcpy(&OV5648MIPI_sensor.eng.reg, pFeaturePara, sizeof(OV5648MIPI_sensor.eng.reg));
            break;
        case SENSOR_FEATURE_GET_ENG_REGISTER:
            if (*pFeatureParaLen >= sizeof(OV5648MIPI_sensor.eng.reg) + sizeof(kal_uint32))
            {
              *((kal_uint32 *)pFeaturePara++) = sizeof(OV5648MIPI_sensor.eng.reg);
              memcpy(pFeaturePara, &OV5648MIPI_sensor.eng.reg, sizeof(OV5648MIPI_sensor.eng.reg));
            }
        case SENSOR_FEATURE_GET_REGISTER_DEFAULT:
            ((PNVRAM_SENSOR_DATA_STRUCT)pFeaturePara)->Version = NVRAM_CAMERA_SENSOR_FILE_VERSION;
            ((PNVRAM_SENSOR_DATA_STRUCT)pFeaturePara)->SensorId = OV5648MIPI_SENSOR_ID;
            memcpy(((PNVRAM_SENSOR_DATA_STRUCT)pFeaturePara)->SensorEngReg, &OV5648MIPI_sensor.eng.reg, sizeof(OV5648MIPI_sensor.eng.reg));
            memcpy(((PNVRAM_SENSOR_DATA_STRUCT)pFeaturePara)->SensorCCTReg, &OV5648MIPI_sensor.eng.cct, sizeof(OV5648MIPI_sensor.eng.cct));
            *pFeatureParaLen = sizeof(NVRAM_SENSOR_DATA_STRUCT);
            break;
        case SENSOR_FEATURE_GET_CONFIG_PARA:
            memcpy(pFeaturePara, &OV5648MIPI_sensor.cfg_data, sizeof(OV5648MIPI_sensor.cfg_data));
            *pFeatureParaLen = sizeof(OV5648MIPI_sensor.cfg_data);
            break;
        case SENSOR_FEATURE_CAMERA_PARA_TO_SENSOR:
             OV5648MIPI_camera_para_to_sensor();
            break;
        case SENSOR_FEATURE_SENSOR_TO_CAMERA_PARA:
            OV5648MIPI_sensor_to_camera_para();
            break;
        case SENSOR_FEATURE_GET_GROUP_COUNT:
            OV5648MIPI_get_sensor_group_count((kal_uint32 *)pFeaturePara);
            *pFeatureParaLen = 4;
            break;
        case SENSOR_FEATURE_GET_GROUP_INFO:
            OV5648MIPI_get_sensor_group_info((MSDK_SENSOR_GROUP_INFO_STRUCT *)pFeaturePara);
            *pFeatureParaLen = sizeof(MSDK_SENSOR_GROUP_INFO_STRUCT);
            break;
        case SENSOR_FEATURE_GET_ITEM_INFO:
            OV5648MIPI_get_sensor_item_info((MSDK_SENSOR_ITEM_INFO_STRUCT *)pFeaturePara);
            *pFeatureParaLen = sizeof(MSDK_SENSOR_ITEM_INFO_STRUCT);
            break;
        case SENSOR_FEATURE_SET_ITEM_INFO:
            OV5648MIPI_set_sensor_item_info((MSDK_SENSOR_ITEM_INFO_STRUCT *)pFeaturePara);
            *pFeatureParaLen = sizeof(MSDK_SENSOR_ITEM_INFO_STRUCT);
            break;
        case SENSOR_FEATURE_GET_ENG_INFO:
            memcpy(pFeaturePara, &OV5648MIPI_sensor.eng_info, sizeof(OV5648MIPI_sensor.eng_info));
            *pFeatureParaLen = sizeof(OV5648MIPI_sensor.eng_info);
            break;
        case SENSOR_FEATURE_GET_LENS_DRIVER_ID:
            // get the lens driver ID from EEPROM or just return LENS_DRIVER_ID_DO_NOT_CARE
            // if EEPROM does not exist in camera module.
            *pFeatureReturnPara32=LENS_DRIVER_ID_DO_NOT_CARE;
            *pFeatureParaLen=4;
            break;
        case SENSOR_FEATURE_SET_VIDEO_MODE:
            OV5648MIPISetVideoMode(*pFeatureData16);
            break;
        case SENSOR_FEATURE_CHECK_SENSOR_ID:
            OV5648GetSensorID(pFeatureReturnPara32);
            break;
        case SENSOR_FEATURE_SET_AUTO_FLICKER_MODE:
            OV5648MIPISetAutoFlickerMode((BOOL)*pFeatureData16,*(pFeatureData16+1));
            break;
        case SENSOR_FEATURE_SET_MAX_FRAME_RATE_BY_SCENARIO:
            OV5648MIPISetMaxFramerateByScenario((MSDK_SCENARIO_ID_ENUM)*pFeatureData32, *(pFeatureData32+1));
            break;
        case SENSOR_FEATURE_GET_DEFAULT_FRAME_RATE_BY_SCENARIO:
            OV5648MIPIGetDefaultFramerateByScenario((MSDK_SCENARIO_ID_ENUM)*pFeatureData32, (MUINT32 *)(*(pFeatureData32+1)));
            break;
        case SENSOR_FEATURE_SET_TEST_PATTERN:
            OV5648MIPI_SetTestPatternMode((BOOL)*pFeatureData16);
            break;
        case SENSOR_FEATURE_GET_TEST_PATTERN_CHECKSUM_VALUE://for factory mode auto testing
            *pFeatureReturnPara32= OV5648MIPI_TEST_PATTERN_CHECKSUM;
            *pFeatureParaLen=4;
             break;
        case SENSOR_FEATURE_SET_MIRROR_FLIP:
            //SENSORDB("[SENSOR_FEATURE_SET_MIRROR_FLIP]Mirror:%d, Flip:%d\n", *pFeatureData32,*(pFeatureData32+1));
            OV5648MIPI_Set_Mirror_Flip(*pFeatureData32, *(pFeatureData32+1));
            break;
        default:
            break;
    }

    return ERROR_NONE;
}   /*  OV5648MIPIFeatureControl()  */
SENSOR_FUNCTION_STRUCT  SensorFuncOV5648MIPI=
{
    OV5648MIPIOpen,
    OV5648MIPIGetInfo,
    OV5648MIPIGetResolution,
    OV5648MIPIFeatureControl,
    OV5648MIPIControl,
    OV5648MIPIClose
};

UINT32 OV5648MIPISensorInit(PSENSOR_FUNCTION_STRUCT *pfFunc)
{
    /* To Do : Check Sensor status here */
    if (pfFunc!=NULL)
        *pfFunc=&SensorFuncOV5648MIPI;

    return ERROR_NONE;
}   /*  OV5648MIPISensorInit  */
