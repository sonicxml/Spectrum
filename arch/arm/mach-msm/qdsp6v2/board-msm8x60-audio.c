/* Copyright (c) 2010-2011, Code Aurora Forum. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 *
 */

#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/gpio.h>
#include <linux/delay.h>
#include <linux/debugfs.h>
#include <linux/mfd/pmic8058.h>
#include <linux/pmic8058-othc.h>
#include <linux/mfd/pmic8901.h>
#include <linux/mfd/msm-adie-codec.h>
#include <linux/regulator/pmic8058-regulator.h>
#include <linux/regulator/pmic8901-regulator.h>
#include <linux/regulator/consumer.h>
#include <linux/regulator/machine.h>

#include <mach/qdsp6v2/audio_dev_ctl.h>
#include <mach/qdsp6v2/apr_audio.h>
#include <mach/board_lge.h>
#include <mach/mpp.h>
#include <asm/mach-types.h>
#include <asm/uaccess.h>

#include "snddev_icodec.h"
#include "snddev_ecodec.h"
#include "timpani_profile_8x60.h"
#include "snddev_hdmi.h"
#include "snddev_mi2s.h"
#include "snddev_virtual.h"

#ifdef CONFIG_DEBUG_FS
static struct dentry *debugfs_hsed_config;
static void snddev_hsed_config_modify_setting(int type);
static void snddev_hsed_config_restore_setting(void);
#endif

#ifdef CONFIG_LGE_AUDIO //dongsung.shin
extern void set_amp_path(int num);
#endif
/* GPIO_CLASS_D0_EN */
#define SNDDEV_GPIO_CLASS_D0_EN 227

/* GPIO_CLASS_D1_EN */
#define SNDDEV_GPIO_CLASS_D1_EN 229

#define SNDDEV_GPIO_MIC2_ANCR_SEL 294

#ifdef CONFIG_LGE_AUDIO
#define SNDDEV_GPIO_EAR_MIC_EN 58
#endif

#define SNDDEV_GPIO_MIC1_ANCL_SEL 295

static struct resource msm_cdcclk_ctl_resources[] = {
	{
		.name   = "msm_snddev_tx_mclk",
		.start  = 108,
		.end    = 108,
		.flags  = IORESOURCE_IO,
	},
	{
		.name   = "msm_snddev_rx_mclk",
		.start  = 109,
		.end    = 109,
		.flags  = IORESOURCE_IO,
	},
};

static struct platform_device msm_cdcclk_ctl_device = {
	.name   = "msm_cdcclk_ctl",
	.num_resources  = ARRAY_SIZE(msm_cdcclk_ctl_resources),
	.resource       = msm_cdcclk_ctl_resources,
};

static struct resource msm_aux_pcm_resources[] = {

	{
		.name   = "aux_pcm_dout",
		.start  = 111,
		.end    = 111,
		.flags  = IORESOURCE_IO,
	},
	{
		.name   = "aux_pcm_din",
		.start  = 112,
		.end    = 112,
		.flags  = IORESOURCE_IO,
	},
	{
		.name   = "aux_pcm_syncout",
		.start  = 113,
		.end    = 113,
		.flags  = IORESOURCE_IO,
	},
	{
		.name   = "aux_pcm_clkin_a",
		.start  = 114,
		.end    = 114,
		.flags  = IORESOURCE_IO,
	},
};

static struct platform_device msm_aux_pcm_device = {
	.name   = "msm_aux_pcm",
	.num_resources  = ARRAY_SIZE(msm_aux_pcm_resources),
	.resource       = msm_aux_pcm_resources,
};

#ifdef LGE_NOT_SUPPORT_DEVICE
static struct resource msm_mi2s_gpio_resources[] = {

	{
		.name   = "mi2s_ws",
		.start  = 101,
		.end    = 101,
		.flags  = IORESOURCE_IO,
	},
	{
		.name   = "mi2s_sclk",
		.start  = 102,
		.end    = 102,
		.flags  = IORESOURCE_IO,
	},
	{
		.name   = "mi2s_mclk",
		.start  = 103,
		.end    = 103,
		.flags  = IORESOURCE_IO,
	},
	{
		.name   = "fm_mi2s_sd",
		.start  = 107,
		.end    = 107,
		.flags  = IORESOURCE_IO,
	},
};

static struct platform_device msm_mi2s_device = {
	.name		= "msm_mi2s",
	.num_resources	= ARRAY_SIZE(msm_mi2s_gpio_resources),
	.resource	= msm_mi2s_gpio_resources,
};
#endif

/* Must be same size as msm_icodec_gpio_resources */
static int msm_icodec_gpio_defaults[] = {
	0,
	0,
};

static struct resource msm_icodec_gpio_resources[] = {
	{
		.name   = "msm_icodec_speaker_left",
		.start  = SNDDEV_GPIO_CLASS_D0_EN,
		.end    = SNDDEV_GPIO_CLASS_D0_EN,
		.flags  = IORESOURCE_IO,
	},
	{
		.name   = "msm_icodec_speaker_right",
		.start  = SNDDEV_GPIO_CLASS_D1_EN,
		.end    = SNDDEV_GPIO_CLASS_D1_EN,
		.flags  = IORESOURCE_IO,
	},
};

static struct platform_device msm_icodec_gpio_device = {
	.name   = "msm_icodec_gpio",
	.num_resources  = ARRAY_SIZE(msm_icodec_gpio_resources),
	.resource       = msm_icodec_gpio_resources,
	.dev = { .platform_data = &msm_icodec_gpio_defaults },
};

static int msm_qt_icodec_gpio_defaults[] = {
	0,
};

static struct resource msm_qt_icodec_gpio_resources[] = {
	{
		.name   = "msm_icodec_speaker_gpio",
		.start  = SNDDEV_GPIO_CLASS_D0_EN,
		.end    = SNDDEV_GPIO_CLASS_D0_EN,
		.flags  = IORESOURCE_IO,
	},
};

static struct platform_device msm_qt_icodec_gpio_device = {
	.name   = "msm_icodec_gpio",
	.num_resources  = ARRAY_SIZE(msm_qt_icodec_gpio_resources),
	.resource       = msm_qt_icodec_gpio_resources,
	.dev = { .platform_data = &msm_qt_icodec_gpio_defaults },
};

static struct regulator *s3;
static struct regulator *mvs;

static int msm_snddev_enable_dmic_power(void)
{
	int ret;

	s3 = regulator_get(NULL, "8058_s3");
	if (IS_ERR(s3)) {
		ret = -EBUSY;
		goto fail_get_s3;
	}

	ret = regulator_set_voltage(s3, 1800000, 1800000);
	if (ret) {
		pr_err("%s: error setting voltage\n", __func__);
		goto fail_s3;
	}

	ret = regulator_enable(s3);
	if (ret) {
		pr_err("%s: error enabling regulator\n", __func__);
		goto fail_s3;
	}

	mvs = regulator_get(NULL, "8901_mvs0");
	if (IS_ERR(mvs))
		goto fail_mvs0_get;

	ret = regulator_enable(mvs);
	if (ret) {
		pr_err("%s: error setting regulator\n", __func__);
		goto fail_mvs0_enable;
	}
	return ret;

fail_mvs0_enable:
	regulator_put(mvs);
	mvs = NULL;
fail_mvs0_get:
	regulator_disable(s3);
fail_s3:
	regulator_put(s3);
	s3 = NULL;
fail_get_s3:
	return ret;
}

static void msm_snddev_disable_dmic_power(void)
{
	int ret;

	if (mvs) {
		ret = regulator_disable(mvs);
		if (ret < 0)
			pr_err("%s: error disabling vreg mvs\n", __func__);
		regulator_put(mvs);
		mvs = NULL;
	}

	if (s3) {
		ret = regulator_disable(s3);
		if (ret < 0)
			pr_err("%s: error disabling regulator s3\n", __func__);
		regulator_put(s3);
		s3 = NULL;
	}
}

static struct regulator *l11;

static int msm_snddev_enable_qt_dmic_power(void)
{
	int ret;

	l11 = regulator_get(NULL, "8058_l11");
	if (IS_ERR(l11))
		return -EBUSY;

	ret = regulator_set_voltage(l11, 1500000, 1500000);
	if (ret) {
		pr_err("%s: error setting regulator\n", __func__);
		goto fail_l11;
	}
	ret = regulator_enable(l11);
	if (ret) {
		pr_err("%s: error enabling regulator\n", __func__);
		goto fail_l11;
	}
	return 0;

fail_l11:
	regulator_put(l11);
	l11 = NULL;
	return ret;
}


static void msm_snddev_disable_qt_dmic_power(void)
{
	int ret;

	if (l11) {
		ret = regulator_disable(l11);
		if (ret < 0)
			pr_err("%s: error disabling regulator l11\n", __func__);
		regulator_put(l11);
		l11 = NULL;
	}
}

#ifdef CONFIG_LGE_AUDIO
static void config_ear_mic_en_gpio(int enable)
{
	if (enable) {
		gpio_set_value_cansleep(SNDDEV_GPIO_EAR_MIC_EN, 1);
	} else {
		gpio_set_value_cansleep(SNDDEV_GPIO_EAR_MIC_EN, 0);
		
	}
	pr_debug("disable Ear MiC GPIO %s, val = %d\n", __func__,enable);		
}
	
#else
#define PM8901_MPP_3 (2) /* PM8901 MPP starts from 0 */
static int config_class_d1_gpio(int enable)
{
	int rc;

	if (enable) {
		rc = gpio_request(SNDDEV_GPIO_CLASS_D1_EN, "CLASSD1_EN");
		if (rc) {
			pr_err("%s: spkr pamp gpio %d request"
			"failed\n", __func__, SNDDEV_GPIO_CLASS_D1_EN);
			return rc;
		}
		gpio_direction_output(SNDDEV_GPIO_CLASS_D1_EN, 1);
	} else {
		gpio_set_value_cansleep(SNDDEV_GPIO_CLASS_D1_EN, 0);
		gpio_free(SNDDEV_GPIO_CLASS_D1_EN);
	}
	return 0;
}

static int config_class_d0_gpio(int enable)
{
	int rc;

	if (enable) {
		rc = pm8901_mpp_config_digital_out(PM8901_MPP_3,
			PM8901_MPP_DIG_LEVEL_MSMIO, 1);

		if (rc) {
			pr_err("%s: CLASS_D0_EN failed\n", __func__);
			return rc;
		}

		rc = gpio_request(SNDDEV_GPIO_CLASS_D0_EN, "CLASSD0_EN");

		if (rc) {
			pr_err("%s: spkr pamp gpio pm8901 mpp3 request"
			"failed\n", __func__);
			pm8901_mpp_config_digital_out(PM8901_MPP_3,
			PM8901_MPP_DIG_LEVEL_MSMIO, 0);
			return rc;
		}

		gpio_direction_output(SNDDEV_GPIO_CLASS_D0_EN, 1);
		gpio_set_value(SNDDEV_GPIO_CLASS_D0_EN, 1);

	} else {
		pm8901_mpp_config_digital_out(PM8901_MPP_3,
		PM8901_MPP_DIG_LEVEL_MSMIO, 0);
		gpio_set_value(SNDDEV_GPIO_CLASS_D0_EN, 0);
		gpio_free(SNDDEV_GPIO_CLASS_D0_EN);
	}
	return 0;
}
#endif

#if 0//def CONFIG_LGE_AUDIO //dongsung.shin
struct pm8058_gpio wca_audio_enable = {
	.direction	= PM_GPIO_DIR_OUT,
	.output_value	= 1,
	.output_buffer	= PM_GPIO_OUT_BUF_CMOS,
	.pull		= PM_GPIO_PULL_UP_30,
	.out_strength	= PM_GPIO_STRENGTH_HIGH,
	.function	= PM_GPIO_FUNC_NORMAL,
	.vin_sel	= 2,
	.inv_int_pol	= 0,
};

struct pm8058_gpio wca_audio_disable = {
	.direction	= PM_GPIO_DIR_OUT,
	.output_value	= 1,
	.output_buffer	= PM_GPIO_OUT_BUF_CMOS,
	.pull		= PM_GPIO_PULL_DN,
	.out_strength	= PM_GPIO_STRENGTH_HIGH,
	.function	= PM_GPIO_FUNC_NORMAL,
	.vin_sel	= 2,
	.inv_int_pol	= 0,
};

#endif
static atomic_t pamp_ref_cnt;

static int msm_snddev_poweramp_on(void)
{
	int rc;
#ifdef CONFIG_LGE_AUDIO //dongsung.shin
	set_amp_path(4);
	rc=0;
        return rc;
#else
	if (atomic_inc_return(&pamp_ref_cnt) > 1)
		return 0;

	pr_debug("%s: enable stereo spkr amp\n", __func__);
	rc = config_class_d0_gpio(1);
	if (rc) {
		pr_err("%s: d0 gpio configuration failed\n", __func__);
		goto config_gpio_fail;
	}
	if (!machine_is_msm8x60_qt()) {
		rc = config_class_d1_gpio(1);
		if (rc) {
			pr_err("%s: d1 gpio configuration failed\n", __func__);
			config_class_d0_gpio(0);
		}
	}
config_gpio_fail:
	return rc;
#endif
}

static void msm_snddev_poweramp_off(void)
{
#ifdef CONFIG_LGE_AUDIO //dongsung.shin
	set_amp_path(0);
#else
	if (atomic_dec_return(&pamp_ref_cnt) == 0) {
		pr_debug("%s: disable stereo spkr amp\n", __func__);
		config_class_d0_gpio(0);
		if (!machine_is_msm8x60_qt())
			config_class_d1_gpio(0);
		msleep(30);
	}
#endif
}
static int msm_snddev_headset_speaker_poweramp_on(void)
{

#ifdef CONFIG_LGE_AUDIO //dongsung.shin
	set_amp_path(5);
#endif
    return 0;
}


int  msm_snddev_headset_poweramp_on(void)
{
#ifdef CONFIG_LGE_AUDIO //dongsung.shin
	set_amp_path(2);
#endif	
    return 0;
}
/* Regulator 8058_l10 supplies regulator 8058_ncp. */
static struct regulator *snddev_reg_ncp;
static struct regulator *snddev_reg_l10;

static atomic_t preg_ref_cnt;

static int msm_snddev_voltage_on(void)
{
	int rc;
	pr_debug("%s\n", __func__);

	if (atomic_inc_return(&preg_ref_cnt) > 1)
		return 0;

#if 0 // dsshin test
	snddev_reg_l10 = regulator_get(NULL, "8058_l10");
	if (IS_ERR(snddev_reg_l10)) {
		pr_err("%s: regulator_get(%s) failed (%ld)\n", __func__,
			"l10", PTR_ERR(snddev_reg_l10));
		return -EBUSY;
	}

	rc = regulator_set_voltage(snddev_reg_l10, 2600000, 2600000);
	if (rc < 0)
		pr_err("%s: regulator_set_voltage(l10) failed (%d)\n",
			__func__, rc);

	rc = regulator_enable(snddev_reg_l10);
	if (rc < 0)
		pr_err("%s: regulator_enable(l10) failed (%d)\n", __func__, rc);
#endif
	snddev_reg_ncp = regulator_get(NULL, "8058_ncp");
	if (IS_ERR(snddev_reg_ncp)) {
		pr_err("%s: regulator_get(%s) failed (%ld)\n", __func__,
			"ncp", PTR_ERR(snddev_reg_ncp));
		return -EBUSY;
	}

	rc = regulator_set_voltage(snddev_reg_ncp, 1800000, 1800000);
	if (rc < 0) {
		pr_err("%s: regulator_set_voltage(ncp) failed (%d)\n",
			__func__, rc);
		goto regulator_fail;
	}

	rc = regulator_enable(snddev_reg_ncp);
	if (rc < 0) {
		pr_err("%s: regulator_enable(ncp) failed (%d)\n", __func__, rc);
		goto regulator_fail;
	}

	return rc;

regulator_fail:
	regulator_put(snddev_reg_ncp);
	snddev_reg_ncp = NULL;
	return rc;
}

static void msm_snddev_voltage_off(void)
{
	int rc;
	pr_debug("%s\n", __func__);

	if (!snddev_reg_ncp)
		goto done;

	if (atomic_dec_return(&preg_ref_cnt) == 0) {
		rc = regulator_disable(snddev_reg_ncp);
		if (rc < 0)
			pr_err("%s: regulator_disable(ncp) failed (%d)\n",
				__func__, rc);
		regulator_put(snddev_reg_ncp);

		snddev_reg_ncp = NULL;
	}

done:
	if (!snddev_reg_l10)
		return;

	rc = regulator_disable(snddev_reg_l10);
	if (rc < 0)
		pr_err("%s: regulator_disable(l10) failed (%d)\n",
			__func__, rc);

	regulator_put(snddev_reg_l10);

	snddev_reg_l10 = NULL;
}

static int msm_snddev_enable_amic_power(void)
{
	int ret = 0;
#ifdef CONFIG_PMIC8058_OTHC

	if (machine_is_msm8x60_fluid()) {

		ret = pm8058_micbias_enable(OTHC_MICBIAS_0,
				OTHC_SIGNAL_ALWAYS_ON);
		if (ret)
			pr_err("%s: Enabling amic power failed\n", __func__);

		ret = gpio_request(SNDDEV_GPIO_MIC2_ANCR_SEL, "MIC2_ANCR_SEL");
		if (ret) {
			pr_err("%s: spkr pamp gpio %d request failed\n",
				__func__, SNDDEV_GPIO_MIC2_ANCR_SEL);
			return ret;
		}
		gpio_direction_output(SNDDEV_GPIO_MIC2_ANCR_SEL, 0);

		ret = gpio_request(SNDDEV_GPIO_MIC1_ANCL_SEL, "MIC1_ANCL_SEL");
		if (ret) {
			pr_err("%s: mic1 ancl gpio %d request failed\n",
				__func__, SNDDEV_GPIO_MIC1_ANCL_SEL);
			gpio_free(SNDDEV_GPIO_MIC2_ANCR_SEL);
			return ret;
		}
		gpio_direction_output(SNDDEV_GPIO_MIC1_ANCL_SEL, 0);

	} else {
#ifdef CONFIG_LGE_AUDIO // dongsung.shin
		ret = pm8058_micbias_enable(OTHC_MICBIAS_0, OTHC_SIGNAL_ALWAYS_ON);
#else
		ret = pm8058_micbias_enable(OTHC_MICBIAS_2, OTHC_SIGNAL_ALWAYS_ON);
#endif
		if (ret)
			pr_err("%s: Enabling amic power failed\n", __func__);
	}
#endif
	return ret;
}

static void msm_snddev_disable_amic_power(void)
{
#ifdef CONFIG_PMIC8058_OTHC
	int ret;
	if (machine_is_msm8x60_fluid()) {
		ret = pm8058_micbias_enable(OTHC_MICBIAS_0,
				OTHC_SIGNAL_OFF);
		gpio_free(SNDDEV_GPIO_MIC1_ANCL_SEL);
		gpio_free(SNDDEV_GPIO_MIC2_ANCR_SEL);
	} else
#ifdef CONFIG_LGE_AUDIO // dongsung.shin
		ret = pm8058_micbias_enable(OTHC_MICBIAS_0, OTHC_SIGNAL_OFF);
#else
		ret = pm8058_micbias_enable(OTHC_MICBIAS_2, OTHC_SIGNAL_OFF);
#endif
	if (ret)
		pr_err("%s: Disabling amic power failed\n", __func__);
#endif
}

static int msm_snddev_enable_anc_power(void)
{
	int ret = 0;
#ifdef CONFIG_PMIC8058_OTHC
	ret = pm8058_micbias_enable(OTHC_MICBIAS_2,
		OTHC_SIGNAL_ALWAYS_ON);
	if (ret)
		pr_err("%s: Enabling anc micbias 2 failed\n", __func__);

	if (machine_is_msm8x60_fluid()) {

		ret = pm8058_micbias_enable(OTHC_MICBIAS_0,
				OTHC_SIGNAL_ALWAYS_ON);
		if (ret)
			pr_err("%s: Enabling anc micbias 0 failed\n", __func__);

		ret = gpio_request(SNDDEV_GPIO_MIC2_ANCR_SEL, "MIC2_ANCR_SEL");
		if (ret) {
			pr_err("%s: mic2 ancr gpio %d request failed\n",
				__func__, SNDDEV_GPIO_MIC2_ANCR_SEL);
			return ret;
		}
		gpio_direction_output(SNDDEV_GPIO_MIC2_ANCR_SEL, 1);

		ret = gpio_request(SNDDEV_GPIO_MIC1_ANCL_SEL, "MIC1_ANCL_SEL");
		if (ret) {
			pr_err("%s: mic1 ancl gpio %d request failed\n",
				__func__, SNDDEV_GPIO_MIC1_ANCL_SEL);
			gpio_free(SNDDEV_GPIO_MIC2_ANCR_SEL);
			return ret;
		}
		gpio_direction_output(SNDDEV_GPIO_MIC1_ANCL_SEL, 1);

	}
#endif
	return ret;
}

static void msm_snddev_disable_anc_power(void)
{
#ifdef CONFIG_PMIC8058_OTHC
	int ret;

	ret = pm8058_micbias_enable(OTHC_MICBIAS_2, OTHC_SIGNAL_OFF);

	if (machine_is_msm8x60_fluid()) {
		ret |= pm8058_micbias_enable(OTHC_MICBIAS_0,
				OTHC_SIGNAL_OFF);
		gpio_free(SNDDEV_GPIO_MIC2_ANCR_SEL);
		gpio_free(SNDDEV_GPIO_MIC1_ANCL_SEL);
	}

	if (ret)
		pr_err("%s: Disabling anc power failed\n", __func__);
#endif
}

static int msm_snddev_enable_dmic_sec_power(void)
{
	int ret;

	ret = msm_snddev_enable_dmic_power();
	if (ret) {
		pr_err("%s: Error: Enabling dmic power failed\n", __func__);
		return ret;
	}
#ifdef CONFIG_PMIC8058_OTHC
	ret = pm8058_micbias_enable(OTHC_MICBIAS_2, OTHC_SIGNAL_ALWAYS_ON);
	if (ret) {
		pr_err("%s: Error: Enabling micbias failed\n", __func__);
		msm_snddev_disable_dmic_power();
		return ret;
	}
#endif
	return 0;
}

static void msm_snddev_disable_dmic_sec_power(void)
{
	msm_snddev_disable_dmic_power();

#ifdef CONFIG_PMIC8058_OTHC
	pm8058_micbias_enable(OTHC_MICBIAS_2, OTHC_SIGNAL_OFF);
#endif
}

 int msm_snddev_enable_hdset_mic_bias(void)
{
#ifdef CONFIG_PMIC8058_OTHC
	int ret;
	pr_debug("enable %s\n", __func__);
#ifdef CONFIG_LGE_AUDIO // dongsung.shin
	ret = pm8058_micbias_enable(OTHC_MICBIAS_1, OTHC_SIGNAL_ALWAYS_ON);
	if (ret)
		pr_err("%s: Enabling Headset Mic Bias failed\n", __func__);
	config_ear_mic_en_gpio(1);
#endif	
#endif
    return 0;
}

 void msm_snddev_disable_hdset_mic_bias(void)
{
#if 0//def CONFIG_PMIC8058_OTHC
	int ret;
	pr_debug("disable %s\n", __func__);
#ifdef CONFIG_LGE_AUDIO // dongsung.shin
	ret = pm8058_micbias_enable(OTHC_MICBIAS_1, OTHC_SIGNAL_OFF);
#endif
	if (ret)
		pr_err("%s: Disabling Headset Mic Bias failed\n", __func__);
	config_ear_mic_en_gpio(0);
#endif
}

static struct adie_codec_action_unit iearpiece_48KHz_osr256_actions[] =
	EAR_PRI_MONO_8000_OSR_256;

static struct adie_codec_hwsetting_entry iearpiece_settings[] = {
	{
		.freq_plan = 48000,
		.osr = 256,
		.actions = iearpiece_48KHz_osr256_actions,
		.action_sz = ARRAY_SIZE(iearpiece_48KHz_osr256_actions),
	}
};

static struct adie_codec_dev_profile iearpiece_profile = {
	.path_type = ADIE_CODEC_RX,
	.settings = iearpiece_settings,
	.setting_sz = ARRAY_SIZE(iearpiece_settings),
};

static struct snddev_icodec_data snddev_iearpiece_data = {
	.capability = (SNDDEV_CAP_RX | SNDDEV_CAP_VOICE),
	.name = "handset_rx",
	.copp_id = 0,
	.profile = &iearpiece_profile,
	.channel_mode = 1,
	.default_sample_rate = 48000,
};

static struct platform_device msm_iearpiece_device = {
	.name = "snddev_icodec",
	.dev = { .platform_data = &snddev_iearpiece_data },
};

static struct adie_codec_action_unit imic_48KHz_osr256_actions[] =
	AMIC_PRI_MONO_OSR_256;

static struct adie_codec_hwsetting_entry imic_settings[] = {
	{
		.freq_plan = 48000,
		.osr = 256,
		.actions = imic_48KHz_osr256_actions,
		.action_sz = ARRAY_SIZE(imic_48KHz_osr256_actions),
	}
};

static struct adie_codec_dev_profile imic_profile = {
	.path_type = ADIE_CODEC_TX,
	.settings = imic_settings,
	.setting_sz = ARRAY_SIZE(imic_settings),
};

static struct snddev_icodec_data snddev_imic_data = {
	.capability = (SNDDEV_CAP_TX | SNDDEV_CAP_VOICE),
	.name = "handset_tx",
	.copp_id = 1,
	.profile = &imic_profile,
	.channel_mode = 1,
	.default_sample_rate = 48000,
	.pamp_on = msm_snddev_enable_amic_power,
	.pamp_off = msm_snddev_disable_amic_power,
};

static struct platform_device msm_imic_device = {
	.name = "snddev_icodec",
	.dev = { .platform_data = &snddev_imic_data },
};

static struct snddev_icodec_data snddev_fluid_ispkr_mic_data = {
	.capability = (SNDDEV_CAP_TX | SNDDEV_CAP_VOICE),
	.name = "speaker_mono_tx",
	.copp_id = PRIMARY_I2S_TX,
	.profile = &imic_profile,
	.channel_mode = 1,
	.default_sample_rate = 48000,
	.pamp_on = msm_snddev_enable_amic_power,
	.pamp_off = msm_snddev_disable_amic_power,
};

static struct platform_device msm_fluid_ispkr_mic_device = {
	.name = "snddev_icodec",
	.dev = { .platform_data = &snddev_fluid_ispkr_mic_data },
};


static struct adie_codec_action_unit headset_ab_cpls_48KHz_osr256_actions[] =
#if 1 //def CONFIG_LGE_AUDIO_NO_NCP_MODE
	HPH_PRI_AB_LEG_STEREO;
#else
	HEADSET_AB_CPLS_48000_OSR_256;
#endif

static struct adie_codec_hwsetting_entry headset_ab_cpls_settings[] = {
	{
		.freq_plan = 48000,
		.osr = 256,
		.actions = headset_ab_cpls_48KHz_osr256_actions,
		.action_sz = ARRAY_SIZE(headset_ab_cpls_48KHz_osr256_actions),
	}
};

static struct adie_codec_dev_profile headset_ab_cpls_profile = {
	.path_type = ADIE_CODEC_RX,
	.settings = headset_ab_cpls_settings,
	.setting_sz = ARRAY_SIZE(headset_ab_cpls_settings),
};

static struct snddev_icodec_data snddev_ihs_stereo_rx_data = {
	.capability = (SNDDEV_CAP_RX | SNDDEV_CAP_VOICE),
	.name = "headset_stereo_rx",
	.copp_id = 0,
	.profile = &headset_ab_cpls_profile,
	.channel_mode = 2,
	.default_sample_rate = 48000,
	.pamp_on = msm_snddev_headset_poweramp_on,
	.pamp_off = msm_snddev_poweramp_off,	
	.voltage_on = msm_snddev_voltage_on,
	.voltage_off = msm_snddev_voltage_off,
};

static struct platform_device msm_headset_stereo_device = {
	.name = "snddev_icodec",
	.dev = { .platform_data = &snddev_ihs_stereo_rx_data },
};

static struct adie_codec_action_unit headset_anc_48KHz_osr256_actions[] =
	ANC_HEADSET_CPLS_AMIC1_AUXL_RX1_48000_OSR_256;

static struct adie_codec_hwsetting_entry headset_anc_settings[] = {
	{
		.freq_plan = 48000,
		.osr = 256,
		.actions = headset_anc_48KHz_osr256_actions,
		.action_sz = ARRAY_SIZE(headset_anc_48KHz_osr256_actions),
	}
};

static struct adie_codec_dev_profile headset_anc_profile = {
	.path_type = ADIE_CODEC_RX,
	.settings = headset_anc_settings,
	.setting_sz = ARRAY_SIZE(headset_anc_settings),
};

static struct snddev_icodec_data snddev_anc_headset_data = {
	.capability = (SNDDEV_CAP_RX | SNDDEV_CAP_VOICE | SNDDEV_CAP_ANC),
	.name = "anc_headset_stereo_rx",
	.copp_id = PRIMARY_I2S_RX,
	.profile = &headset_anc_profile,
	.channel_mode = 2,
	.default_sample_rate = 48000,
	.pamp_on = msm_snddev_enable_anc_power,
	.pamp_off = msm_snddev_disable_anc_power,
	.voltage_on = msm_snddev_voltage_on,
	.voltage_off = msm_snddev_voltage_off,
};

static struct platform_device msm_anc_headset_device = {
	.name = "snddev_icodec",
	.dev = { .platform_data = &snddev_anc_headset_data },
};

static struct adie_codec_action_unit ispkr_stereo_48KHz_osr256_actions[] =
#ifdef CONFIG_LGE_AUDIO //dongsung.shin
#if 1//def CONFIG_LGE_AUDIO_NO_NCP_MODE
	HPH_PRI_AB_LEG_STEREO;
#else
	HEADSET_AB_CPLS_48000_OSR_256;
#endif
#else
       SPEAKER_PRI_STEREO_48000_OSR_256;
#endif

static struct adie_codec_hwsetting_entry ispkr_stereo_settings[] = {
	{
		.freq_plan = 48000,
		.osr = 256,
		.actions = ispkr_stereo_48KHz_osr256_actions,
		.action_sz = ARRAY_SIZE(ispkr_stereo_48KHz_osr256_actions),
	}
};

static struct adie_codec_dev_profile ispkr_stereo_profile = {
	.path_type = ADIE_CODEC_RX,
	.settings = ispkr_stereo_settings,
	.setting_sz = ARRAY_SIZE(ispkr_stereo_settings),
};

static struct snddev_icodec_data snddev_ispkr_stereo_data = {
	.capability = (SNDDEV_CAP_RX | SNDDEV_CAP_VOICE),
	.name = "speaker_stereo_rx",
	.copp_id = 0,
	.profile = &ispkr_stereo_profile,
	.channel_mode = 2,
	.default_sample_rate = 48000,
	.pamp_on = msm_snddev_poweramp_on,
	.pamp_off = msm_snddev_poweramp_off,
#ifdef CONFIG_LGE_AUDIO //dongsung.shin
	.voltage_on = msm_snddev_voltage_on,
	.voltage_off = msm_snddev_voltage_off,
#endif
};

static struct platform_device msm_ispkr_stereo_device = {
	.name = "snddev_icodec",
	.dev = { .platform_data = &snddev_ispkr_stereo_data },
};

#ifdef CONFIG_LGE_AUDIO

#else /*CONFIG_LGE_AUDIO*/
static struct adie_codec_action_unit idmic_mono_48KHz_osr256_actions[] =
	DMIC1_PRI_MONO_OSR_256;

static struct adie_codec_hwsetting_entry idmic_mono_settings[] = {
	{
		.freq_plan = 48000,
		.osr = 256,
		.actions = idmic_mono_48KHz_osr256_actions,
		.action_sz = ARRAY_SIZE(idmic_mono_48KHz_osr256_actions),
	}
};

static struct adie_codec_dev_profile idmic_mono_profile = {
	.path_type = ADIE_CODEC_TX,
	.settings = idmic_mono_settings,
	.setting_sz = ARRAY_SIZE(idmic_mono_settings),
};
#endif/*CONFIG_LGE_AUDIO*/

static struct snddev_icodec_data snddev_ispkr_mic_data = {
	.capability = (SNDDEV_CAP_TX | SNDDEV_CAP_VOICE),
	.name = "speaker_mono_tx",
	.copp_id = PRIMARY_I2S_TX,
#ifdef CONFIG_LGE_AUDIO
	.profile = &imic_profile,
#else
	.profile = &idmic_mono_profile,
#endif	
	.channel_mode = 1,
	.default_sample_rate = 48000,
#ifdef CONFIG_LGE_AUDIO
	.pamp_on = msm_snddev_enable_amic_power,
	.pamp_off = msm_snddev_disable_amic_power,
	
#else
	.pamp_on = msm_snddev_enable_dmic_power,
	.pamp_off = msm_snddev_disable_dmic_power,
#endif	
};

static struct platform_device msm_ispkr_mic_device = {
	.name = "snddev_icodec",
	.dev = { .platform_data = &snddev_ispkr_mic_data },
};

static struct adie_codec_action_unit iearpiece_ffa_48KHz_osr256_actions[] =
	EAR_PRI_MONO_8000_OSR_256;

static struct adie_codec_hwsetting_entry iearpiece_ffa_settings[] = {
	{
		.freq_plan = 48000,
		.osr = 256,
		.actions = iearpiece_ffa_48KHz_osr256_actions,
		.action_sz = ARRAY_SIZE(iearpiece_ffa_48KHz_osr256_actions),
	}
};

static struct adie_codec_dev_profile iearpiece_ffa_profile = {
	.path_type = ADIE_CODEC_RX,
	.settings = iearpiece_ffa_settings,
	.setting_sz = ARRAY_SIZE(iearpiece_ffa_settings),
};

static struct snddev_icodec_data snddev_iearpiece_ffa_data = {
	.capability = (SNDDEV_CAP_RX | SNDDEV_CAP_VOICE),
	.name = "handset_rx",
	.copp_id = 0,
	.profile = &iearpiece_ffa_profile,
	.channel_mode = 1,
	.default_sample_rate = 48000,
};

static struct platform_device msm_iearpiece_ffa_device = {
	.name = "snddev_icodec",
	.dev = { .platform_data = &snddev_iearpiece_ffa_data },
};

static struct snddev_icodec_data snddev_imic_ffa_data = {
	.capability = (SNDDEV_CAP_TX | SNDDEV_CAP_VOICE),
	.name = "handset_tx",
	.copp_id = PRIMARY_I2S_TX,
#ifdef CONFIG_LGE_AUDIO
	.profile = &imic_profile,
#else
	.profile = &idmic_mono_profile,
#endif	
	.channel_mode = 1,
	.default_sample_rate = 48000,
#ifdef CONFIG_LGE_AUDIO
	.pamp_on = msm_snddev_enable_amic_power,
	.pamp_off = msm_snddev_disable_amic_power,

#else
	.pamp_on = msm_snddev_enable_dmic_power,
	.pamp_off = msm_snddev_disable_dmic_power,
#endif	
};

static struct platform_device msm_imic_ffa_device = {
	.name = "snddev_icodec",
	.dev = { .platform_data = &snddev_imic_ffa_data },
};

static struct snddev_icodec_data snddev_qt_dual_dmic_d0_data = {
	.capability = (SNDDEV_CAP_TX | SNDDEV_CAP_VOICE),
	.name = "speaker_mono_tx",
	.copp_id = PRIMARY_I2S_TX,
#ifdef CONFIG_LGE_AUDIO
	.profile = &imic_profile,
#else
	.profile = &idmic_mono_profile,
#endif	
	.channel_mode = 1,
	.default_sample_rate = 48000,
	.pamp_on = msm_snddev_enable_qt_dmic_power,
	.pamp_off = msm_snddev_disable_qt_dmic_power,
};

static struct platform_device msm_qt_dual_dmic_d0_device = {
	.name = "snddev_icodec",
	.dev = { .platform_data = &snddev_qt_dual_dmic_d0_data },
};

static struct adie_codec_action_unit dual_mic_endfire_8KHz_osr256_actions[] =
	DMIC1_PRI_STEREO_OSR_256;

static struct adie_codec_hwsetting_entry dual_mic_endfire_settings[] = {
	{
		.freq_plan = 48000,
		.osr = 256,
		.actions = dual_mic_endfire_8KHz_osr256_actions,
		.action_sz = ARRAY_SIZE(dual_mic_endfire_8KHz_osr256_actions),
	}
};

static struct adie_codec_dev_profile dual_mic_endfire_profile = {
	.path_type = ADIE_CODEC_TX,
	.settings = dual_mic_endfire_settings,
	.setting_sz = ARRAY_SIZE(dual_mic_endfire_settings),
};

static struct snddev_icodec_data snddev_dual_mic_endfire_data = {
	.capability = (SNDDEV_CAP_TX | SNDDEV_CAP_VOICE),
	.name = "handset_dual_mic_endfire_tx",
	.copp_id = PRIMARY_I2S_TX,
	.profile = &dual_mic_endfire_profile,
	.channel_mode = 2,
	.default_sample_rate = 48000,
	.pamp_on = msm_snddev_enable_dmic_power,
	.pamp_off = msm_snddev_disable_dmic_power,
};

static struct platform_device msm_hs_dual_mic_endfire_device = {
	.name = "snddev_icodec",
	.dev = { .platform_data = &snddev_dual_mic_endfire_data },
};

static struct snddev_icodec_data snddev_dual_mic_spkr_endfire_data = {
	.capability = (SNDDEV_CAP_TX | SNDDEV_CAP_VOICE),
	.name = "speaker_dual_mic_endfire_tx",
	.copp_id = PRIMARY_I2S_TX,
	.profile = &dual_mic_endfire_profile,
	.channel_mode = 2,
	.default_sample_rate = 48000,
	.pamp_on = msm_snddev_enable_dmic_power,
	.pamp_off = msm_snddev_disable_dmic_power,
};

static struct platform_device msm_spkr_dual_mic_endfire_device = {
	.name = "snddev_icodec",
	.id = 15,
	.dev = { .platform_data = &snddev_dual_mic_spkr_endfire_data },
};

static struct adie_codec_action_unit dual_mic_broadside_8osr256_actions[] =
	HS_DMIC2_STEREO_OSR_256;

static struct adie_codec_hwsetting_entry dual_mic_broadside_settings[] = {
	{
		.freq_plan = 48000,
		.osr = 256,
		.actions = dual_mic_broadside_8osr256_actions,
		.action_sz = ARRAY_SIZE(dual_mic_broadside_8osr256_actions),
	}
};

static struct adie_codec_dev_profile dual_mic_broadside_profile = {
	.path_type = ADIE_CODEC_TX,
	.settings = dual_mic_broadside_settings,
	.setting_sz = ARRAY_SIZE(dual_mic_broadside_settings),
};

static struct snddev_icodec_data snddev_hs_dual_mic_broadside_data = {
	.capability = (SNDDEV_CAP_TX | SNDDEV_CAP_VOICE),
	.name = "handset_dual_mic_broadside_tx",
	.copp_id = PRIMARY_I2S_TX,
	.profile = &dual_mic_broadside_profile,
	.channel_mode = 2,
	.default_sample_rate = 48000,
	.pamp_on = msm_snddev_enable_dmic_sec_power,
	.pamp_off = msm_snddev_disable_dmic_sec_power,
};

static struct platform_device msm_hs_dual_mic_broadside_device = {
	.name = "snddev_icodec",
	.id = 21,
	.dev = { .platform_data = &snddev_hs_dual_mic_broadside_data },
};

static struct snddev_icodec_data snddev_spkr_dual_mic_broadside_data = {
	.capability = (SNDDEV_CAP_TX | SNDDEV_CAP_VOICE),
	.name = "speaker_dual_mic_broadside_tx",
	.copp_id = PRIMARY_I2S_TX,
	.profile = &dual_mic_broadside_profile,
	.channel_mode = 2,
	.default_sample_rate = 48000,
	.pamp_on = msm_snddev_enable_dmic_sec_power,
	.pamp_off = msm_snddev_disable_dmic_sec_power,
};

static struct platform_device msm_spkr_dual_mic_broadside_device = {
	.name = "snddev_icodec",
	.id = 18,
	.dev = { .platform_data = &snddev_spkr_dual_mic_broadside_data },
};

static struct snddev_hdmi_data snddev_hdmi_stereo_rx_data = {
	.capability = SNDDEV_CAP_RX ,
	.name = "hdmi_stereo_rx",
	.copp_id = HDMI_RX,
	.channel_mode = 0,
	.default_sample_rate = 48000,
};

static struct platform_device msm_snddev_hdmi_stereo_rx_device = {
	.name = "snddev_hdmi",
	.dev = { .platform_data = &snddev_hdmi_stereo_rx_data },
};

static struct snddev_mi2s_data snddev_mi2s_fm_tx_data = {
	.capability = SNDDEV_CAP_TX ,
	.name = "fmradio_stereo_tx",
	.copp_id = MI2S_TX,
	.channel_mode = 2, /* stereo */
	.sd_lines = MI2S_SD3, /* sd3 */
	.sample_rate = 48000,
};

static struct platform_device msm_mi2s_fm_tx_device = {
	.name = "snddev_mi2s",
	.dev = { .platform_data = &snddev_mi2s_fm_tx_data },
};

static struct snddev_mi2s_data snddev_mi2s_fm_rx_data = {
	.capability = SNDDEV_CAP_RX ,
	.name = "fmradio_stereo_rx",
	.copp_id = MI2S_RX,
	.channel_mode = 2, /* stereo */
	.sd_lines = MI2S_SD3, /* sd3 */
	.sample_rate = 48000,
};

static struct platform_device msm_mi2s_fm_rx_device = {
	.name = "snddev_mi2s",
	.id = 1,
	.dev = { .platform_data = &snddev_mi2s_fm_rx_data },
};

static struct adie_codec_action_unit iheadset_mic_tx_osr256_actions[] =
	HEADSET_AMIC2_TX_MONO_PRI_OSR_256;

static struct adie_codec_hwsetting_entry iheadset_mic_tx_settings[] = {
	{
		.freq_plan = 48000,
		.osr = 256,
		.actions = iheadset_mic_tx_osr256_actions,
		.action_sz = ARRAY_SIZE(iheadset_mic_tx_osr256_actions),
	}
};

static struct adie_codec_dev_profile iheadset_mic_profile = {
	.path_type = ADIE_CODEC_TX,
	.settings = iheadset_mic_tx_settings,
	.setting_sz = ARRAY_SIZE(iheadset_mic_tx_settings),
};

static struct snddev_icodec_data snddev_headset_mic_data = {
	.capability = (SNDDEV_CAP_TX | SNDDEV_CAP_VOICE),
	.name = "headset_mono_tx",
	.copp_id = PRIMARY_I2S_TX,
	.profile = &iheadset_mic_profile,
	.channel_mode = 1,
	.default_sample_rate = 48000,
#ifdef CONFIG_LGE_AUDIO
	.pamp_on = msm_snddev_enable_hdset_mic_bias,
	.pamp_off = msm_snddev_disable_hdset_mic_bias,
#endif	
	
};

static struct platform_device msm_headset_mic_device = {
	.name = "snddev_icodec",
	.dev = { .platform_data = &snddev_headset_mic_data },
};

static struct adie_codec_action_unit
	ihs_stereo_speaker_stereo_rx_48KHz_osr256_actions[] =
#ifdef CONFIG_LGE_AUDIO //dongsung.shin
#if 1//def CONFIG_LGE_AUDIO_NO_NCP_MODE
		HPH_PRI_AB_LEG_STEREO;
#else
		HEADSET_AB_CPLS_48000_OSR_256;
#endif
#else
	SPEAKER_HPH_AB_CPL_PRI_STEREO_48000_OSR_256;
#endif

static struct adie_codec_hwsetting_entry
	ihs_stereo_speaker_stereo_rx_settings[] = {
	{
		.freq_plan = 48000,
		.osr = 256,
		.actions = ihs_stereo_speaker_stereo_rx_48KHz_osr256_actions,
		.action_sz =
		ARRAY_SIZE(ihs_stereo_speaker_stereo_rx_48KHz_osr256_actions),
	}
};

static struct adie_codec_dev_profile ihs_stereo_speaker_stereo_rx_profile = {
	.path_type = ADIE_CODEC_RX,
	.settings = ihs_stereo_speaker_stereo_rx_settings,
	.setting_sz = ARRAY_SIZE(ihs_stereo_speaker_stereo_rx_settings),
};

static struct snddev_icodec_data snddev_ihs_stereo_speaker_stereo_rx_data = {
	.capability = (SNDDEV_CAP_RX | SNDDEV_CAP_VOICE),
	.name = "headset_stereo_speaker_stereo_rx",
	.copp_id = 0,
	.profile = &ihs_stereo_speaker_stereo_rx_profile,
	.channel_mode = 2,
	.default_sample_rate = 48000,
#ifdef CONFIG_LGE_AUDIO //dongsung.shin
	.pamp_on = msm_snddev_headset_speaker_poweramp_on,
#else
	.pamp_on = msm_snddev_poweramp_on,
#endif		
	.pamp_off = msm_snddev_poweramp_off,

	.voltage_on = msm_snddev_voltage_on,
	.voltage_off = msm_snddev_voltage_off,
};

static struct platform_device msm_ihs_stereo_speaker_stereo_rx_device = {
	.name = "snddev_icodec",
	.id = 22,
	.dev = { .platform_data = &snddev_ihs_stereo_speaker_stereo_rx_data },
};

/* define the value for BT_SCO */

static struct snddev_ecodec_data snddev_bt_sco_earpiece_data = {
	.capability = (SNDDEV_CAP_RX | SNDDEV_CAP_VOICE),
	.name = "bt_sco_rx",
	.copp_id = PCM_RX,
	.channel_mode = 1,
};

static struct snddev_ecodec_data snddev_bt_sco_mic_data = {
	.capability = (SNDDEV_CAP_TX | SNDDEV_CAP_VOICE),
	.name = "bt_sco_tx",
	.copp_id = PCM_TX,
	.channel_mode = 1,
};

struct platform_device msm_bt_sco_earpiece_device = {
	.name = "msm_snddev_ecodec",
	.dev = { .platform_data = &snddev_bt_sco_earpiece_data },
};

struct platform_device msm_bt_sco_mic_device = {
	.name = "msm_snddev_ecodec",
	.dev = { .platform_data = &snddev_bt_sco_mic_data },
};

static struct adie_codec_action_unit itty_mono_tx_actions[] =
	TTY_HEADSET_MONO_TX_OSR_256;

static struct adie_codec_hwsetting_entry itty_mono_tx_settings[] = {
	{
		.freq_plan = 48000,
		.osr = 256,
		.actions = itty_mono_tx_actions,
		.action_sz = ARRAY_SIZE(itty_mono_tx_actions),
	},
};

static struct adie_codec_dev_profile itty_mono_tx_profile = {
	.path_type = ADIE_CODEC_TX,
	.settings = itty_mono_tx_settings,
	.setting_sz = ARRAY_SIZE(itty_mono_tx_settings),
};

static struct snddev_icodec_data snddev_itty_mono_tx_data = {
	.capability = (SNDDEV_CAP_TX | SNDDEV_CAP_VOICE | SNDDEV_CAP_TTY),
	.name = "tty_headset_mono_tx",
	.copp_id = PRIMARY_I2S_TX,
	.profile = &itty_mono_tx_profile,
	.channel_mode = 1,
	.default_sample_rate = 48000,
};

static struct platform_device msm_itty_mono_tx_device = {
	.name = "snddev_icodec",
	.dev = { .platform_data = &snddev_itty_mono_tx_data },
};

static struct adie_codec_action_unit itty_mono_rx_actions[] =
	TTY_HEADSET_MONO_RX_8000_OSR_256;

static struct adie_codec_hwsetting_entry itty_mono_rx_settings[] = {
	{
		.freq_plan = 48000,
		.osr = 256,
		.actions = itty_mono_rx_actions,
		.action_sz = ARRAY_SIZE(itty_mono_rx_actions),
	},
};

static struct adie_codec_dev_profile itty_mono_rx_profile = {
	.path_type = ADIE_CODEC_RX,
	.settings = itty_mono_rx_settings,
	.setting_sz = ARRAY_SIZE(itty_mono_rx_settings),
};

static struct snddev_icodec_data snddev_itty_mono_rx_data = {
	.capability = (SNDDEV_CAP_RX | SNDDEV_CAP_VOICE | SNDDEV_CAP_TTY),
	.name = "tty_headset_mono_rx",
	.copp_id = PRIMARY_I2S_RX,
	.profile = &itty_mono_rx_profile,
	.channel_mode = 1,
	.default_sample_rate = 48000,
	.voltage_on = msm_snddev_voltage_on,
	.voltage_off = msm_snddev_voltage_off,
};

static struct platform_device msm_itty_mono_rx_device = {
	.name = "snddev_icodec",
	.dev = { .platform_data = &snddev_itty_mono_rx_data },
};

static struct adie_codec_action_unit linein_pri_actions[] =
	LINEIN_PRI_STEREO_OSR_256;

static struct adie_codec_hwsetting_entry linein_pri_settings[] = {
	{
		.freq_plan = 48000,
		.osr = 256,
		.actions = linein_pri_actions,
		.action_sz = ARRAY_SIZE(linein_pri_actions),
	},
};

static struct adie_codec_dev_profile linein_pri_profile = {
	.path_type = ADIE_CODEC_TX,
	.settings = linein_pri_settings,
	.setting_sz = ARRAY_SIZE(linein_pri_settings),
};

static struct snddev_icodec_data snddev_linein_pri_data = {
	.capability = SNDDEV_CAP_TX,
	.name = "linein_pri_tx",
	.copp_id = PRIMARY_I2S_TX,
	.profile = &linein_pri_profile,
	.channel_mode = 2,
	.default_sample_rate = 48000,
	.voltage_on = msm_snddev_voltage_on,
	.voltage_off = msm_snddev_voltage_off,
};

static struct platform_device msm_linein_pri_device = {
	.name = "snddev_icodec",
	.dev = { .platform_data = &snddev_linein_pri_data },
};

static struct adie_codec_action_unit auxpga_lp_lo_actions[] =
	LB_AUXPGA_LO_STEREO;

static struct adie_codec_hwsetting_entry auxpga_lp_lo_settings[] = {
	{
		.freq_plan = 48000,
		.osr = 256,
		.actions = auxpga_lp_lo_actions,
		.action_sz = ARRAY_SIZE(auxpga_lp_lo_actions),
	},
};

static struct adie_codec_dev_profile auxpga_lp_lo_profile = {
	.path_type = ADIE_CODEC_LB,
	.settings = auxpga_lp_lo_settings,
	.setting_sz = ARRAY_SIZE(auxpga_lp_lo_settings),
};

static struct snddev_icodec_data snddev_auxpga_lp_lo_data = {
	.capability = SNDDEV_CAP_LB,
	.name = "speaker_stereo_lb",
	.copp_id = PRIMARY_I2S_RX,
	.profile = &auxpga_lp_lo_profile,
	.channel_mode = 2,
	.default_sample_rate = 48000,
	.pamp_on = msm_snddev_poweramp_on,
	.pamp_off = msm_snddev_poweramp_off,
	.dev_vol_type = SNDDEV_DEV_VOL_ANALOG,
};

static struct platform_device msm_auxpga_lp_lo_device = {
	.name = "snddev_icodec",
	.dev = { .platform_data = &snddev_auxpga_lp_lo_data },
};

static struct adie_codec_action_unit auxpga_lp_hs_actions[] =
	LB_AUXPGA_HPH_AB_CPLS_STEREO;

static struct adie_codec_hwsetting_entry auxpga_lp_hs_settings[] = {
	{
		.freq_plan = 48000,
		.osr = 256,
		.actions = auxpga_lp_hs_actions,
		.action_sz = ARRAY_SIZE(auxpga_lp_hs_actions),
	},
};

static struct adie_codec_dev_profile auxpga_lp_hs_profile = {
	.path_type = ADIE_CODEC_LB,
	.settings = auxpga_lp_hs_settings,
	.setting_sz = ARRAY_SIZE(auxpga_lp_hs_settings),
};

static struct snddev_icodec_data snddev_auxpga_lp_hs_data = {
	.capability = SNDDEV_CAP_LB,
	.name = "hs_stereo_lb",
	.copp_id = PRIMARY_I2S_RX,
	.profile = &auxpga_lp_hs_profile,
	.channel_mode = 2,
	.default_sample_rate = 48000,
	.voltage_on = msm_snddev_voltage_on,
	.voltage_off = msm_snddev_voltage_off,
	.dev_vol_type = SNDDEV_DEV_VOL_ANALOG,
};

static struct platform_device msm_auxpga_lp_hs_device = {
	.name = "snddev_icodec",
	.dev = { .platform_data = &snddev_auxpga_lp_hs_data },
};

static struct adie_codec_action_unit ftm_headset_mono_rx_actions[] =
	HPH_PRI_AB_CPLS_MONO;

static struct adie_codec_hwsetting_entry ftm_headset_mono_rx_settings[] = {
	{
		.freq_plan = 48000,
		.osr = 256,
		.actions = ftm_headset_mono_rx_actions,
		.action_sz = ARRAY_SIZE(ftm_headset_mono_rx_actions),
	},
};

static struct adie_codec_dev_profile ftm_headset_mono_rx_profile = {
	.path_type = ADIE_CODEC_RX,
	.settings = ftm_headset_mono_rx_settings,
	.setting_sz = ARRAY_SIZE(ftm_headset_mono_rx_settings),
};

static struct snddev_icodec_data ftm_headset_mono_rx_data = {
	.capability = SNDDEV_CAP_RX,
	.name = "ftm_headset_mono_rx",
	.copp_id = PRIMARY_I2S_RX,
	.profile = &ftm_headset_mono_rx_profile,
	.channel_mode = 1,
	.default_sample_rate = 48000,
	.voltage_on = msm_snddev_voltage_on,
	.voltage_off = msm_snddev_voltage_off,
};

static struct platform_device ftm_headset_mono_rx_device = {
	.name = "snddev_icodec",
	.dev = { .platform_data = &ftm_headset_mono_rx_data},
};

static struct adie_codec_action_unit ftm_headset_mono_diff_rx_actions[] =
	HEADSET_MONO_DIFF_RX;

static struct adie_codec_hwsetting_entry ftm_headset_mono_diff_rx_settings[] = {
	{
		.freq_plan = 48000,
		.osr = 256,
		.actions = ftm_headset_mono_diff_rx_actions,
		.action_sz = ARRAY_SIZE(ftm_headset_mono_diff_rx_actions),
	},
};

static struct adie_codec_dev_profile ftm_headset_mono_diff_rx_profile = {
	.path_type = ADIE_CODEC_RX,
	.settings = ftm_headset_mono_diff_rx_settings,
	.setting_sz = ARRAY_SIZE(ftm_headset_mono_diff_rx_settings),
};

static struct snddev_icodec_data ftm_headset_mono_diff_rx_data = {
	.capability = SNDDEV_CAP_RX,
	.name = "ftm_headset_mono_diff_rx",
	.copp_id = PRIMARY_I2S_RX,
	.profile = &ftm_headset_mono_diff_rx_profile,
	.channel_mode = 1,
	.default_sample_rate = 48000,
	.voltage_on = msm_snddev_voltage_on,
	.voltage_off = msm_snddev_voltage_off,
};

static struct platform_device ftm_headset_mono_diff_rx_device = {
	.name = "snddev_icodec",
	.dev = { .platform_data = &ftm_headset_mono_diff_rx_data},
};

static struct adie_codec_action_unit ftm_spkr_mono_rx_actions[] =
	SPEAKER_PRI_STEREO_48000_OSR_256;

static struct adie_codec_hwsetting_entry ftm_spkr_mono_rx_settings[] = {
	{
		.freq_plan = 48000,
		.osr = 256,
		.actions = ftm_spkr_mono_rx_actions,
		.action_sz = ARRAY_SIZE(ftm_spkr_mono_rx_actions),
	},
};

static struct adie_codec_dev_profile ftm_spkr_mono_rx_profile = {
	.path_type = ADIE_CODEC_RX,
	.settings = ftm_spkr_mono_rx_settings,
	.setting_sz = ARRAY_SIZE(ftm_spkr_mono_rx_settings),
};

static struct snddev_icodec_data ftm_spkr_mono_rx_data = {
	.capability = SNDDEV_CAP_RX,
	.name = "ftm_spkr_mono_rx",
	.copp_id = PRIMARY_I2S_RX,
	.profile = &ftm_spkr_mono_rx_profile,
	.channel_mode = 1,
	.default_sample_rate = 48000,
	.pamp_on = msm_snddev_poweramp_on,
	.pamp_off = msm_snddev_poweramp_off,
};

static struct platform_device ftm_spkr_mono_rx_device = {
	.name = "snddev_icodec",
	.dev = { .platform_data = &ftm_spkr_mono_rx_data},
};

static struct adie_codec_action_unit ftm_spkr_l_rx_actions[] =
	FTM_SPKR_L_RX;

static struct adie_codec_hwsetting_entry ftm_spkr_l_rx_settings[] = {
	{
		.freq_plan = 48000,
		.osr = 256,
		.actions = ftm_spkr_l_rx_actions,
		.action_sz = ARRAY_SIZE(ftm_spkr_l_rx_actions),
	},
};

static struct adie_codec_dev_profile ftm_spkr_l_rx_profile = {
	.path_type = ADIE_CODEC_RX,
	.settings = ftm_spkr_l_rx_settings,
	.setting_sz = ARRAY_SIZE(ftm_spkr_l_rx_settings),
};

static struct snddev_icodec_data ftm_spkr_l_rx_data = {
	.capability = SNDDEV_CAP_RX,
	.name = "ftm_spkr_l_rx",
	.copp_id = PRIMARY_I2S_RX,
	.profile = &ftm_spkr_l_rx_profile,
	.channel_mode = 1,
	.default_sample_rate = 48000,
	.pamp_on = msm_snddev_poweramp_on,
	.pamp_off = msm_snddev_poweramp_off,
};

static struct platform_device ftm_spkr_l_rx_device = {
	.name = "snddev_icodec",
	.dev = { .platform_data = &ftm_spkr_l_rx_data},
};

static struct adie_codec_action_unit ftm_spkr_r_rx_actions[] =
	SPKR_R_RX;

static struct adie_codec_hwsetting_entry ftm_spkr_r_rx_settings[] = {
	{
		.freq_plan = 48000,
		.osr = 256,
		.actions = ftm_spkr_r_rx_actions,
		.action_sz = ARRAY_SIZE(ftm_spkr_r_rx_actions),
	},
};

static struct adie_codec_dev_profile ftm_spkr_r_rx_profile = {
	.path_type = ADIE_CODEC_RX,
	.settings = ftm_spkr_r_rx_settings,
	.setting_sz = ARRAY_SIZE(ftm_spkr_r_rx_settings),
};

static struct snddev_icodec_data ftm_spkr_r_rx_data = {
	.capability = SNDDEV_CAP_RX,
	.name = "ftm_spkr_r_rx",
	.copp_id = PRIMARY_I2S_RX,
	.profile = &ftm_spkr_r_rx_profile,
	.channel_mode = 1,
	.default_sample_rate = 48000,
	.pamp_on = msm_snddev_poweramp_on,
	.pamp_off = msm_snddev_poweramp_off,
};

static struct platform_device ftm_spkr_r_rx_device = {
	.name = "snddev_icodec",
	.dev = { .platform_data = &ftm_spkr_r_rx_data},
};

static struct adie_codec_action_unit ftm_spkr_mono_diff_rx_actions[] =
	SPKR_MONO_DIFF_RX;

static struct adie_codec_hwsetting_entry ftm_spkr_mono_diff_rx_settings[] = {
	{
		.freq_plan = 48000,
		.osr = 256,
		.actions = ftm_spkr_mono_diff_rx_actions,
		.action_sz = ARRAY_SIZE(ftm_spkr_mono_diff_rx_actions),
	},
};

static struct adie_codec_dev_profile ftm_spkr_mono_diff_rx_profile = {
	.path_type = ADIE_CODEC_RX,
	.settings = ftm_spkr_mono_diff_rx_settings,
	.setting_sz = ARRAY_SIZE(ftm_spkr_mono_diff_rx_settings),
};

static struct snddev_icodec_data ftm_spkr_mono_diff_rx_data = {
	.capability = SNDDEV_CAP_RX,
	.name = "ftm_spkr_mono_diff_rx",
	.copp_id = PRIMARY_I2S_RX,
	.profile = &ftm_spkr_mono_diff_rx_profile,
	.channel_mode = 1,
	.default_sample_rate = 48000,
	.pamp_on = msm_snddev_poweramp_on,
	.pamp_off = msm_snddev_poweramp_off,
};

static struct platform_device ftm_spkr_mono_diff_rx_device = {
	.name = "snddev_icodec",
	.dev = { .platform_data = &ftm_spkr_mono_diff_rx_data},
};

static struct adie_codec_action_unit ftm_headset_mono_l_rx_actions[] =
	HPH_PRI_AB_CPLS_MONO_LEFT;

static struct adie_codec_hwsetting_entry ftm_headset_mono_l_rx_settings[] = {
	{
		.freq_plan = 48000,
		.osr = 256,
		.actions = ftm_headset_mono_l_rx_actions,
		.action_sz = ARRAY_SIZE(ftm_headset_mono_l_rx_actions),
	},
};

static struct adie_codec_dev_profile ftm_headset_mono_l_rx_profile = {
	.path_type = ADIE_CODEC_RX,
	.settings = ftm_headset_mono_l_rx_settings,
	.setting_sz = ARRAY_SIZE(ftm_headset_mono_l_rx_settings),
};

static struct snddev_icodec_data ftm_headset_mono_l_rx_data = {
	.capability = SNDDEV_CAP_RX,
	.name = "ftm_headset_mono_l_rx",
	.copp_id = PRIMARY_I2S_RX,
	.profile = &ftm_headset_mono_l_rx_profile,
	.channel_mode = 1,
	.default_sample_rate = 48000,
	.voltage_on = msm_snddev_voltage_on,
	.voltage_off = msm_snddev_voltage_off,
};

static struct platform_device ftm_headset_mono_l_rx_device = {
	.name = "snddev_icodec",
	.dev = { .platform_data = &ftm_headset_mono_l_rx_data},
};

static struct adie_codec_action_unit ftm_headset_mono_r_rx_actions[] =
	HPH_PRI_AB_CPLS_MONO_RIGHT;

static struct adie_codec_hwsetting_entry ftm_headset_mono_r_rx_settings[] = {
	{
		.freq_plan = 48000,
		.osr = 256,
		.actions = ftm_headset_mono_r_rx_actions,
		.action_sz = ARRAY_SIZE(ftm_headset_mono_r_rx_actions),
	},
};

static struct adie_codec_dev_profile ftm_headset_mono_r_rx_profile = {
	.path_type = ADIE_CODEC_RX,
	.settings = ftm_headset_mono_r_rx_settings,
	.setting_sz = ARRAY_SIZE(ftm_headset_mono_r_rx_settings),
};

static struct snddev_icodec_data ftm_headset_mono_r_rx_data = {
	.capability = SNDDEV_CAP_RX,
	.name = "ftm_headset_mono_r_rx",
	.copp_id = PRIMARY_I2S_RX,
	.profile = &ftm_headset_mono_r_rx_profile,
	.channel_mode = 1,
	.default_sample_rate = 48000,
	.voltage_on = msm_snddev_voltage_on,
	.voltage_off = msm_snddev_voltage_off,
};

static struct platform_device ftm_headset_mono_r_rx_device = {
	.name = "snddev_icodec",
	.dev = { .platform_data = &ftm_headset_mono_r_rx_data},
};

static struct adie_codec_action_unit ftm_linein_l_tx_actions[] =
	LINEIN_MONO_L_TX;

static struct adie_codec_hwsetting_entry ftm_linein_l_tx_settings[] = {
	{
		.freq_plan = 48000,
		.osr = 256,
		.actions = ftm_linein_l_tx_actions,
		.action_sz = ARRAY_SIZE(ftm_linein_l_tx_actions),
	},
};

static struct adie_codec_dev_profile ftm_linein_l_tx_profile = {
	.path_type = ADIE_CODEC_TX,
	.settings = ftm_linein_l_tx_settings,
	.setting_sz = ARRAY_SIZE(ftm_linein_l_tx_settings),
};

static struct snddev_icodec_data ftm_linein_l_tx_data = {
	.capability = SNDDEV_CAP_TX,
	.name = "ftm_linein_l_tx",
	.copp_id = PRIMARY_I2S_TX,
	.profile = &ftm_linein_l_tx_profile,
	.channel_mode = 1,
	.default_sample_rate = 48000,
};

static struct platform_device ftm_linein_l_tx_device = {
	.name = "snddev_icodec",
	.dev = { .platform_data = &ftm_linein_l_tx_data },
};

static struct adie_codec_action_unit ftm_linein_r_tx_actions[] =
	LINEIN_MONO_R_TX;

static struct adie_codec_hwsetting_entry ftm_linein_r_tx_settings[] = {
	{
		.freq_plan = 48000,
		.osr = 256,
		.actions = ftm_linein_r_tx_actions,
		.action_sz = ARRAY_SIZE(ftm_linein_r_tx_actions),
	},
};

static struct adie_codec_dev_profile ftm_linein_r_tx_profile = {
	.path_type = ADIE_CODEC_TX,
	.settings = ftm_linein_r_tx_settings,
	.setting_sz = ARRAY_SIZE(ftm_linein_r_tx_settings),
};

static struct snddev_icodec_data ftm_linein_r_tx_data = {
	.capability = SNDDEV_CAP_TX,
	.name = "ftm_linein_r_tx",
	.copp_id = PRIMARY_I2S_TX,
	.profile = &ftm_linein_r_tx_profile,
	.channel_mode = 1,
	.default_sample_rate = 48000,
};

static struct platform_device ftm_linein_r_tx_device = {
	.name = "snddev_icodec",
	.dev = { .platform_data = &ftm_linein_r_tx_data },
};

static struct adie_codec_action_unit ftm_aux_out_rx_actions[] =
	AUX_OUT_RX;

static struct adie_codec_hwsetting_entry ftm_aux_out_rx_settings[] = {
	{
		.freq_plan = 48000,
		.osr = 256,
		.actions = ftm_aux_out_rx_actions,
		.action_sz = ARRAY_SIZE(ftm_aux_out_rx_actions),
	},
};

static struct adie_codec_dev_profile ftm_aux_out_rx_profile = {
	.path_type = ADIE_CODEC_RX,
	.settings = ftm_aux_out_rx_settings,
	.setting_sz = ARRAY_SIZE(ftm_aux_out_rx_settings),
};

static struct snddev_icodec_data ftm_aux_out_rx_data = {
	.capability = SNDDEV_CAP_RX,
	.name = "ftm_aux_out_rx",
	.copp_id = PRIMARY_I2S_RX,
	.profile = &ftm_aux_out_rx_profile,
	.channel_mode = 2,
	.default_sample_rate = 48000,
};

static struct platform_device ftm_aux_out_rx_device = {
	.name = "snddev_icodec",
	.dev = { .platform_data = &ftm_aux_out_rx_data},
};

static struct adie_codec_action_unit ftm_dmic1_left_tx_actions[] =
	DMIC1_LEFT_TX;

static struct adie_codec_hwsetting_entry ftm_dmic1_left_tx_settings[] = {
	{
		.freq_plan = 48000,
		.osr = 256,
		.actions = ftm_dmic1_left_tx_actions,
		.action_sz = ARRAY_SIZE(ftm_dmic1_left_tx_actions),
	},
};

static struct adie_codec_dev_profile ftm_dmic1_left_tx_profile = {
	.path_type = ADIE_CODEC_TX,
	.settings = ftm_dmic1_left_tx_settings,
	.setting_sz = ARRAY_SIZE(ftm_dmic1_left_tx_settings),
};

static struct snddev_icodec_data ftm_dmic1_left_tx_data = {
	.capability = SNDDEV_CAP_TX,
	.name = "ftm_dmic1_left_tx",
	.copp_id = PRIMARY_I2S_TX,
	.profile = &ftm_dmic1_left_tx_profile,
	.channel_mode = 1,
	.default_sample_rate = 48000,
	.pamp_on = msm_snddev_enable_dmic_power,
	.pamp_off = msm_snddev_disable_dmic_power,
};

static struct platform_device ftm_dmic1_left_tx_device = {
	.name = "snddev_icodec",
	.dev = { .platform_data = &ftm_dmic1_left_tx_data},
};

static struct adie_codec_action_unit ftm_dmic1_right_tx_actions[] =
	DMIC1_RIGHT_TX;

static struct adie_codec_hwsetting_entry ftm_dmic1_right_tx_settings[] = {
	{
		.freq_plan = 48000,
		.osr = 256,
		.actions = ftm_dmic1_right_tx_actions,
		.action_sz = ARRAY_SIZE(ftm_dmic1_right_tx_actions),
	},
};

static struct adie_codec_dev_profile ftm_dmic1_right_tx_profile = {
	.path_type = ADIE_CODEC_TX,
	.settings = ftm_dmic1_right_tx_settings,
	.setting_sz = ARRAY_SIZE(ftm_dmic1_right_tx_settings),
};

static struct snddev_icodec_data ftm_dmic1_right_tx_data = {
	.capability = SNDDEV_CAP_TX,
	.name = "ftm_dmic1_right_tx",
	.copp_id = PRIMARY_I2S_TX,
	.profile = &ftm_dmic1_right_tx_profile,
	.channel_mode = 1,
	.default_sample_rate = 48000,
	.pamp_on = msm_snddev_enable_dmic_power,
	.pamp_off = msm_snddev_disable_dmic_power,
};

static struct platform_device ftm_dmic1_right_tx_device = {
	.name = "snddev_icodec",
	.dev = { .platform_data = &ftm_dmic1_right_tx_data},
};

static struct adie_codec_action_unit ftm_dmic1_l_and_r_tx_actions[] =
	DMIC1_LEFT_AND_RIGHT_TX;

static struct adie_codec_hwsetting_entry ftm_dmic1_l_and_r_tx_settings[] = {
	{
		.freq_plan = 48000,
		.osr = 256,
		.actions = ftm_dmic1_l_and_r_tx_actions,
		.action_sz = ARRAY_SIZE(ftm_dmic1_l_and_r_tx_actions),
	},
};

static struct adie_codec_dev_profile ftm_dmic1_l_and_r_tx_profile = {
	.path_type = ADIE_CODEC_TX,
	.settings = ftm_dmic1_l_and_r_tx_settings,
	.setting_sz = ARRAY_SIZE(ftm_dmic1_l_and_r_tx_settings),
};

static struct snddev_icodec_data ftm_dmic1_l_and_r_tx_data = {
	.capability = SNDDEV_CAP_TX,
	.name = "ftm_dmic1_l_and_r_tx",
	.copp_id = PRIMARY_I2S_TX,
	.profile = &ftm_dmic1_l_and_r_tx_profile,
	.channel_mode = 2,
	.default_sample_rate = 48000,
	.pamp_on = msm_snddev_enable_dmic_power,
	.pamp_off = msm_snddev_disable_dmic_power,
};

static struct platform_device ftm_dmic1_l_and_r_tx_device = {
	.name = "snddev_icodec",
	.dev = { .platform_data = &ftm_dmic1_l_and_r_tx_data},
};

static struct adie_codec_action_unit ftm_dmic2_left_tx_actions[] =
	DMIC2_LEFT_TX;

static struct adie_codec_hwsetting_entry ftm_dmic2_left_tx_settings[] = {
	{
		.freq_plan = 48000,
		.osr = 256,
		.actions = ftm_dmic2_left_tx_actions,
		.action_sz = ARRAY_SIZE(ftm_dmic2_left_tx_actions),
	},
};

static struct adie_codec_dev_profile ftm_dmic2_left_tx_profile = {
	.path_type = ADIE_CODEC_TX,
	.settings = ftm_dmic2_left_tx_settings,
	.setting_sz = ARRAY_SIZE(ftm_dmic2_left_tx_settings),
};

static struct snddev_icodec_data ftm_dmic2_left_tx_data = {
	.capability = SNDDEV_CAP_TX,
	.name = "ftm_dmic2_left_tx",
	.copp_id = PRIMARY_I2S_TX,
	.profile = &ftm_dmic2_left_tx_profile,
	.channel_mode = 1,
	.default_sample_rate = 48000,
	.pamp_on = msm_snddev_enable_dmic_power,
	.pamp_off = msm_snddev_disable_dmic_power,
};

static struct platform_device ftm_dmic2_left_tx_device = {
	.name = "snddev_icodec",
	.dev = { .platform_data = &ftm_dmic2_left_tx_data },
};

static struct adie_codec_action_unit ftm_dmic2_right_tx_actions[] =
	DMIC2_RIGHT_TX;

static struct adie_codec_hwsetting_entry ftm_dmic2_right_tx_settings[] = {
	{
		.freq_plan = 48000,
		.osr = 256,
		.actions = ftm_dmic2_right_tx_actions,
		.action_sz = ARRAY_SIZE(ftm_dmic2_right_tx_actions),
	},
};

static struct adie_codec_dev_profile ftm_dmic2_right_tx_profile = {
	.path_type = ADIE_CODEC_TX,
	.settings = ftm_dmic2_right_tx_settings,
	.setting_sz = ARRAY_SIZE(ftm_dmic2_right_tx_settings),
};

static struct snddev_icodec_data ftm_dmic2_right_tx_data = {
	.capability = SNDDEV_CAP_TX,
	.name = "ftm_dmic2_right_tx",
	.copp_id = PRIMARY_I2S_TX,
	.profile = &ftm_dmic2_right_tx_profile,
	.channel_mode = 1,
	.default_sample_rate = 48000,
	.pamp_on = msm_snddev_enable_dmic_power,
	.pamp_off = msm_snddev_disable_dmic_power,
};

static struct platform_device ftm_dmic2_right_tx_device = {
	.name = "snddev_icodec",
	.dev = { .platform_data = &ftm_dmic2_right_tx_data },
};

static struct adie_codec_action_unit ftm_dmic2_l_and_r_tx_actions[] =
	DMIC2_LEFT_AND_RIGHT_TX;

static struct adie_codec_hwsetting_entry ftm_dmic2_l_and_r_tx_settings[] = {
	{
		.freq_plan = 48000,
		.osr = 256,
		.actions = ftm_dmic2_l_and_r_tx_actions,
		.action_sz = ARRAY_SIZE(ftm_dmic2_l_and_r_tx_actions),
	},
};

static struct adie_codec_dev_profile ftm_dmic2_l_and_r_tx_profile = {
	.path_type = ADIE_CODEC_TX,
	.settings = ftm_dmic2_l_and_r_tx_settings,
	.setting_sz = ARRAY_SIZE(ftm_dmic2_l_and_r_tx_settings),
};

static struct snddev_icodec_data ftm_dmic2_l_and_r_tx_data = {
	.capability = SNDDEV_CAP_TX,
	.name = "ftm_dmic2_l_and_r_tx",
	.copp_id = PRIMARY_I2S_TX,
	.profile = &ftm_dmic2_l_and_r_tx_profile,
	.channel_mode = 2,
	.default_sample_rate = 48000,
	.pamp_on = msm_snddev_enable_dmic_power,
	.pamp_off = msm_snddev_disable_dmic_power,
};

static struct platform_device ftm_dmic2_l_and_r_tx_device = {
	.name = "snddev_icodec",
	.dev = { .platform_data = &ftm_dmic2_l_and_r_tx_data},
};

static struct adie_codec_action_unit ftm_handset_mic1_aux_in_actions[] =
	HANDSET_MIC1_AUX_IN;

static struct adie_codec_hwsetting_entry ftm_handset_mic1_aux_in_settings[] = {
	{
		.freq_plan = 48000,
		.osr = 256,
		.actions = ftm_handset_mic1_aux_in_actions,
		.action_sz = ARRAY_SIZE(ftm_handset_mic1_aux_in_actions),
	},
};

static struct adie_codec_dev_profile ftm_handset_mic1_aux_in_profile = {
	.path_type = ADIE_CODEC_TX,
	.settings = ftm_handset_mic1_aux_in_settings,
	.setting_sz = ARRAY_SIZE(ftm_handset_mic1_aux_in_settings),
};

static struct snddev_icodec_data ftm_handset_mic1_aux_in_data = {
	.capability = SNDDEV_CAP_TX,
	.name = "ftm_handset_mic1_aux_in",
	.copp_id = PRIMARY_I2S_TX,
	.profile = &ftm_handset_mic1_aux_in_profile,
	.channel_mode = 2,
	.default_sample_rate = 48000,
	/* Assumption is that inputs are not tied to analog mic, so
	 * no need to enable mic bias.
	 */
};

static struct platform_device ftm_handset_mic1_aux_in_device = {
	.name = "snddev_icodec",
	.dev = { .platform_data = &ftm_handset_mic1_aux_in_data},
};

static struct snddev_mi2s_data snddev_mi2s_sd0_rx_data = {
	.capability = SNDDEV_CAP_RX ,
	.name = "mi2s_sd0_rx",
	.copp_id = MI2S_RX,
	.channel_mode = 2, /* stereo */
	.sd_lines = MI2S_SD0, /* sd0 */
	.sample_rate = 48000,
};

static struct platform_device ftm_mi2s_sd0_rx_device = {
	.name = "snddev_mi2s",
	.dev = { .platform_data = &snddev_mi2s_sd0_rx_data },
};

static struct snddev_mi2s_data snddev_mi2s_sd1_rx_data = {
	.capability = SNDDEV_CAP_RX ,
	.name = "mi2s_sd1_rx",
	.copp_id = MI2S_RX,
	.channel_mode = 2, /* stereo */
	.sd_lines = MI2S_SD1, /* sd1 */
	.sample_rate = 48000,
};

static struct platform_device ftm_mi2s_sd1_rx_device = {
	.name = "snddev_mi2s",
	.dev = { .platform_data = &snddev_mi2s_sd1_rx_data },
};

static struct snddev_mi2s_data snddev_mi2s_sd2_rx_data = {
	.capability = SNDDEV_CAP_RX ,
	.name = "mi2s_sd2_rx",
	.copp_id = MI2S_RX,
	.channel_mode = 2, /* stereo */
	.sd_lines = MI2S_SD2, /* sd2 */
	.sample_rate = 48000,
};

static struct platform_device ftm_mi2s_sd2_rx_device = {
	.name = "snddev_mi2s",
	.dev = { .platform_data = &snddev_mi2s_sd2_rx_data },
};

/* earpiece */
static struct adie_codec_action_unit ftm_handset_adie_lp_rx_actions[] =
	EAR_PRI_MONO_LB;

static struct adie_codec_hwsetting_entry ftm_handset_adie_lp_rx_settings[] = {
	{
		.freq_plan = 48000,
		.osr = 256,
		.actions = ftm_handset_adie_lp_rx_actions,
		.action_sz = ARRAY_SIZE(ftm_handset_adie_lp_rx_actions),
	}
};

static struct adie_codec_dev_profile ftm_handset_adie_lp_rx_profile = {
	.path_type = ADIE_CODEC_RX,
	.settings = ftm_handset_adie_lp_rx_settings,
	.setting_sz = ARRAY_SIZE(ftm_handset_adie_lp_rx_settings),
};

static struct snddev_icodec_data ftm_handset_adie_lp_rx_data = {
	.capability = (SNDDEV_CAP_RX | SNDDEV_CAP_VOICE),
	.name = "ftm_handset_adie_lp_rx",
	.copp_id = 0,
	.profile = &ftm_handset_adie_lp_rx_profile,
	.channel_mode = 1,
	.default_sample_rate = 48000,
};

static struct platform_device ftm_handset_adie_lp_rx_device = {
	.name = "snddev_icodec",
	.dev = { .platform_data = &ftm_handset_adie_lp_rx_data },
};

static struct adie_codec_action_unit ftm_headset_l_adie_lp_rx_actions[] =
	FTM_HPH_PRI_AB_CPLS_MONO_LB_LEFT;

static struct adie_codec_hwsetting_entry ftm_headset_l_adie_lp_rx_settings[] = {
	{
		.freq_plan = 48000,
		.osr = 256,
		.actions = ftm_headset_l_adie_lp_rx_actions,
		.action_sz = ARRAY_SIZE(ftm_headset_l_adie_lp_rx_actions),
	},
};

static struct adie_codec_dev_profile ftm_headset_l_adie_lp_rx_profile = {
	.path_type = ADIE_CODEC_RX,
	.settings = ftm_headset_l_adie_lp_rx_settings,
	.setting_sz = ARRAY_SIZE(ftm_headset_l_adie_lp_rx_settings),
};

static struct snddev_icodec_data ftm_headset_l_adie_lp_rx_data = {
	.capability = SNDDEV_CAP_RX,
	.name = "ftm_headset_l_adie_lp_rx",
	.copp_id = PRIMARY_I2S_RX,
	.profile = &ftm_headset_l_adie_lp_rx_profile,
	.channel_mode = 1,
	.default_sample_rate = 48000,
	.voltage_on = msm_snddev_voltage_on,
	.voltage_off = msm_snddev_voltage_off,
};

static struct platform_device ftm_headset_l_adie_lp_rx_device = {
	.name = "snddev_icodec",
	.dev = { .platform_data = &ftm_headset_l_adie_lp_rx_data },
};

static struct adie_codec_action_unit ftm_headset_r_adie_lp_rx_actions[] =
	FTM_HPH_PRI_AB_CPLS_MONO_LB_RIGHT;

static struct adie_codec_hwsetting_entry ftm_headset_r_adie_lp_rx_settings[] = {
	{
		.freq_plan = 48000,
		.osr = 256,
		.actions = ftm_headset_r_adie_lp_rx_actions,
		.action_sz = ARRAY_SIZE(ftm_headset_r_adie_lp_rx_actions),
	},
};

static struct adie_codec_dev_profile ftm_headset_r_adie_lp_rx_profile = {
	.path_type = ADIE_CODEC_RX,
	.settings = ftm_headset_r_adie_lp_rx_settings,
	.setting_sz = ARRAY_SIZE(ftm_headset_r_adie_lp_rx_settings),
};

static struct snddev_icodec_data ftm_headset_r_adie_lp_rx_data = {
	.capability = SNDDEV_CAP_RX,
	.name = "ftm_headset_r_adie_lp_rx",
	.copp_id = PRIMARY_I2S_RX,
	.profile = &ftm_headset_r_adie_lp_rx_profile,
	.channel_mode = 1,
	.default_sample_rate = 48000,
	.voltage_on = msm_snddev_voltage_on,
	.voltage_off = msm_snddev_voltage_off,
};

static struct platform_device ftm_headset_r_adie_lp_rx_device = {
	.name = "snddev_icodec",
	.dev = { .platform_data = &ftm_headset_r_adie_lp_rx_data },
};

static struct adie_codec_action_unit ftm_spkr_l_rx_lp_actions[] =
	FTM_SPKR_L_RX;

static struct adie_codec_hwsetting_entry ftm_spkr_l_rx_lp_settings[] = {
	{
		.freq_plan = 48000,
		.osr = 256,
		.actions = ftm_spkr_l_rx_lp_actions,
		.action_sz = ARRAY_SIZE(ftm_spkr_l_rx_lp_actions),
	},
};

static struct adie_codec_dev_profile ftm_spkr_l_rx_lp_profile = {
	.path_type = ADIE_CODEC_RX,
	.settings = ftm_spkr_l_rx_lp_settings,
	.setting_sz = ARRAY_SIZE(ftm_spkr_l_rx_lp_settings),
};

static struct snddev_icodec_data ftm_spkr_l_rx_lp_data = {
	.capability = SNDDEV_CAP_RX,
	.name = "ftm_spk_l_adie_lp_rx",
	.copp_id = PRIMARY_I2S_RX,
	.profile = &ftm_spkr_l_rx_lp_profile,
	.channel_mode = 1,
	.default_sample_rate = 48000,
	.pamp_on = msm_snddev_poweramp_on,
	.pamp_off = msm_snddev_poweramp_off,
};

static struct platform_device ftm_spk_l_adie_lp_rx_device = {
	.name = "snddev_icodec",
	.dev = { .platform_data = &ftm_spkr_l_rx_lp_data},
};

static struct adie_codec_action_unit ftm_spkr_r_adie_lp_rx_actions[] =
	FTM_SPKR_RX_LB;

static struct adie_codec_hwsetting_entry ftm_spkr_r_adie_lp_rx_settings[] = {
	{
		.freq_plan = 48000,
		.osr = 256,
		.actions = ftm_spkr_r_adie_lp_rx_actions,
		.action_sz = ARRAY_SIZE(ftm_spkr_r_adie_lp_rx_actions),
	},
};

static struct adie_codec_dev_profile ftm_spkr_r_adie_lp_rx_profile = {
	.path_type = ADIE_CODEC_RX,
	.settings = ftm_spkr_r_adie_lp_rx_settings,
	.setting_sz = ARRAY_SIZE(ftm_spkr_r_adie_lp_rx_settings),
};

static struct snddev_icodec_data ftm_spkr_r_adie_lp_rx_data = {
	.capability = SNDDEV_CAP_RX,
	.name = "ftm_spk_r_adie_lp_rx",
	.copp_id = PRIMARY_I2S_RX,
	.profile = &ftm_spkr_r_adie_lp_rx_profile,
	.channel_mode = 1,
	.default_sample_rate = 48000,
	.pamp_on = msm_snddev_poweramp_on,
	.pamp_off = msm_snddev_poweramp_off,
};

static struct platform_device ftm_spk_r_adie_lp_rx_device = {
	.name = "snddev_icodec",
	.dev = { .platform_data = &ftm_spkr_r_adie_lp_rx_data},
};

static struct adie_codec_action_unit ftm_spkr_adie_lp_rx_actions[] =
	FTM_SPKR_RX_LB;

static struct adie_codec_hwsetting_entry ftm_spkr_adie_lp_rx_settings[] = {
	{
		.freq_plan = 48000,
		.osr = 256,
		.actions = ftm_spkr_adie_lp_rx_actions,
		.action_sz = ARRAY_SIZE(ftm_spkr_adie_lp_rx_actions),
	},
};

static struct adie_codec_dev_profile ftm_spkr_adie_lp_rx_profile = {
	.path_type = ADIE_CODEC_RX,
	.settings = ftm_spkr_adie_lp_rx_settings,
	.setting_sz = ARRAY_SIZE(ftm_spkr_adie_lp_rx_settings),
};

static struct snddev_icodec_data ftm_spkr_adie_lp_rx_data = {
	.capability = SNDDEV_CAP_RX,
	.name = "ftm_spk_adie_lp_rx",
	.copp_id = PRIMARY_I2S_RX,
	.profile = &ftm_spkr_adie_lp_rx_profile,
	.channel_mode = 1,
	.default_sample_rate = 48000,
	.pamp_on = msm_snddev_poweramp_on,
	.pamp_off = msm_snddev_poweramp_off,
};

static struct platform_device ftm_spk_adie_lp_rx_device = {
	.name = "snddev_icodec",
	.dev = { .platform_data = &ftm_spkr_adie_lp_rx_data},
};

static struct adie_codec_action_unit ftm_handset_dual_tx_lp_actions[] =
	FTM_AMIC_DUAL_HANDSET_TX_LB;

static struct adie_codec_hwsetting_entry ftm_handset_dual_tx_lp_settings[] = {
	{
		.freq_plan = 48000,
		.osr = 256,
		.actions = ftm_handset_dual_tx_lp_actions,
		.action_sz = ARRAY_SIZE(ftm_handset_dual_tx_lp_actions),
	}
};

static struct adie_codec_dev_profile ftm_handset_dual_tx_lp_profile = {
	.path_type = ADIE_CODEC_TX,
	.settings = ftm_handset_dual_tx_lp_settings,
	.setting_sz = ARRAY_SIZE(ftm_handset_dual_tx_lp_settings),
};

static struct snddev_icodec_data ftm_handset_dual_tx_lp_data = {
	.capability = (SNDDEV_CAP_TX | SNDDEV_CAP_VOICE),
	.name = "handset_mic1_handset_mic2",
	.copp_id = 1,
	.profile = &ftm_handset_dual_tx_lp_profile,
	.channel_mode = 2,
	.default_sample_rate = 48000,
	.pamp_on = msm_snddev_enable_amic_power,
	.pamp_off = msm_snddev_disable_amic_power,
};

static struct platform_device ftm_handset_dual_tx_lp_device = {
	.name = "snddev_icodec",
	.dev = { .platform_data = &ftm_handset_dual_tx_lp_data },
};

static struct adie_codec_action_unit ftm_handset_mic_adie_lp_tx_actions[] =
	FTM_HANDSET_LB_TX;

static struct adie_codec_hwsetting_entry
	ftm_handset_mic_adie_lp_tx_settings[] = {
	{
		.freq_plan = 48000,
		.osr = 256,
		.actions = ftm_handset_mic_adie_lp_tx_actions,
		.action_sz = ARRAY_SIZE(ftm_handset_mic_adie_lp_tx_actions),
	}
};

static struct adie_codec_dev_profile ftm_handset_mic_adie_lp_tx_profile = {
	.path_type = ADIE_CODEC_TX,
	.settings = ftm_handset_mic_adie_lp_tx_settings,
	.setting_sz = ARRAY_SIZE(ftm_handset_mic_adie_lp_tx_settings),
};

static struct snddev_icodec_data ftm_handset_mic_adie_lp_tx_data = {
	.capability = (SNDDEV_CAP_TX | SNDDEV_CAP_VOICE),
	.name = "ftm_handset_mic_adie_lp_tx",
	.copp_id = 1,
	.profile = &ftm_handset_mic_adie_lp_tx_profile,
	.channel_mode = 1,
	.default_sample_rate = 48000,
	.pamp_on = msm_snddev_enable_amic_power,
	.pamp_off = msm_snddev_disable_amic_power,
};

static struct platform_device ftm_handset_mic_adie_lp_tx_device = {
	.name = "snddev_icodec",
	.dev = { .platform_data = &ftm_handset_mic_adie_lp_tx_data },
};

static struct adie_codec_action_unit ftm_headset_mic_adie_lp_tx_actions[] =
	FTM_HEADSET_LB_TX;

static struct adie_codec_hwsetting_entry
			ftm_headset_mic_adie_lp_tx_settings[] = {
	{
		.freq_plan = 48000,
		.osr = 256,
		.actions = ftm_headset_mic_adie_lp_tx_actions,
		.action_sz = ARRAY_SIZE(ftm_headset_mic_adie_lp_tx_actions),
	}
};

static struct adie_codec_dev_profile ftm_headset_mic_adie_lp_tx_profile = {
	.path_type = ADIE_CODEC_TX,
	.settings = ftm_headset_mic_adie_lp_tx_settings,
	.setting_sz = ARRAY_SIZE(ftm_headset_mic_adie_lp_tx_settings),
};

static struct snddev_icodec_data ftm_headset_mic_adie_lp_tx_data = {
	.capability = (SNDDEV_CAP_TX | SNDDEV_CAP_VOICE),
	.name = "ftm_headset_mic_adie_lp_tx",
	.copp_id = PRIMARY_I2S_TX,
	.profile = &ftm_headset_mic_adie_lp_tx_profile,
	.channel_mode = 1,
	.default_sample_rate = 48000,
};

static struct platform_device ftm_headset_mic_adie_lp_tx_device = {
	.name = "snddev_icodec",
	.dev = { .platform_data = &ftm_headset_mic_adie_lp_tx_data },
};

static struct snddev_virtual_data snddev_uplink_rx_data = {
	.capability = SNDDEV_CAP_RX,
	.name = "uplink_rx",
	.copp_id = VOICE_PLAYBACK_TX,
};

static struct platform_device msm_uplink_rx_device = {
	.name = "snddev_virtual",
	.dev = { .platform_data = &snddev_uplink_rx_data },
};

static struct snddev_hdmi_data snddev_hdmi_non_linear_pcm_rx_data = {
	.capability = SNDDEV_CAP_RX ,
	.name = "hdmi_pass_through",
	.default_sample_rate = 48000,
	.on_apps = 1,
};

static struct platform_device msm_snddev_hdmi_non_linear_pcm_rx_device = {
	.name = "snddev_hdmi",
	.dev = { .platform_data = &snddev_hdmi_non_linear_pcm_rx_data },
};


#ifdef CONFIG_DEBUG_FS
static struct adie_codec_action_unit
	ihs_stereo_rx_class_d_legacy_48KHz_osr256_actions[] =
	HPH_PRI_D_LEG_STEREO;

static struct adie_codec_hwsetting_entry
	ihs_stereo_rx_class_d_legacy_settings[] = {
	{
		.freq_plan = 48000,
		.osr = 256,
		.actions =
		ihs_stereo_rx_class_d_legacy_48KHz_osr256_actions,
		.action_sz = ARRAY_SIZE
		(ihs_stereo_rx_class_d_legacy_48KHz_osr256_actions),
	}
};

static struct adie_codec_action_unit
	ihs_stereo_rx_class_ab_legacy_48KHz_osr256_actions[] =
	HPH_PRI_AB_LEG_STEREO;

static struct adie_codec_hwsetting_entry
	ihs_stereo_rx_class_ab_legacy_settings[] = {
	{
		.freq_plan = 48000,
		.osr = 256,
		.actions =
		ihs_stereo_rx_class_ab_legacy_48KHz_osr256_actions,
		.action_sz = ARRAY_SIZE
		(ihs_stereo_rx_class_ab_legacy_48KHz_osr256_actions),
	}
};

static void snddev_hsed_config_modify_setting(int type)
{
	struct platform_device *device;
	struct snddev_icodec_data *icodec_data;

	device = &msm_headset_stereo_device;
	icodec_data = (struct snddev_icodec_data *)device->dev.platform_data;

	if (icodec_data) {
		if (type == 1) {
			icodec_data->voltage_on = NULL;
			icodec_data->voltage_off = NULL;
			icodec_data->profile->settings =
				ihs_stereo_rx_class_d_legacy_settings;
			icodec_data->profile->setting_sz =
			ARRAY_SIZE(ihs_stereo_rx_class_d_legacy_settings);
		} else if (type == 2) {
			icodec_data->voltage_on = NULL;
			icodec_data->voltage_off = NULL;
			icodec_data->profile->settings =
				ihs_stereo_rx_class_ab_legacy_settings;
			icodec_data->profile->setting_sz =
			ARRAY_SIZE(ihs_stereo_rx_class_ab_legacy_settings);
		}
	}
}

static void snddev_hsed_config_restore_setting(void)
{
	struct platform_device *device;
	struct snddev_icodec_data *icodec_data;

	device = &msm_headset_stereo_device;
	icodec_data = (struct snddev_icodec_data *)device->dev.platform_data;

	if (icodec_data) {
		icodec_data->voltage_on = msm_snddev_voltage_on;
		icodec_data->voltage_off = msm_snddev_voltage_off;
		icodec_data->profile->settings = headset_ab_cpls_settings;
		icodec_data->profile->setting_sz =
			ARRAY_SIZE(headset_ab_cpls_settings);
	}
}

static ssize_t snddev_hsed_config_debug_write(struct file *filp,
	const char __user *ubuf, size_t cnt, loff_t *ppos)
{
	char *lb_str = filp->private_data;
	char cmd;

	if (get_user(cmd, ubuf))
		return -EFAULT;

	if (!strcmp(lb_str, "msm_hsed_config")) {
		switch (cmd) {
		case '0':
			snddev_hsed_config_restore_setting();
			break;

		case '1':
			snddev_hsed_config_modify_setting(1);
			break;

		case '2':
			snddev_hsed_config_modify_setting(2);
			break;

		default:
			break;
		}
	}
	return cnt;
}

static int snddev_hsed_config_debug_open(struct inode *inode, struct file *file)
{
	file->private_data = inode->i_private;
	return 0;
}

static const struct file_operations snddev_hsed_config_debug_fops = {
	.open = snddev_hsed_config_debug_open,
	.write = snddev_hsed_config_debug_write
};
#endif

static struct platform_device *snd_devices_ffa[] __initdata = {
	&msm_iearpiece_ffa_device,
	&msm_imic_ffa_device,
	&msm_ispkr_stereo_device,
	&msm_snddev_hdmi_stereo_rx_device,
	&msm_headset_mic_device,
	&msm_ispkr_mic_device,
	&msm_bt_sco_earpiece_device,
	&msm_bt_sco_mic_device,
	&msm_headset_stereo_device,
	&msm_itty_mono_tx_device,
	&msm_itty_mono_rx_device,
	&msm_mi2s_fm_tx_device,
	&msm_mi2s_fm_rx_device,
	&msm_hs_dual_mic_endfire_device,
	&msm_spkr_dual_mic_endfire_device,
	&msm_hs_dual_mic_broadside_device,
	&msm_spkr_dual_mic_broadside_device,
	&msm_ihs_stereo_speaker_stereo_rx_device,
	&msm_anc_headset_device,
	&msm_auxpga_lp_hs_device,
	&msm_auxpga_lp_lo_device,
	&msm_linein_pri_device,
	&msm_icodec_gpio_device,
	&msm_snddev_hdmi_non_linear_pcm_rx_device,
};

static struct platform_device *snd_devices_surf[] __initdata = {
	&msm_iearpiece_device,
	&msm_imic_device,
	&msm_ispkr_stereo_device,
	&msm_snddev_hdmi_stereo_rx_device,
	&msm_headset_mic_device,
	&msm_ispkr_mic_device,
	&msm_bt_sco_earpiece_device,
	&msm_bt_sco_mic_device,
	&msm_headset_stereo_device,
	&msm_itty_mono_tx_device,
	&msm_itty_mono_rx_device,
#ifdef LGE_NOT_SUPPORT_DEVICE
	&msm_mi2s_fm_tx_device,
	&msm_mi2s_fm_rx_device,
#endif
	&msm_ihs_stereo_speaker_stereo_rx_device,
	&msm_auxpga_lp_hs_device,
	&msm_auxpga_lp_lo_device,
	&msm_linein_pri_device,
	&msm_icodec_gpio_device,
	&msm_snddev_hdmi_non_linear_pcm_rx_device,
};

static struct platform_device *snd_devices_fluid[] __initdata = {
	&msm_iearpiece_device,
	&msm_imic_device,
	&msm_ispkr_stereo_device,
	&msm_snddev_hdmi_stereo_rx_device,
	&msm_headset_stereo_device,
	&msm_headset_mic_device,
	&msm_fluid_ispkr_mic_device,
	&msm_bt_sco_earpiece_device,
	&msm_bt_sco_mic_device,
	&msm_mi2s_fm_tx_device,
	&msm_mi2s_fm_rx_device,
	&msm_anc_headset_device,
	&msm_auxpga_lp_hs_device,
	&msm_auxpga_lp_lo_device,
	&msm_icodec_gpio_device,
	&msm_snddev_hdmi_non_linear_pcm_rx_device,
};

static struct platform_device *snd_devices_qt[] __initdata = {
	&msm_headset_stereo_device,
	&msm_headset_mic_device,
	&msm_ispkr_stereo_device,
	&msm_qt_dual_dmic_d0_device,
	&msm_snddev_hdmi_stereo_rx_device,
	&msm_qt_icodec_gpio_device,
};

static struct platform_device *snd_devices_common[] __initdata = {
	&msm_aux_pcm_device,
	&msm_cdcclk_ctl_device,
#ifdef LGE_NOT_SUPPORT_DEVICE
	&msm_mi2s_device,
#endif
	&msm_uplink_rx_device,
};

static struct platform_device *snd_devices_ftm[] __initdata = {
	&ftm_headset_mono_rx_device,
	&ftm_headset_mono_l_rx_device,
	&ftm_headset_mono_r_rx_device,
	&ftm_headset_mono_diff_rx_device,
	&ftm_spkr_mono_rx_device,
	&ftm_spkr_l_rx_device,
	&ftm_spkr_r_rx_device,
	&ftm_spkr_mono_diff_rx_device,
	&ftm_linein_l_tx_device,
	&ftm_linein_r_tx_device,
	&ftm_aux_out_rx_device,
	&ftm_dmic1_left_tx_device,
	&ftm_dmic1_right_tx_device,
	&ftm_dmic1_l_and_r_tx_device,
	&ftm_dmic2_left_tx_device,
	&ftm_dmic2_right_tx_device,
	&ftm_dmic2_l_and_r_tx_device,
	&ftm_handset_mic1_aux_in_device,
	&ftm_mi2s_sd0_rx_device,
	&ftm_mi2s_sd1_rx_device,
	&ftm_mi2s_sd2_rx_device,
	&ftm_handset_mic_adie_lp_tx_device,
	&ftm_headset_mic_adie_lp_tx_device,
	&ftm_handset_adie_lp_rx_device,
	&ftm_headset_l_adie_lp_rx_device,
	&ftm_headset_r_adie_lp_rx_device,
	&ftm_spk_l_adie_lp_rx_device,
	&ftm_spk_r_adie_lp_rx_device,
	&ftm_spk_adie_lp_rx_device,
	&ftm_handset_dual_tx_lp_device,
};


void __init msm_snddev_init(void)
{
	int i;
	int dev_id;
#ifndef CONFIG_LGE_AUDIO
	int rc;
#endif
	atomic_set(&pamp_ref_cnt, 0);
	atomic_set(&preg_ref_cnt, 0);

	for (i = 0, dev_id = 0; i < ARRAY_SIZE(snd_devices_common); i++)
		snd_devices_common[i]->id = dev_id++;

	platform_add_devices(snd_devices_common,
		ARRAY_SIZE(snd_devices_common));

	/* Auto detect device base on machine info */
	if (machine_is_msm8x60_surf() || machine_is_msm8x60_charm_surf()) {
		for (i = 0; i < ARRAY_SIZE(snd_devices_surf); i++)
			snd_devices_surf[i]->id = dev_id++;

		platform_add_devices(snd_devices_surf,
		ARRAY_SIZE(snd_devices_surf));
		pr_err("%s: SURF device %d \n", __func__, __LINE__);
	} else if (machine_is_msm8x60_ffa() ||
			machine_is_msm8x60_charm_ffa()) {
		for (i = 0; i < ARRAY_SIZE(snd_devices_ffa); i++)
			snd_devices_ffa[i]->id = dev_id++;

		platform_add_devices(snd_devices_ffa,
		ARRAY_SIZE(snd_devices_ffa));
		pr_err("%s: FFA device %d \n", __func__, __LINE__);
	} else if (machine_is_msm8x60_fluid()) {
		for (i = 0; i < ARRAY_SIZE(snd_devices_fluid); i++)
			snd_devices_fluid[i]->id = dev_id++;

		platform_add_devices(snd_devices_fluid,
		ARRAY_SIZE(snd_devices_fluid));
	} else if (machine_is_msm8x60_qt()) {
		for (i = 0; i < ARRAY_SIZE(snd_devices_qt); i++)
			snd_devices_qt[i]->id = dev_id++;

		platform_add_devices(snd_devices_qt,
		ARRAY_SIZE(snd_devices_qt));
	}

	if (machine_is_msm8x60_surf()) {
		for (i = 0; i < ARRAY_SIZE(snd_devices_ftm); i++)
			snd_devices_ftm[i]->id = dev_id++;

		platform_add_devices(snd_devices_ftm,
				ARRAY_SIZE(snd_devices_ftm));
	}

#ifdef CONFIG_DEBUG_FS
	debugfs_hsed_config = debugfs_create_file("msm_hsed_config",
				S_IFREG | S_IRUGO, NULL,
		(void *) "msm_hsed_config", &snddev_hsed_config_debug_fops);
#endif

#ifdef CONFIG_LGE_AUDIO

#else
	rc = gpio_request(SNDDEV_GPIO_CLASS_D1_EN, "CLASSD1_EN");
	if (rc) {
		pr_err("%s: spkr pamp gpio %d request"
			"failed\n", __func__, SNDDEV_GPIO_CLASS_D1_EN);
	} else {
		gpio_direction_output(SNDDEV_GPIO_CLASS_D1_EN, 0);
		gpio_free(SNDDEV_GPIO_CLASS_D1_EN);
	}
#endif	
}
