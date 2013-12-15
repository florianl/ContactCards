/*
 *	cards.h
 */

#ifndef	cards_H
#define	cards_H

/*
 *	IDENTIFICATION TYPES
 */
#define	CARDTYPE_FN				11
#define	CARDTYPE_N				12
#define	CARDTYPE_NICKNAME		13

/*
 *	DELIVERY ADDRESSING TYPES
 */
#define	CARDTYPE_ADR			21
#define	CARDTYPE_LABEL			22

/*
 *	TELECOMMUNICATIONS ADDRESSING TYPES
 */
#define	CARDTYPE_TEL			31
#define	CARDTYPE_EMAIL			32

/*
 *	SECURITY TYPES
 */
#define	CARDTYPE_CLASS			71
#define	CARDTYPE_KEY			72

extern char *getSingleCardAttribut(int type, char *card);

#endif	/*	cards_H		*/
