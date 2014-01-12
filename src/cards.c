/*
 *	cards.c
 */

#include "ContactCards.h"

/*
 * Based on the comment by Andrew Moore on
 * http://www.php.net/manual/en/function.uniqid.php#94959
 */

static char *getUID(void){
	printfunc(__func__);

	char		*uid = NULL;
	GRand		*rand = g_rand_new();

	uid = g_strdup_printf("%04x%04x-%04x-%04x-%04x-%04x%04x%04x",
		g_rand_int_range(rand, 0, 0xffff),
		g_rand_int_range(rand, 0, 0xffff),
		g_rand_int_range(rand, 0, 0xffff),
		g_rand_int_range(rand, 0, 0x0fff) | 0x4000,
		g_rand_int_range(rand, 0, 0x3fff) | 0x8000,
		g_rand_int_range(rand, 0, 0xffff),
		g_rand_int_range(rand, 0, 0xffff),
		g_rand_int_range(rand, 0, 0xffff));

	return uid;
}

char *buildCard(GSList *list){
	printfunc(__func__);

	char				*card = NULL;
	GSList				*next;
	GString				*cardString;

	cardString = g_string_new(NULL);

	g_string_append(cardString, "BEGIN:VCARD\n");
	g_string_append(cardString, "VERSION:3.0\n");

	g_string_append(cardString, "PRODID:-//ContactCards//ContactCards");
	g_string_append(cardString, VERSION);
	g_string_append(cardString, "//EN\r\n");
	g_string_append(cardString, "UID:");
	g_string_append(cardString, getUID());
	g_string_append(cardString, "\r\n");

	while(list){
		ContactCards_item_t		*item;
		next = list->next;

		if(!list->data){
			goto stepForward;
		}
		item = (ContactCards_item_t *)list->data;
		dbgCC("> %d\n", item->itemID);
stepForward:
		list = next;
	}

	g_string_append(cardString, "END:VCARD\n");

	card = g_strndup(cardString->str,  cardString->len);

	return card;
}

static char *getAttributValue(char *line){
	printfunc(__func__);

	char				**elements = g_strsplit(line, ":", 2);
	char				**element = elements;
	char				*value = NULL;

	value =  element[1];

	return value;
}

char *getSingleCardAttribut(int type, char *card){
	printfunc(__func__);

	char						**lines = g_strsplit(card, "\n", 0);
	char						**line = lines;
	char						*value = NULL;

	while (*line != NULL) {
		switch(type){
			case CARDTYPE_FN:
				if(g_str_has_prefix(*line, "FN"))
					goto getValue;
				else 
					goto next;
			case CARDTYPE_N:
				if(g_str_has_prefix(*line, "N"))
					goto getValue;
				else 
					goto next;
			case CARDTYPE_NICKNAME:
				if(g_str_has_prefix(*line, "NICKNAME"))
					goto getValue;
				else 
					goto next;
			case CARDTYPE_ADR:
				if(g_str_has_prefix(*line, "ADR"))
					goto getValue;
				else 
					goto next;
			case CARDTYPE_LABEL:
				if(g_str_has_prefix(*line, "LABEL"))
					goto getValue;
				else 
					goto next;
			case CARDTYPE_TEL:
				if(g_str_has_prefix(*line, "TEL"))
					goto getValue;
				else 
					goto next;
			case CARDTYPE_EMAIL:
				if(g_str_has_prefix(*line, "EMAIL"))
					goto getValue;
				else 
					goto next;
			case CARDTYPE_CLASS:
				if(g_str_has_prefix(*line, "CLASS"))
					goto getValue;
				else 
					goto next;
			case CARDTYPE_KEY:
				if(g_str_has_prefix(*line, "KEY"))
					goto getValue;
				else 
					goto next;
			default:
				goto next;
		}
		getValue:
			value = getAttributValue(*line);
			break;
		next:
			line++;
	}

	return value;
}
