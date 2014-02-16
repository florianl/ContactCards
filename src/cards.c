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

/*
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
	g_string_append(tmp, ";");
	g_string_append(tmp, "\n");
	adr = g_strndup(tmp->str, tmp->len);

stepEmpty:
	return adr;
}

static char *buildEMail(GSList *list){
	printfunc(__func__);

	char				*mail = "";
	GSList				*next;
	GString				*tmp;

	tmp = g_string_new(NULL);

	g_string_append(tmp, "EMAIL");

	while(list){
		ContactCards_item_t		*item;
		next = list->next;

		if(!list->data){
			goto stepForward;
		}
		item = (ContactCards_item_t *)list->data;
		switch(item->itemID){
			case CARDTYPE_EMAIL_OPT:
				if(!gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(item->element)))
					break;
				g_string_append(tmp,";TYPE=");
				g_string_append(tmp, gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(item->element)));
				break;
			default:
				if(gtk_entry_buffer_get_length(GTK_ENTRY_BUFFER(item->element)) == 0)
					goto stepEmpty;
				g_string_append(tmp,":");
				g_string_append(tmp, gtk_entry_buffer_get_text (GTK_ENTRY_BUFFER(item->element)));
		}
stepForward:
		list = next;
	}

	g_string_append(tmp, "\n");
	mail = g_strndup(tmp->str, tmp->len);

stepEmpty:
	return mail;
}

static char *buildTele(GSList *list){
	printfunc(__func__);

	char				*tel = "";
	GSList				*next;
	GString				*tmp;

	tmp = g_string_new(NULL);

	g_string_append(tmp, "TEL");

	while(list){
		ContactCards_item_t		*item;
		next = list->next;

		if(!list->data){
			goto stepForward;
		}
		item = (ContactCards_item_t *)list->data;
		switch(item->itemID){
			case CARDTYPE_TEL_OPT:
				if(!gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(item->element)))
					break;
				g_string_append(tmp,";TYPE=");
				g_string_append(tmp, gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(item->element)));
				break;
			default:
				if(gtk_entry_buffer_get_length(GTK_ENTRY_BUFFER(item->element)) == 0)
					goto stepEmpty;
				g_string_append(tmp,":");
				g_string_append(tmp, gtk_entry_buffer_get_text (GTK_ENTRY_BUFFER(item->element)));
		}
stepForward:
		list = next;
	}

	g_string_append(tmp, "\n");
	tel = g_strndup(tmp->str, tmp->len);

stepEmpty:
	return tel;
}

char *buildCard(GSList *list){
	printfunc(__func__);

	char				*card = NULL;
	char				*firstN = NULL;
	char				*lastN = NULL;
	int					bDay, bMonth, bYear;
	char				*bDate;
	GSList				*next;
	GString				*cardString;

	cardString = g_string_new(NULL);

	g_string_append(cardString, "BEGIN:VCARD\n");
	g_string_append(cardString, "VERSION:3.0\n");

	g_string_append(cardString, "PRODID:-//ContactCards//ContactCards");
	g_string_append(cardString, VERSION);
	g_string_append(cardString, "//EN\n");
	g_string_append(cardString, "UID:");
	g_string_append(cardString, getUID());
	g_string_append(cardString, "\n");

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
				g_string_append(cardString, buildTele(item->element));
				break;
			case CARDTYPE_FN_FIRST:
				firstN = g_strstrip((char *)gtk_entry_buffer_get_text(GTK_ENTRY_BUFFER(item->element)));
				break;
			case CARDTYPE_FN_LAST:
				lastN = g_strstrip((char *)gtk_entry_buffer_get_text(GTK_ENTRY_BUFFER(item->element)));
				break;
			case CARDTYPE_BDAY:
				gtk_calendar_get_date(GTK_CALENDAR(item->element), &bYear, &bMonth, &bDay);
				bMonth++;
				bDate = g_strdup_printf("%04d-%02d-%02d", bYear, bMonth, bDay);
				g_string_append(cardString, "BDAY:");
				g_string_append(cardString, bDate);
				g_string_append(cardString, "\n");
				break;
			case CARDTYPE_EMAIL:
				g_string_append(cardString, buildEMail(item->element));
				break;
			case CONTACT_ADD_WINDOW:
			default:
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
	g_string_append(cardString, ";;;");
	g_string_append(cardString, "\n");

	g_string_append(cardString, "FN:");
	g_string_append(cardString, firstN);
	g_string_append(cardString, " ");
	g_string_append(cardString, lastN);
	g_string_append(cardString, "\n");

	g_string_append(cardString, "END:VCARD\n");

	card = g_strndup(cardString->str, cardString->len);

	return card;
}

static char *getAttributValue(char *line){
	printfunc(__func__);

	char				**elements = g_strsplit(line, ":", 2);
	char				**element = elements;
	char				*value = NULL;

	value = element[1];

	return value;
}

GSList *getMultipleCardAttribut(int type, char *card){
	printfunc(__func__);

	GSList				*list = g_slist_alloc();
	char				**lines = g_strsplit(card, "\n", 0);
	char				**line = lines;
	char				*value = NULL;

	while (*line != NULL) {
		switch(type){
			case CARDTYPE_ADR:
				if(g_str_has_prefix(*line, "ADR"))
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
			default:
				goto next;
		}
		getValue:
			value = getAttributValue(*line);
			list = g_slist_append(list, value);
		next:
			line++;
	}

	return list;
}

GString *getCardPhoto(char *card){
	printfunc(__func__);

	char		*start = g_strrstr(card, "PHOTO");
	GString		*buf;
	int			i = 0;
	int			j = 0;

	if(start == NULL){
		return NULL;
	}

	while(start[i] != ':')
		i++;
	i++;	/* Set i to the point after the :	*/

	buf = g_string_new(NULL);
	while(start[i+j] != ':')
		g_string_append_unichar(buf, start[i+(j++)]);

	/* Remove unnecessary stuff	*/
	i = buf->len;

	while(buf->str[i] != '\n')
		i--;
	g_string_truncate(buf, i);

	return buf;
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
			case CARDTYPE_LABEL:
				if(g_str_has_prefix(*line, "LABEL"))
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
