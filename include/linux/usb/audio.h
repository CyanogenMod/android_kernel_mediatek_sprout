/*
 * <linux/usb/audio.h> -- USB Audio definitions.
 *
 * Copyright (C) 2006 Thumtronics Pty Ltd.
 * Developed for Thumtronics by Grey Innovation
 * Ben Williamson <ben.williamson@greyinnovation.com>
 *
 * This software is distributed under the terms of the GNU General Public
 * License ("GPL") version 2, as published by the Free Software Foundation.
 *
 * This file holds USB constants and structures defined
 * by the USB Device Class Definition for Audio Devices.
 * Comments below reference relevant sections of that document:
 *
 * http://www.usb.org/developers/devclass_docs/audio10.pdf
 *
 * Types and defines in this file are either specific to version 1.0 of
 * this standard or common for newer versions.
 */
#ifndef __LINUX_USB_AUDIO_H
#define __LINUX_USB_AUDIO_H

#include <uapi/linux/usb/audio.h>

<<<<<<< HEAD
static inline __u8 uac_mixer_unit_bNrChannels(struct uac_mixer_unit_descriptor *desc)
{
	return desc->baSourceID[desc->bNrInPins];
}

static inline __u32 uac_mixer_unit_wChannelConfig(struct uac_mixer_unit_descriptor *desc,
						  int protocol)
{
	if (protocol == UAC_VERSION_1)
		return (desc->baSourceID[desc->bNrInPins + 2] << 8) |
			desc->baSourceID[desc->bNrInPins + 1];
	else
		return  (desc->baSourceID[desc->bNrInPins + 4] << 24) |
			(desc->baSourceID[desc->bNrInPins + 3] << 16) |
			(desc->baSourceID[desc->bNrInPins + 2] << 8)  |
			(desc->baSourceID[desc->bNrInPins + 1]);
}

static inline __u8 uac_mixer_unit_iChannelNames(struct uac_mixer_unit_descriptor *desc,
						int protocol)
{
	return (protocol == UAC_VERSION_1) ?
		desc->baSourceID[desc->bNrInPins + 3] :
		desc->baSourceID[desc->bNrInPins + 5];
}

static inline __u8 *uac_mixer_unit_bmControls(struct uac_mixer_unit_descriptor *desc,
					      int protocol)
{
	return (protocol == UAC_VERSION_1) ?
		&desc->baSourceID[desc->bNrInPins + 4] :
		&desc->baSourceID[desc->bNrInPins + 6];
}

static inline __u8 uac_mixer_unit_iMixer(struct uac_mixer_unit_descriptor *desc)
{
	__u8 *raw = (__u8 *) desc;
	return raw[desc->bLength - 1];
}

/* 4.3.2.4 Selector Unit Descriptor */
struct uac_selector_unit_descriptor {
	__u8 bLength;
	__u8 bDescriptorType;
	__u8 bDescriptorSubtype;
	__u8 bUintID;
	__u8 bNrInPins;
	__u8 baSourceID[];
} __attribute__ ((packed));

static inline __u8 uac_selector_unit_iSelector(struct uac_selector_unit_descriptor *desc)
{
	__u8 *raw = (__u8 *) desc;
	return raw[desc->bLength - 1];
}

/* 4.3.2.5 Feature Unit Descriptor */
struct uac_feature_unit_descriptor {
	__u8 bLength;
	__u8 bDescriptorType;
	__u8 bDescriptorSubtype;
	__u8 bUnitID;
	__u8 bSourceID;
	__u8 bControlSize;
	__u8 bmaControls[0]; /* variable length */
} __attribute__((packed));

static inline __u8 uac_feature_unit_iFeature(struct uac_feature_unit_descriptor *desc)
{
	__u8 *raw = (__u8 *) desc;
	return raw[desc->bLength - 1];
}

/* 4.3.2.6 Processing Unit Descriptors */
struct uac_processing_unit_descriptor {
	__u8 bLength;
	__u8 bDescriptorType;
	__u8 bDescriptorSubtype;
	__u8 bUnitID;
	__u16 wProcessType;
	__u8 bNrInPins;
	__u8 baSourceID[];
} __attribute__ ((packed));

static inline __u8 uac_processing_unit_bNrChannels(struct uac_processing_unit_descriptor *desc)
{
	return desc->baSourceID[desc->bNrInPins];
}

static inline __u32 uac_processing_unit_wChannelConfig(struct uac_processing_unit_descriptor *desc,
						       int protocol)
{
	if (protocol == UAC_VERSION_1)
		return (desc->baSourceID[desc->bNrInPins + 2] << 8) |
			desc->baSourceID[desc->bNrInPins + 1];
	else
		return  (desc->baSourceID[desc->bNrInPins + 4] << 24) |
			(desc->baSourceID[desc->bNrInPins + 3] << 16) |
			(desc->baSourceID[desc->bNrInPins + 2] << 8)  |
			(desc->baSourceID[desc->bNrInPins + 1]);
}

static inline __u8 uac_processing_unit_iChannelNames(struct uac_processing_unit_descriptor *desc,
						     int protocol)
{
	return (protocol == UAC_VERSION_1) ?
		desc->baSourceID[desc->bNrInPins + 3] :
		desc->baSourceID[desc->bNrInPins + 5];
}

static inline __u8 uac_processing_unit_bControlSize(struct uac_processing_unit_descriptor *desc,
						    int protocol)
{
	return (protocol == UAC_VERSION_1) ?
		desc->baSourceID[desc->bNrInPins + 4] :
		desc->baSourceID[desc->bNrInPins + 6];
}

static inline __u8 *uac_processing_unit_bmControls(struct uac_processing_unit_descriptor *desc,
						   int protocol)
{
	return (protocol == UAC_VERSION_1) ?
		&desc->baSourceID[desc->bNrInPins + 5] :
		&desc->baSourceID[desc->bNrInPins + 7];
}

static inline __u8 uac_processing_unit_iProcessing(struct uac_processing_unit_descriptor *desc,
						   int protocol)
{
	__u8 control_size = uac_processing_unit_bControlSize(desc, protocol);
	return *(uac_processing_unit_bmControls(desc, protocol)
			+ control_size);
}

static inline __u8 *uac_processing_unit_specific(struct uac_processing_unit_descriptor *desc,
						 int protocol)
{
	__u8 control_size = uac_processing_unit_bControlSize(desc, protocol);
	return uac_processing_unit_bmControls(desc, protocol)
			+ control_size + 1;
}

/* 4.5.2 Class-Specific AS Interface Descriptor */
struct uac1_as_header_descriptor {
	__u8  bLength;			/* in bytes: 7 */
	__u8  bDescriptorType;		/* USB_DT_CS_INTERFACE */
	__u8  bDescriptorSubtype;	/* AS_GENERAL */
	__u8  bTerminalLink;		/* Terminal ID of connected Terminal */
	__u8  bDelay;			/* Delay introduced by the data path */
	__le16 wFormatTag;		/* The Audio Data Format */
} __attribute__ ((packed));

#define UAC_DT_AS_HEADER_SIZE		7

/* Formats - A.1.1 Audio Data Format Type I Codes */
#define UAC_FORMAT_TYPE_I_UNDEFINED	0x0
#define UAC_FORMAT_TYPE_I_PCM		0x1
#define UAC_FORMAT_TYPE_I_PCM8		0x2
#define UAC_FORMAT_TYPE_I_IEEE_FLOAT	0x3
#define UAC_FORMAT_TYPE_I_ALAW		0x4
#define UAC_FORMAT_TYPE_I_MULAW		0x5

struct uac_format_type_i_continuous_descriptor {
	__u8  bLength;			/* in bytes: 8 + (ns * 3) */
	__u8  bDescriptorType;		/* USB_DT_CS_INTERFACE */
	__u8  bDescriptorSubtype;	/* FORMAT_TYPE */
	__u8  bFormatType;		/* FORMAT_TYPE_1 */
	__u8  bNrChannels;		/* physical channels in the stream */
	__u8  bSubframeSize;		/* */
	__u8  bBitResolution;
	__u8  bSamFreqType;
	__u8  tLowerSamFreq[3];
	__u8  tUpperSamFreq[3];
} __attribute__ ((packed));

#define UAC_FORMAT_TYPE_I_CONTINUOUS_DESC_SIZE	14

struct uac_format_type_i_discrete_descriptor {
	__u8  bLength;			/* in bytes: 8 + (ns * 3) */
	__u8  bDescriptorType;		/* USB_DT_CS_INTERFACE */
	__u8  bDescriptorSubtype;	/* FORMAT_TYPE */
	__u8  bFormatType;		/* FORMAT_TYPE_1 */
	__u8  bNrChannels;		/* physical channels in the stream */
	__u8  bSubframeSize;		/* */
	__u8  bBitResolution;
	__u8  bSamFreqType;
	__u8  tSamFreq[][3];
} __attribute__ ((packed));

#define DECLARE_UAC_FORMAT_TYPE_I_DISCRETE_DESC(n)		\
struct uac_format_type_i_discrete_descriptor_##n {		\
	__u8  bLength;						\
	__u8  bDescriptorType;					\
	__u8  bDescriptorSubtype;				\
	__u8  bFormatType;					\
	__u8  bNrChannels;					\
	__u8  bSubframeSize;					\
	__u8  bBitResolution;					\
	__u8  bSamFreqType;					\
	__u8  tSamFreq[n][3];					\
} __attribute__ ((packed))

#define UAC_FORMAT_TYPE_I_DISCRETE_DESC_SIZE(n)	(8 + (n * 3))

struct uac_format_type_i_ext_descriptor {
	__u8 bLength;
	__u8 bDescriptorType;
	__u8 bDescriptorSubtype;
	__u8 bFormatType;
	__u8 bSubslotSize;
	__u8 bBitResolution;
	__u8 bHeaderLength;
	__u8 bControlSize;
	__u8 bSideBandProtocol;
} __attribute__((packed));

/* Formats - Audio Data Format Type I Codes */

#define UAC_FORMAT_TYPE_II_MPEG	0x1001
#define UAC_FORMAT_TYPE_II_AC3	0x1002

struct uac_format_type_ii_discrete_descriptor {
	__u8 bLength;
	__u8 bDescriptorType;
	__u8 bDescriptorSubtype;
	__u8 bFormatType;
	__le16 wMaxBitRate;
	__le16 wSamplesPerFrame;
	__u8 bSamFreqType;
	__u8 tSamFreq[][3];
} __attribute__((packed));

struct uac_format_type_ii_ext_descriptor {
	__u8 bLength;
	__u8 bDescriptorType;
	__u8 bDescriptorSubtype;
	__u8 bFormatType;
	__u16 wMaxBitRate;
	__u16 wSamplesPerFrame;
	__u8 bHeaderLength;
	__u8 bSideBandProtocol;
} __attribute__((packed));

/* type III */
#define UAC_FORMAT_TYPE_III_IEC1937_AC3	0x2001
#define UAC_FORMAT_TYPE_III_IEC1937_MPEG1_LAYER1	0x2002
#define UAC_FORMAT_TYPE_III_IEC1937_MPEG2_NOEXT	0x2003
#define UAC_FORMAT_TYPE_III_IEC1937_MPEG2_EXT	0x2004
#define UAC_FORMAT_TYPE_III_IEC1937_MPEG2_LAYER1_LS	0x2005
#define UAC_FORMAT_TYPE_III_IEC1937_MPEG2_LAYER23_LS	0x2006

/* Formats - A.2 Format Type Codes */
#define UAC_FORMAT_TYPE_UNDEFINED	0x0
#define UAC_FORMAT_TYPE_I		0x1
#define UAC_FORMAT_TYPE_II		0x2
#define UAC_FORMAT_TYPE_III		0x3
#define UAC_EXT_FORMAT_TYPE_I		0x81
#define UAC_EXT_FORMAT_TYPE_II		0x82
#define UAC_EXT_FORMAT_TYPE_III		0x83

struct uac_iso_endpoint_descriptor {
	__u8  bLength;			/* in bytes: 7 */
	__u8  bDescriptorType;		/* USB_DT_CS_ENDPOINT */
	__u8  bDescriptorSubtype;	/* EP_GENERAL */
	__u8  bmAttributes;
	__u8  bLockDelayUnits;
	__le16 wLockDelay;
} __attribute__((packed));
#define UAC_ISO_ENDPOINT_DESC_SIZE	7

#define UAC_EP_CS_ATTR_SAMPLE_RATE	0x01
#define UAC_EP_CS_ATTR_PITCH_CONTROL	0x02
#define UAC_EP_CS_ATTR_FILL_MAX		0x80

/* status word format (3.7.1.1) */

#define UAC1_STATUS_TYPE_ORIG_MASK		0x0f
#define UAC1_STATUS_TYPE_ORIG_AUDIO_CONTROL_IF	0x0
#define UAC1_STATUS_TYPE_ORIG_AUDIO_STREAM_IF	0x1
#define UAC1_STATUS_TYPE_ORIG_AUDIO_STREAM_EP	0x2

#define UAC1_STATUS_TYPE_IRQ_PENDING		(1 << 7)
#define UAC1_STATUS_TYPE_MEM_CHANGED		(1 << 6)

struct uac1_status_word {
	__u8 bStatusType;
	__u8 bOriginator;
} __attribute__((packed));

#ifdef __KERNEL__
=======
>>>>>>> v3.10.88

struct usb_audio_control {
	struct list_head list;
	const char *name;
	u8 type;
	int data[5];
	int (*set)(struct usb_audio_control *con, u8 cmd, int value);
	int (*get)(struct usb_audio_control *con, u8 cmd);
};

struct usb_audio_control_selector {
	struct list_head list;
	struct list_head control;
	u8 id;
	const char *name;
	u8 type;
	struct usb_descriptor_header *desc;
};

#endif /* __LINUX_USB_AUDIO_H */
