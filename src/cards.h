/* Copyright (c) 2013-2014 Florian L. <dev@der-flo.net>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef	cards_H
#define	cards_H

/*
 *	To Have and Have Not
 *	According to RFC 6350
 */
#define	MUST_BE_MASK			0xF
#define	HAVE_BEGIN				0x1
#define	HAVE_FN					0x2
#define	HAVE_VERSION			0x4
#define	HAVE_END				0x8

/*
 *	GENERAL TYPES
 */
#define	CARDTYPE_BEGIN			11
#define	CARDTYPE_END			12
#define	CARDTYPE_SOURCE			13
#define	CARDTYPE_KIND			14
#define	CARDTYPE_XML			15

/*
 *	IDENTIFICATION TYPES
 */
#define	CARDTYPE_FN				21
#define	CARDTYPE_FN_FIRST		211
#define	CARDTYPE_FN_LAST		212
#define	CARDTYPE_FN_PREFIX		213
#define	CARDTYPE_FN_MIDDLE		214
#define	CARDTYPE_FN_SUFFIX		215
#define	CARDTYPE_N				22
#define	CARDTYPE_NICKNAME		23
#define	CARDTYPE_PHOTO			24
#define	CARDTYPE_BDAY			25
#define	CARDTYPE_ANNIVERSARY	26
#define	CARDTYPE_GENDER			27

/*
 *	DELIVERY ADDRESSING TYPES
 */
#define	CARDTYPE_ADR			310
#define	CARDTYPE_ADR_OPT		311
#define	CARDTYPE_ADR_OFFICE_BOX	312
#define	CARDTYPE_ADR_EXT_ADDR	313
#define	CARDTYPE_ADR_STREET		314
#define	CARDTYPE_ADR_CITY		315
#define	CARDTYPE_ADR_REGION		316
#define	CARDTYPE_ADR_ZIP		317
#define	CARDTYPE_ADR_COUNTRY	318

/*
 *	TELECOMMUNICATIONS ADDRESSING TYPES
 */
#define	CARDTYPE_TEL			410
#define	CARDTYPE_TEL_OPT		411
#define	CARDTYPE_EMAIL			420
#define	CARDTYPE_EMAIL_OPT		421
#define	CARDTYPE_IMPP			430
#define	CARDTYPE_IMPP_OPT		431
#define	CARDTYPE_LANG			44

/*
 *	GEOGRAPHICAL TYPES
 */
#define	CARDTYPE_TZ				51
#define	CARDTYPE_GEO			52

/*
 *	ORGANIZATIONAL TYPES
 */
#define	CARDTYPE_TITLE			61
#define	CARDTYPE_ROLE			62
#define	CARDTYPE_LOGO			63
#define	CARDTYPE_ORG			64
#define	CARDTYPE_MEMBER			65
#define	CARDTYPE_RELATED		66

/*
 *	EXPLANATORY TYPES
 */
#define	CARDTYPE_CATEGORIES		71
#define	CARDTYPE_NOTE			72
#define	CARDTYPE_PRODID			73
#define	CARDTYPE_REV			74
#define	CARDTYPE_SOUND			75
#define	CARDTYPE_UID			76
#define	CARDTYPE_CLIENTPIDMAP	77
#define	CARDTYPE_URL			78
#define	CARDTYPE_VERSION		79

/*
 *	SECURITY TYPES
 */
#define	CARDTYPE_KEY			81

/*
 *	CALENDAR TYPES
 */
#define	CARDTYPE_FBURL			91
#define	CARDTYPE_CALADRURI		92
#define	CARDTYPE_CALURI			93

/*
 *	OFF THE RFC
 */
#define	CARDTYPE_SHOWAS			100

/*
 *	TYPE VALUES
 */
#define	TYPE_WORK						1 << 1
#define	TYPE_HOME						1 << 2
#define	TYPE_TEL_TEXT					1 << 3
#define	TYPE_TEL_VOICE					1 << 4
#define	TYPE_TEL_FAX					1 << 5
#define	TYPE_TEL_CELL					1 << 6
#define	TYPE_TEL_VIDEO					1 << 7
#define	TYPE_TEL_PAGER					1 << 8
#define	TYPE_TEL_TEXTPHONE				1 << 9
#define	TYPE_RELATED_CONTACT			1 << 10
#define	TYPE_RELATED_ACQUAINTANCE		1 << 11
#define	TYPE_RELATED_FRIEND				1 << 12
#define	TYPE_RELATED_MET				1 << 13
#define	TYPE_RELATED_CO_WORKER			1 << 14
#define	TYPE_RELATED_COLLEAGUE			1 << 15
#define	TYPE_RELATED_CO_RESIDENT		1 << 16
#define	TYPE_RELATED_NEIGHBOR			1 << 17
#define	TYPE_RELATED_CHILD				1 << 18
#define	TYPE_RELATED_PARENT				1 << 19
#define	TYPE_RELATED_SIBLING			1 << 20
#define	TYPE_RELATED_SPOUSE				1 << 21
#define	TYPE_RELATED_KIN				1 << 22
#define	TYPE_RELATED_MUSE				1 << 23
#define	TYPE_RELATED_CRUSH				1 << 24
#define	TYPE_RELATED_DATE				1 << 25
#define	TYPE_RELATED_SWEETHEART			1 << 26
#define	TYPE_RELATED_ME					1 << 27
#define	TYPE_RELATED_AGENT				1 << 28
#define	TYPE_RELATED_EMERGENCY			1 << 29

extern char *buildCard(GSList *list);
extern char *getSingleCardAttribut(int type, char *card);
extern char *getUID(void);
extern GSList *getMultipleCardAttribut(int key, char *card, gboolean attribute);
extern ContactCards_pix_t *getCardPhoto(char *card);
extern char *mergeCards(GSList *new, char *old);
extern GSList *validateFile(char *content);

#endif	/*	cards_H		*/
