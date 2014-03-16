/*
 *	cards.h
 */

#ifndef	cards_H
#define	cards_H

/*
 *	IDENTIFICATION TYPES
 */
#define	CARDTYPE_FN				11
#define	CARDTYPE_FN_FIRST		111
#define	CARDTYPE_FN_LAST		112
#define	CARDTYPE_FN_PREFIX		113
#define	CARDTYPE_FN_MIDDLE		114
#define	CARDTYPE_FN_SUFFIX		115
#define	CARDTYPE_N				12
#define	CARDTYPE_NICKNAME		13
#define	CARDTYPE_PHOTO			14
#define	CARDTYPE_BDAY			15

/*
 *	DELIVERY ADDRESSING TYPES
 */
#define	CARDTYPE_ADR			210
#define	CARDTYPE_ADR_OPT		211
#define	CARDTYPE_ADR_OFFICE_BOX	212
#define	CARDTYPE_ADR_EXT_ADDR	213
#define	CARDTYPE_ADR_STREET		214
#define	CARDTYPE_ADR_CITY		215
#define	CARDTYPE_ADR_REGION		216
#define	CARDTYPE_ADR_ZIP		217
#define	CARDTYPE_ADR_COUNTRY	218
#define	CARDTYPE_LABEL			22

/*
 *	TELECOMMUNICATIONS ADDRESSING TYPES
 */
#define	CARDTYPE_TEL			310
#define	CARDTYPE_TEL_OPT		311
#define	CARDTYPE_EMAIL			320
#define	CARDTYPE_EMAIL_OPT		321

/*
 *	EXPLANATORY TYPES
 */
#define	CARDTYPE_UID			670
#define	CARDTYPE_URL			680
#define	CARDTYPE_URL_OPT		681

/*
 *	SECURITY TYPES
 */
#define	CARDTYPE_CLASS			71
#define	CARDTYPE_KEY			72

extern char *buildCard(GSList *list);
extern char *getSingleCardAttribut(int type, char *card);
extern GSList *getMultipleCardAttribut(int type, char *card);
extern ContactCards_pix_t *getCardPhoto(char *card);
extern char *mergeCards(GSList *new, char *old);

#endif	/*	cards_H		*/
