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

#include "contactcards.h"

/**
 * getUID - create a UID for a new vCard
 *
 * Based on the comment by Andrew Moore on
 * http://www.php.net/manual/en/function.uniqid.php#94959
 */
char *getUID(void){
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

/**
 * buildAdr - create the address field of a vCard
 *
 * RFC 2426 - 3.2.1 ADR Type Definition:
 *
 * The structured type value corresponds, in sequence, to
 * the post office box;
 * the extended address;
 * the street address;
 * the locality (e.g., city);
 * the region (e.g., state or province);
 * the postal code;
 * the country name.
 */
static char *buildAdr(GSList *list){
	printfunc(__func__);

	char				*adr = "";
	char				*poBox = NULL;
	char				*extAdr = NULL;
	char				*str = NULL;
	char				*loc = NULL;
	char				*reg = NULL;
	char				*zip = NULL;
	char				*country = NULL;
	GSList				*next;
	GString				*tmp;

	tmp = g_string_new(NULL);

	g_string_append(tmp, "ADR");

	while(list){
		ContactCards_item_t		*item;
		next = list->next;

		if(!list->data){
			goto stepForward;
		}
		item = (ContactCards_item_t *)list->data;
		switch(item->itemID){
			case CARDTYPE_ADR_OPT:
				if(!gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(item->element)))
					break;
				g_string_append(tmp,";TYPE=");
				g_string_append(tmp, gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(item->element)));
				break;
			case CARDTYPE_ADR_OFFICE_BOX:
				if(gtk_entry_buffer_get_length(GTK_ENTRY_BUFFER(item->element)) == 0)
					break;
				poBox = (char *) gtk_entry_buffer_get_text(GTK_ENTRY_BUFFER(item->element));
				break;
			case CARDTYPE_ADR_EXT_ADDR:
				if(gtk_entry_buffer_get_length(GTK_ENTRY_BUFFER(item->element)) == 0)
					break;
				extAdr = (char *) gtk_entry_buffer_get_text(GTK_ENTRY_BUFFER(item->element));
				break;
			case CARDTYPE_ADR_STREET:
				if(gtk_entry_buffer_get_length(GTK_ENTRY_BUFFER(item->element)) == 0)
					break;
				str = (char *) gtk_entry_buffer_get_text(GTK_ENTRY_BUFFER(item->element));
				break;
			case CARDTYPE_ADR_CITY:
				if(gtk_entry_buffer_get_length(GTK_ENTRY_BUFFER(item->element)) == 0)
					break;
				loc = (char *) gtk_entry_buffer_get_text(GTK_ENTRY_BUFFER(item->element));
				break;
			case CARDTYPE_ADR_REGION:
				if(gtk_entry_buffer_get_length(GTK_ENTRY_BUFFER(item->element)) == 0)
					break;
				reg = (char *) gtk_entry_buffer_get_text(GTK_ENTRY_BUFFER(item->element));
				break;
			case CARDTYPE_ADR_ZIP:
				if(gtk_entry_buffer_get_length(GTK_ENTRY_BUFFER(item->element)) == 0)
					break;
				zip = (char *) gtk_entry_buffer_get_text(GTK_ENTRY_BUFFER(item->element));
				break;
			case CARDTYPE_ADR_COUNTRY:
				if(gtk_entry_buffer_get_length(GTK_ENTRY_BUFFER(item->element)) == 0)
					break;
				country = (char *) gtk_entry_buffer_get_text(GTK_ENTRY_BUFFER(item->element));
				break;
		}
stepForward:
		list = next;
	}

	if(!poBox && !extAdr && !str && !loc && !reg && !zip && !country)
		goto stepEmpty;

	g_string_append(tmp, ":");
	if(poBox)
		g_string_append(tmp, poBox);
	g_string_append(tmp, ";");
	if(extAdr)
		g_string_append(tmp, extAdr);
	g_string_append(tmp, ";");
	if(str)
		g_string_append(tmp, str);
	g_string_append(tmp, ";");
	if(loc)
		g_string_append(tmp, loc);
	g_string_append(tmp, ";");
	if(reg)
		g_string_append(tmp, reg);
	g_string_append(tmp, ";");
	if(zip)
		g_string_append(tmp, zip);
	g_string_append(tmp, ";");
	if(country)
		g_string_append(tmp, country);
	g_string_append(tmp, "\r\n");
	adr = g_strndup(tmp->str, tmp->len);

	g_string_free(tmp, TRUE);
	g_free(poBox);
	g_free(extAdr);
	g_free(str);
	g_free(loc);
	g_free(reg);
	g_free(zip);
	g_free(country);

stepEmpty:
	return adr;
}

/**
 * buildSingleLine - builds a single line for a vCard
 */
static char *buildSingleLine(int type, GSList *list){
	printfunc(__func__);

	char				*line = "";
	GSList				*next;
	GString				*tmp;

	tmp = g_string_new(NULL);

	switch(type){
		case CARDTYPE_TEL:
			g_string_append(tmp, "TEL");
			break;
		case CARDTYPE_EMAIL:
			g_string_append(tmp, "EMAIL");
			break;
		case CARDTYPE_URL:
			g_string_append(tmp, "URL");
			break;
		case CARDTYPE_IMPP:
			g_string_append(tmp, "IMPP");
			break;
		case CARDTYPE_NOTE:
			g_string_append(tmp, "NOTE");
			break;
		default:
			return line;
	}

	while(list){
		ContactCards_item_t		*item;
		next = list->next;

		if(!list->data){
			goto stepForward;
		}
		item = (ContactCards_item_t *)list->data;
		if(item->itemID == (type +1)){
				if(!gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(item->element)))
					break;
				g_string_append(tmp,";TYPE=");
				g_string_append(tmp, gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(item->element)));
		} else {
				g_string_append(tmp,":");
				if(type == CARDTYPE_NOTE){
					GtkTextIter		start, end;
					gtk_text_buffer_get_bounds(GTK_TEXT_BUFFER(item->element), &start, &end);
					g_string_append(tmp, g_strescape(gtk_text_buffer_get_text(GTK_TEXT_BUFFER(item->element), &start, &end, FALSE), NULL));
				} else {
					if(gtk_entry_buffer_get_length(GTK_ENTRY_BUFFER(item->element)) == 0)
						goto stepEmpty;
					g_string_append(tmp, gtk_entry_buffer_get_text (GTK_ENTRY_BUFFER(item->element)));
				}
		}
stepForward:
		list = next;
	}

	g_string_append(tmp, "\r\n");
	line = g_strndup(tmp->str, tmp->len);

	g_string_free(tmp, TRUE);

stepEmpty:
	return line;
}

/**
 * buildCard - create a RFC conform vCard from the user input
 */
char *buildCard(GSList *list){
	printfunc(__func__);

	char				*card = NULL;
	char				*firstN , *lastN, *middleN, *prefixN, *suffixN;
	int					bDay, bMonth, bYear;
	char				*bDate;
	GSList				*next;
	GString				*cardString;

	cardString = g_string_new(NULL);

	firstN = lastN = middleN = prefixN = suffixN = NULL;

	g_string_append(cardString, "BEGIN:VCARD\r\n");
	g_string_append(cardString, "VERSION:4.0\r\n");

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
		switch(item->itemID){
			case CARDTYPE_ADR:
				g_string_append(cardString, buildAdr(item->element));
				break;
			case CARDTYPE_TEL:
				g_string_append(cardString, buildSingleLine(CARDTYPE_TEL, item->element));
				break;
			case CARDTYPE_FN_FIRST:
				firstN = g_strstrip((char *)gtk_entry_buffer_get_text(GTK_ENTRY_BUFFER(item->element)));
				break;
			case CARDTYPE_FN_LAST:
				lastN = g_strstrip((char *)gtk_entry_buffer_get_text(GTK_ENTRY_BUFFER(item->element)));
				break;
			case CARDTYPE_FN_PREFIX:
				prefixN = g_strstrip((char *)gtk_entry_buffer_get_text(GTK_ENTRY_BUFFER(item->element)));
				break;
			case CARDTYPE_FN_MIDDLE:
				middleN = g_strstrip((char *)gtk_entry_buffer_get_text(GTK_ENTRY_BUFFER(item->element)));
				break;
			case CARDTYPE_FN_SUFFIX:
				suffixN = g_strstrip((char *)gtk_entry_buffer_get_text(GTK_ENTRY_BUFFER(item->element)));
				break;
			case CARDTYPE_BDAY:
				gtk_calendar_get_date(GTK_CALENDAR(item->element), &bYear, &bMonth, &bDay);
				bMonth++;
				bDate = g_strdup_printf("%04d-%02d-%02d", bYear, bMonth, bDay);
				g_string_append(cardString, "BDAY:");
				g_string_append(cardString, bDate);
				g_string_append(cardString, "\r\n");
				break;
			case CARDTYPE_EMAIL:
				g_string_append(cardString, buildSingleLine(CARDTYPE_EMAIL, item->element));
				break;
			case CARDTYPE_URL:
				g_string_append(cardString, buildSingleLine(CARDTYPE_URL, item->element));
				break;
			case CARDTYPE_IMPP:
				g_string_append(cardString, buildSingleLine(CARDTYPE_IMPP, item->element));
				break;
			case CARDTYPE_NOTE:
				g_string_append(cardString, buildSingleLine(CARDTYPE_NOTE, item->element));
				break;
			default:
				verboseCC("[%s] %s\n", __func__, g_strstrip((char *)gtk_entry_buffer_get_text(GTK_ENTRY_BUFFER(item->element))));
				break;
		}
stepForward:
		list = next;
	}

/*
 * RFC 2426 - 3.1.2 N Type Definition :
 *
 * The structured type value corresponds, in sequence, to
 * the Family Name,
 * Given Name,
 * Additional Names,
 * Honorific Prefixes, and
 * Honorific Suffixes
 */
	g_string_append(cardString, "N:");
	g_string_append(cardString, lastN);
	g_string_append(cardString, ";");
	g_string_append(cardString, firstN);
	g_string_append(cardString, ";");
	g_string_append(cardString, middleN);
	g_string_append(cardString, ";");
	g_string_append(cardString, prefixN);
	g_string_append(cardString, ";");
	g_string_append(cardString, suffixN);
	g_string_append(cardString, "\r\n");

	g_string_append(cardString, "FN:");
	g_string_append(cardString, prefixN);
	g_string_append(cardString, " ");
	g_string_append(cardString, firstN);
	g_string_append(cardString, " ");
	g_string_append(cardString, middleN);
	g_string_append(cardString, " ");
	g_string_append(cardString, lastN);
	g_string_append(cardString, " ");
	g_string_append(cardString, suffixN);
	g_string_append(cardString, "\r\n");

	g_string_append(cardString, "END:VCARD\r\n");

	card = g_strndup(cardString->str, cardString->len);

	g_free(firstN);
	g_free(lastN);
	g_string_free(cardString, TRUE);

	return card;
}

/**
 * getTypeValue(element[0])
 */
static unsigned long getAttributType(char *line){
	printfunc(__func__);

	char				**attributes = g_strsplit(line, ";", 0);
	char				**attribute = attributes;
	unsigned long		type = 0;

	while (*attribute != NULL) {
		char 		*tmp = g_utf8_strdown(*attribute, -1);

		if(g_str_has_prefix(tmp, "type=") == TRUE){

			if(g_str_has_suffix(tmp, "work") == TRUE)
				type |= TYPE_WORK;
			if(g_str_has_suffix(tmp, "home") == TRUE)
				type |= TYPE_HOME;
			if(g_str_has_suffix(tmp, "text") == TRUE)
				type |= TYPE_TEL_TEXT;
			if(g_str_has_suffix(tmp, "voice") == TRUE)
				type |= TYPE_TEL_VOICE;
			if(g_str_has_suffix(tmp, "fax") == TRUE)
				type |= TYPE_TEL_FAX;
			if(g_str_has_suffix(tmp, "cell") == TRUE)
				type |= TYPE_TEL_CELL;
			if(g_str_has_suffix(tmp, "video") == TRUE)
				type |= TYPE_TEL_VIDEO;
			if(g_str_has_suffix(tmp, "pager") == TRUE)
				type |= TYPE_TEL_PAGER;
			if(g_str_has_suffix(tmp, "textphone") == TRUE)
				type |= TYPE_TEL_TEXTPHONE;
			if(g_str_has_suffix(tmp, "contact") == TRUE)
				type |= TYPE_RELATED_CONTACT;
			if(g_str_has_suffix(tmp, "acquaintance") == TRUE)
				type |= TYPE_RELATED_ACQUAINTANCE;
			if(g_str_has_suffix(tmp, "friend") == TRUE)
				type |= TYPE_RELATED_FRIEND;
			if(g_str_has_suffix(tmp, "met") == TRUE)
				type |= TYPE_RELATED_MET;
			if(g_str_has_suffix(tmp, "co-worker") == TRUE)
				type |= TYPE_RELATED_CO_WORKER;
			if(g_str_has_suffix(tmp, "colleague") == TRUE)
				type |= TYPE_RELATED_COLLEAGUE;
			if(g_str_has_suffix(tmp, "co-resident") == TRUE)
				type |= TYPE_RELATED_CO_RESIDENT;
			if(g_str_has_suffix(tmp, "neighbor") == TRUE)
				type |= TYPE_RELATED_NEIGHBOR;
			if(g_str_has_suffix(tmp, "child") == TRUE)
				type |= TYPE_RELATED_CHILD;
			if(g_str_has_suffix(tmp, "parent") == TRUE)
				type |= TYPE_RELATED_PARENT;
			if(g_str_has_suffix(tmp, "sibling") == TRUE)
				type |= TYPE_RELATED_SIBLING;
			if(g_str_has_suffix(tmp, "spouse") == TRUE)
				type |= TYPE_RELATED_SPOUSE;
			if(g_str_has_suffix(tmp, "kin") == TRUE)
				type |= TYPE_RELATED_KIN;
			if(g_str_has_suffix(tmp, "muse") == TRUE)
				type |= TYPE_RELATED_MUSE;
			if(g_str_has_suffix(tmp, "crush") == TRUE)
				type |= TYPE_RELATED_CRUSH;
			if(g_str_has_suffix(tmp, "date") == TRUE)
				type |= TYPE_RELATED_DATE;
			if(g_str_has_suffix(tmp, "sweetheart") == TRUE)
				type |= TYPE_RELATED_SWEETHEART;
			if(g_str_has_suffix(tmp, "me") == TRUE)
				type |= TYPE_RELATED_ME;
			if(g_str_has_suffix(tmp, "agent") == TRUE)
				type |= TYPE_RELATED_AGENT;
			if(g_str_has_suffix(tmp, "emergency") == TRUE)
				type |= TYPE_RELATED_EMERGENCY;
		}
		g_free(tmp);
		attribute++;
	}

	g_strfreev(attributes);

	return type;
}

/**
 * getAttributValueWithType - split the line of a vCard in its elements and return the value with its type
 */
static ContactCards_item_t *getAttributValueWithType(char *line){
	printfunc(__func__);

	char						**elements = g_strsplit(line, ":", 2);
	char						**element = elements;
	ContactCards_item_t			*item;
	unsigned long				type = 0;
	char						*attr = NULL;

	item = g_new(ContactCards_item_t, 1);

	type = getAttributType(element[0]);
	attr = g_strdup(element[1]);

	item->itemID = type;
	item->element = attr;

	g_strfreev(elements);

	return item;
}

/**
 * getAttributValue - split the line of a vCard in its elements and return the value
 */
static char *getAttributValue(char *line){
	printfunc(__func__);

	char				**elements = g_strsplit(line, ":", 2);
	char				**element = elements;
	char				*attr = NULL;

	attr = g_strdup(element[1]);

	g_strfreev(elements);

	return attr;
}

/**
 * getMultipleCardAttribut - return a vCard element which can occur multiple times
 */
GSList *getMultipleCardAttribut(int key, char *card, gboolean attribute){
	printfunc(__func__);

	GSList				*list = g_slist_alloc();
	char				**lines = g_strsplit(card, "\n", 0);
	char				**line = lines;
	void				*value = NULL;

	while (*line != NULL) {
		switch(key){
			case CARDTYPE_ADR:
				if(g_regex_match_simple ("^(item)?ADR", *line, 0,0))
					goto getValue;
				else
					goto next;
			case CARDTYPE_TEL:
				if(g_regex_match_simple ("^(item)?TEL", *line, 0,0))
					goto getValue;
				else
					goto next;
			case CARDTYPE_URL:
				if(g_regex_match_simple ("^(item)?URL", *line, 0,0))
					goto getValue;
				else
					goto next;
			case CARDTYPE_EMAIL:
				if(g_regex_match_simple ("^(item)?EMAIL", *line, 0,0))
					goto getValue;
				else
					goto next;
			case CARDTYPE_SOURCE:
				if(g_regex_match_simple ("^(item)?SOURCE", *line, 0,0))
					goto getValue;
				else
					goto next;
			case CARDTYPE_NICKNAME:
				if(g_regex_match_simple ("^(item)?NICKNAME", *line, 0,0))
					goto getValue;
				else
					goto next;
			case CARDTYPE_IMPP:
				if(g_regex_match_simple ("^(item)?IMPP", *line, 0,0))
					goto getValue;
				else
					goto next;
			case CARDTYPE_LANG:
				if(g_regex_match_simple ("^(item)?LANG", *line, 0,0))
					goto getValue;
				else
					goto next;
			case CARDTYPE_TZ:
				if(g_regex_match_simple ("^(item)?TZ", *line, 0,0))
					goto getValue;
				else
					goto next;
			case CARDTYPE_GEO:
				if(g_regex_match_simple ("^(item)?GEO", *line, 0,0))
					goto getValue;
				else
					goto next;
			case CARDTYPE_TITLE:
				if(g_regex_match_simple ("^(item)?TITLE", *line, 0,0))
					goto getValue;
				else
					goto next;
			case CARDTYPE_ROLE:
				if(g_regex_match_simple ("^(item)?ROLE", *line, 0,0))
					goto getValue;
				else
					goto next;
			case CARDTYPE_LOGO:
				if(g_regex_match_simple ("^(item)?LOGO", *line, 0,0))
					goto getValue;
				else
					goto next;
			case CARDTYPE_ORG:
				if(g_regex_match_simple ("^(item)?ORG", *line, 0,0))
					goto getValue;
				else
					goto next;
			case CARDTYPE_MEMBER:
				if(g_regex_match_simple ("^(item)?MEMBER", *line, 0,0))
					goto getValue;
				else
					goto next;
			case CARDTYPE_RELATED:
				if(g_regex_match_simple ("^(item)?RELATED", *line, 0,0))
					goto getValue;
				else
					goto next;
			case CARDTYPE_CATEGORIES:
				if(g_regex_match_simple ("^(item)?CATEGORIES", *line, 0,0))
					goto getValue;
				else
					goto next;
			case CARDTYPE_NOTE:
				if(g_regex_match_simple ("^(item)?NOTE", *line, 0,0))
					goto getValue;
				else
					goto next;
			case CARDTYPE_SOUND:
				if(g_regex_match_simple ("^(item)?SOUND", *line, 0,0))
					goto getValue;
				else
					goto next;
			default:
				goto next;
		}
		getValue:
			if(attribute == TRUE){
				value = getAttributValueWithType(*line);
			} else {
				value = getAttributValue(*line);
			}
			list = g_slist_append(list, value);
		next:
			line++;
	}

	g_strfreev(lines);
	return list;
}

/**
 * getCardPhoto - return the photo of a vCard
 */
ContactCards_pix_t *getCardPhoto(char *card){
	printfunc(__func__);

	char				*start = g_strrstr(card, "PHOTO");
	GString				*buf;
	GString				*tmp;
	guchar				*pix = NULL;
	gsize				len;
	int					i = 0;
	int					j = 0;
	ContactCards_pix_t	*pic;

	pic = g_new(ContactCards_pix_t,1);

	pic->pixel	= NULL;
	pic->size	= 0;

	if(start == NULL){
		return pic;
	}

	while(start[i] != ':')
		i++;
	i++;	/* Set i to the point after the :	*/

	buf = g_string_new(NULL);
	tmp = g_string_new(NULL);

	while(start[i+j] != ':')
		g_string_append_unichar(buf, start[i+(j++)]);

	/* Remove unnecessary stuff	*/
	i = buf->len;

	while(buf->str[i] != '\n')
		i--;
	g_string_truncate(buf, i);

	i = 0;
	j = 1;
	while(buf->str[i])
	{
		switch(buf->str[i]){
			case 43:			/*	+		*/
			case 47:			/*	/		*/
			case 48 ... 57:		/*	0-9		*/
			case 61:			/*	=		*/
			case 65 ... 90:		/*	A-Z		*/
			case 97 ... 122:	/*	a-z		*/
				g_string_append_unichar(tmp, buf->str[i]);
				if(j%79 == 0 && j != 1)
					g_string_append_unichar(tmp, '\n');
				break;
			default:
				break;
		}
		i++;
		j++;
	}
	pix = g_base64_decode(tmp->str, &len);
	pic->pixel = pix;
	pic->size = (int) len;

	g_string_free(buf, TRUE);
	g_string_free(tmp, TRUE);

	return pic;
}

/**
 * getSingleCardAttribut - return a vCard element which can occur only once
 */
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
				if(g_str_has_prefix(*line, "N:"))
					goto getValue;
				else
					goto next;
			case CARDTYPE_UID:
				if(g_str_has_prefix(*line, "UID"))
					goto getValue;
				else
					goto next;
			case CARDTYPE_BDAY:
				if(g_str_has_prefix(*line, "BDAY"))
					goto getValue;
				else
					goto next;
			case CARDTYPE_KIND:
				if(g_str_has_prefix(*line, "KIND"))
					goto getValue;
				else
					goto next;
			case CARDTYPE_ANNIVERSARY:
				if(g_str_has_prefix(*line, "ANNIVERSARY"))
					goto getValue;
				else
					goto next;
			case CARDTYPE_GENDER:
				if(g_str_has_prefix(*line, "GENDER"))
					goto getValue;
				else
					goto next;
			case CARDTYPE_SHOWAS:
				if(g_str_has_prefix(*line, "X-ABSHOWAS"))
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

	g_strfreev(lines);
	return value;
}

/**
 * replaceAntiquatedLine - replaces a line of a vCard with a new value
 * This function only works for values which occur only once
 */
static char *replaceAntiquatedLine(char *vCard, char *pattern, char *newLine){
	printfunc(__func__);

	GString		*data;
	const char	*end;
	const char *p = vCard;

	unsigned int	i = 0;
	int			k = 0;
	int			sPos = 0;

	data = g_string_new(NULL);
	data = g_string_assign(data, vCard);

	end = vCard + strlen(vCard) - strlen(pattern);

	sPos = 0;
	while(p <= end && *p){
		for(i=0; i < strlen(pattern); i++){
			if(p[i] != pattern[i]){
				goto nextLoop;
			}
		}
		break;
nextLoop:
		p++;
		sPos++;
	}

	if(i == (strlen(vCard) - strlen(pattern))){
		/* didn't find it	*/
		return vCard;
	}
	if(pattern[0] == '\n')
		sPos++;

	/*	sPos has to be before END:VCARD	*/
	if(sPos > ((int) strlen(vCard) - 11)){
		sPos = (int) strlen(vCard) - 11;
	} else {
		k= 0;
		while(vCard[sPos + k] != '\n')
			k++;
		k++;
		g_string_erase(data, sPos, k);
	}

	g_string_insert(data, sPos, newLine);

	return data->str;
}

/**
 * removeGroupMember - remove private group member of a value
 */
GString *removeGroupMember(GString *data, GString *group){
	printfunc(__func__);

	char			*p = NULL;
	char			*end = NULL;
	int				i = 0 ;
	int				sPos = 0;
	int				ePos = 0;

	p = g_strstr_len(data->str, data->len, group->str);

	while(p){
		end = data->str + data->len;
		while(p <= end && *p) {
			i++;
			p++;
		}
		sPos = data->len - i;

		for(i=0;data->str[sPos+1+i] != '\n';i++);
		ePos = sPos + 1 +i;

		data = g_string_erase(data, sPos, ePos-sPos);

		p = NULL;
		p = g_strstr_len(data->str, data->len, group->str);
	}

	return data;
}

/**
 * removeValue - remove a value from vCard
 */
static char *removeValue(char *card, char *value){
	printfunc(__func__);

	char			*new = NULL;
	char			*p;
	char			*end;
	GString			*data;
	GString			*group;

	unsigned int	i = 0;
	int				sPos = 0;
	int				ePos = 0;
	int				isGroup = 0;

	data = g_string_new(NULL);
	data = g_string_assign(data, card);

	p = g_strstr_len(card, strlen(card), value);

	if(!p){
		/*	didn't find value in card	*/
		return card;
	}
	end = card + strlen(card);
	while(p <= end && *p) {
		i++;
		p++;
	}

	sPos = data->len - i;

	for(i=0;(data->str[sPos-i] != '\n') && (sPos-i > 0) ;i++){
		/* search for group members	*/
		if(data->str[sPos-i] == '.'){
			isGroup = 1;
		}
	}
	sPos = sPos -i;

	for(i=0;data->str[sPos+1+i] != '\n';i++);
	ePos = sPos + 1 +i;

	if(sPos == 0)
		ePos++;

	if(isGroup){
		group = g_string_new(NULL);
		for(i=0; data->str[sPos+i] != '.' ;i++)
			group = g_string_append_c(group, data->str[sPos+i]);
		group = g_string_append_c(group, '.');
	}

	data = g_string_erase(data, sPos, ePos-sPos);

	if(isGroup){
		data = removeGroupMember(data, group);
		g_string_free(group, TRUE);
	}

	new = g_strdup(data->str);
	g_free(card);
	g_string_free(data, TRUE);

	return new;
}

/**
 * valueCmp - compares the values of two lists
 */
int valueCmp(gconstpointer a, gconstpointer b){
	printfunc(__func__);

	return g_strcmp0((char *)a, (char *)b);
}

static inline int findData(GSList *list, char *data){
	printfunc(__func__);

	int			ret = 0;

	while(list){
		GSList				*next = list->next;
		char				*value = list->data;
		if(value != NULL){
			if(g_strstr_len(value, strlen(value), data) != NULL){
				return 1;
			}
		}
		list = next;
	}

	return ret;
}

/**
 * mergeMultipleItems - merges items, which can occur multiple times, into
 * the vCard
 */
char *mergeMultipleItems(char *old, char *new){
	printfunc(__func__);

	GSList				*present;
	GSList				*future;
	GString				*data;

	/*	Phone	*/
	present = getMultipleCardAttribut(CARDTYPE_TEL, old, FALSE);
	future = getMultipleCardAttribut(CARDTYPE_TEL, new, FALSE);
	if (g_slist_length(present) > 1){
		while(present){
				GSList				*next = present->next;
				char				*value = (char *) present->data;
				if(value != NULL){
					if(findData(future, value) == 1){
						new = removeValue(new, value);
					} else {
						old = removeValue(old, value);
					}
				}
				present = next;
		}
	}
	g_slist_free_full(future, g_free);
	g_slist_free_full(present, g_free);

	/*	Url	*/
	present = getMultipleCardAttribut(CARDTYPE_URL, old, FALSE);
	future = getMultipleCardAttribut(CARDTYPE_URL, new, FALSE);
	if (g_slist_length(present) > 1){
		while(present){
				GSList				*next = present->next;
				char				*value = (char *) present->data;
				if(value != NULL){
					if(findData(future, value) == 1){
						new = removeValue(new, value);
					} else {
						old = removeValue(old, value);
					}
				}
				present = next;
		}
	}
	g_slist_free_full(future, g_free);
	g_slist_free_full(present, g_free);

	/*	EMail	*/
	present = getMultipleCardAttribut(CARDTYPE_EMAIL, old, FALSE);
	future = getMultipleCardAttribut(CARDTYPE_EMAIL, new, FALSE);
	if (g_slist_length(present) > 1){
		while(present){
				GSList				*next = present->next;
				char				*value = (char *) present->data;
				if(value != NULL){
					if(findData(future, value) == 1){
						new = removeValue(new, value);
					} else {
						old = removeValue(old, value);
					}
				}
				present = next;
		}
	}
	g_slist_free_full(future, g_free);
	g_slist_free_full(present, g_free);

	/*	Postal Address	*/
	present = getMultipleCardAttribut(CARDTYPE_ADR, old, FALSE);
	future = getMultipleCardAttribut(CARDTYPE_ADR, new, FALSE);
	if (g_slist_length(present) > 1){
		while(present){
				GSList				*next = present->next;
				char				*value = (char *) present->data;
				if(value != NULL){
					if(findData(future, value) == 1){
						new = removeValue(new, value);
					} else {
						old = removeValue(old, value);
					}
				}
				present = next;
		}
	}
	g_slist_free_full(future, g_free);
	g_slist_free_full(present, g_free);

	/*	Note	*/
	present = getMultipleCardAttribut(CARDTYPE_NOTE, old, FALSE);
	future = getMultipleCardAttribut(CARDTYPE_NOTE, new, FALSE);
	if (g_slist_length(present) > 1){
		while(present){
				GSList				*next = present->next;
				char				*value = (char *) present->data;
				if(value != NULL){
					if(findData(future, value) == 1){
						new = removeValue(new, value);
					} else {
						old = removeValue(old, value);
					}
				}
				present = next;
		}
	}
	g_slist_free_full(future, g_free);
	g_slist_free_full(present, g_free);

	data = g_string_new(NULL);
	data = g_string_assign(data, old);
	g_free(old);
	data = g_string_insert(data, data->len - 11, new);

	old = g_strdup(data->str);
	g_string_free(data, TRUE);

	return old;
}

/**
 * mergeCards - merge the old vCard from the database with the new changes
 * This function is needed to keep the stuff alive which is not
 * displayed and editable so far
 */
char *mergeCards(GSList *new, char *old){
	printfunc(__func__);

	char			*firstN , *lastN, *middleN, *prefixN, *suffixN;
	GSList			*next;
	GString			*value, *cmp;

	firstN = lastN = middleN = prefixN = suffixN = NULL;

	cmp = g_string_new(NULL);

	while(new){
		ContactCards_item_t		*item;
		next = new->next;
		if(!new->data)
			goto stepForward;
		item = (ContactCards_item_t *)new->data;
		value = g_string_new(NULL);
		switch(item->itemID){
			case CARDTYPE_FN_FIRST:
				firstN = g_strstrip((char *)gtk_entry_buffer_get_text(GTK_ENTRY_BUFFER(item->element)));
				break;
			case CARDTYPE_FN_LAST:
				lastN = g_strstrip((char *)gtk_entry_buffer_get_text(GTK_ENTRY_BUFFER(item->element)));
				break;
			case CARDTYPE_FN_PREFIX:
				prefixN = g_strstrip((char *)gtk_entry_buffer_get_text(GTK_ENTRY_BUFFER(item->element)));
				break;
			case CARDTYPE_FN_MIDDLE:
				middleN = g_strstrip((char *)gtk_entry_buffer_get_text(GTK_ENTRY_BUFFER(item->element)));
				break;
			case CARDTYPE_FN_SUFFIX:
				suffixN = g_strstrip((char *)gtk_entry_buffer_get_text(GTK_ENTRY_BUFFER(item->element)));
				break;
			case CARDTYPE_ADR:
				g_string_append(cmp, buildAdr(item->element));
				break;
			case CARDTYPE_TEL:
				g_string_append(cmp, buildSingleLine(CARDTYPE_TEL, item->element));
				break;
			case CARDTYPE_EMAIL:
				g_string_append(cmp, buildSingleLine(CARDTYPE_EMAIL, item->element));
				break;
			case CARDTYPE_URL:
				g_string_append(cmp, buildSingleLine(CARDTYPE_URL, item->element));
				break;
			case CARDTYPE_IMPP:
				g_string_append(cmp, buildSingleLine(CARDTYPE_IMPP, item->element));
				break;
			case CARDTYPE_NOTE:
				g_string_append(cmp, buildSingleLine(CARDTYPE_NOTE, item->element));
				break;
			default:
				break;
		}
		g_string_free(value, TRUE);
stepForward:
		new = next;
	}

	value = g_string_new(NULL);
	g_string_append(value, "N:");
	g_string_append(value, lastN);
	g_string_append(value, ";");
	g_string_append(value, firstN);
	g_string_append(value, ";");
	g_string_append(value, middleN);
	g_string_append(value, ";");
	g_string_append(value, prefixN);
	g_string_append(value, ";");
	g_string_append(value, suffixN);
	g_string_append(value, "\r\n");

	if(g_strstr_len(old, -1, value->str) == NULL)
	{
		old = replaceAntiquatedLine(old, "\nN:", value->str);
		g_string_free(value, TRUE);

		value = g_string_new(NULL);
		g_string_append(value, "FN:");
		g_string_append(value, prefixN);
		g_string_append(value, " ");
		g_string_append(value, firstN);
		g_string_append(value, " ");
		g_string_append(value, middleN);
		g_string_append(value, " ");
		g_string_append(value, lastN);
		g_string_append(value, " ");
		g_string_append(value, suffixN);
		g_string_append(value, "\r\n");

		old = replaceAntiquatedLine(old, "\nFN:", value->str);
		g_string_free(value, TRUE);
	}

	value = g_string_new(NULL);
	g_string_append(value, "PRODID:-//ContactCards//ContactCards");
	g_string_append(value, VERSION);
	g_string_append(value, "//EN\r\n");
	old = replaceAntiquatedLine(old, "\nPRODID:", value->str);
	g_string_free(value, TRUE);

	old = mergeMultipleItems(old, cmp->str);
	g_string_free(cmp, FALSE);

	return old;
}

/**
 * validateCard - validates a single vCard
 */
char *validateCard(char *card){
	printfunc(__func__);

	char			*vcf = NULL;
	char			**lines = g_strsplit(card, "\n", -1);
	char			**line = lines;
	unsigned int	valid = 0;

	/*	The last END:VCARD was lost at g_strsplit() in validateFile()	*/
	valid ^= HAVE_END;

	if(*line != NULL){
		if(g_str_has_prefix(*line, "BEGIN:")){
			valid ^= HAVE_BEGIN;
			line++;
		} else{
			valid = FALSE;
			g_strfreev(lines);
			return vcf;
		}
	} else {
		valid = FALSE;
		g_strfreev(lines);
		return vcf;
	}

	while(*line != NULL){
		if(g_str_has_prefix(*line, "BEGIN:")){
			valid ^= HAVE_BEGIN;
			goto next;
		}
		if(g_str_has_prefix(*line, "FN:")){
			valid ^= HAVE_FN;
			goto next;
		}
		if(g_str_has_prefix(*line, "VERSION:")){
			valid ^= HAVE_VERSION;
			goto next;
		}
		if(g_str_has_prefix(*line, "END:")){
			valid ^= HAVE_END;
			goto next;
		}
next:
		line++;
	}
	g_strfreev(lines);

	if(valid == MUST_BE_MASK){
		/*	Append END:VCARD back to the to string after it was lost at g_strsplit()	*/
		vcf = g_strconcat (card, "END:VCARD\r\n", NULL);
	}

	return vcf;
}

/**
 * validateFile - returns a list of valid vCards
 */
GSList *validateFile(char *content){
	printfunc(__func__);

	GSList				*list = g_slist_alloc();
	char				**cards = g_strsplit(content, "END:VCARD\n", -1);
	char				**card = cards;

	while(*card != NULL){
		char		*new = NULL;
		new = validateCard(*card);
		if(new != NULL){
			list = g_slist_append(list, new);
		}
		card++;
	}
	g_strfreev(cards);

	return list;
}
