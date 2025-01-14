/*
 * F-List Pidgin - a libpurple protocol plugin for F-Chat
 *
 * Copyright 2011 F-List Pidgin developers.
 *
 * This file is part of F-List Pidgin.
 *
 * F-List Pidgin is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * F-List Pidgin is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with F-List Pidgin.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef PURPLE_PLUGINS
  #define PURPLE_PLUGINS
#endif

#include "f-list.h"

//libpurple headers
#include "accountopt.h"
#include "connection.h"
#include "core.h"
#include "debug.h"
#include "dnsquery.h"
#include "proxy.h"
#include "prpl.h"
#include "request.h"
#include "sslconn.h"
#include "version.h"

GSList *gender_list;
GHashTable *str_to_gender;
GHashTable *gender_to_struct;

GSList *status_list;
GHashTable *str_to_status;
GHashTable *status_to_str;
GHashTable *status_to_proper_str;
GHashTable *friend_status_to_proper_str;

GHashTable *str_to_channel_mode;

GHashTable *account_to_string;
GHashTable *string_to_account;

#define FLIST_GENDER_COUNT 8
struct FListGenderStruct_ genders[FLIST_GENDER_COUNT] = {
    { FLIST_GENDER_NONE, "None", "None", "#B8B800", "<font color=\"#B8B800\">None</font>" },
    { FLIST_GENDER_MALE, "Male", "Male", "#6699FF", "<font color=\"#6699FF\">Male</font>" },
    { FLIST_GENDER_FEMALE, "Female", "Female", "#FF6699", "<font color=\"#FF6699\">Female</font>" },
    { FLIST_GENDER_HERM, "Herm", "Herm", "#9B30FF", "<font color=\"#9B30FF\">Herm</font>" },
    { FLIST_GENDER_MALEHERM, "Male-Herm", "Male-Herm", "#007FFF", "<font color=\"#007FFF\">Male-Herm</font>" },
    { FLIST_GENDER_CUNTBOY, "Cunt-boy", "Cunt-boy", "#00CC66", "<font color=\"#00CC66\">Cunt-boy</font>" },
    { FLIST_GENDER_SHEMALE, "Shemale", "Shemale", "#CC66FF", "<font color=\"#CC66FF\">Shemale</font>" },
    { FLIST_GENDER_TRANSGENDER, "Transgender", "Transgender", "#EE8822", "<font color=\"#EE8822\">Transgender</font>" }
};
struct FListGenderStruct_ gender_unknown = { FLIST_GENDER_UNKNOWN, "Unknown", "Unknown", "#FFFFBB", "<font color=\"#FFFFBB\">Unknown</font>" };

static gpointer _flist_lookup(GHashTable *table, gpointer key, gpointer def) {
    gpointer ret;
    if(!g_hash_table_lookup_extended(table, key, NULL, &ret)) return def;
    return ret;
}
GSList *flist_get_gender_list() {
    return gender_list;
}
GSList *flist_get_status_list() {
    return status_list;
}

FListGender flist_parse_gender(const gchar *k) {
    const struct FListGenderStruct_* gender = _flist_lookup(str_to_gender, (gpointer) k, (gpointer) &gender_unknown);
    return (FListGender) gender->gender;
}
const gchar *flist_format_gender(FListGender k) {
    const struct FListGenderStruct_* gender = _flist_lookup(gender_to_struct, (gpointer) k, (gpointer) &gender_unknown);
    return gender->display_name;
}
const gchar *flist_format_gender_color(FListGender k) {
    const struct FListGenderStruct_* gender = _flist_lookup(gender_to_struct, (gpointer) k, (gpointer) &gender_unknown);
    return gender->colored_name;
}

const gchar *flist_gender_color(FListGender k) {
    const struct FListGenderStruct_* gender = _flist_lookup(gender_to_struct, (gpointer) k, (gpointer) &gender_unknown);
    return gender->color;
}

FListChannelMode flist_parse_channel_mode(const gchar *k) {
    return (FListChannelMode) _flist_lookup(str_to_channel_mode, (gpointer) k, (gpointer) CHANNEL_MODE_UNKNOWN);
}
FListStatus flist_parse_status(const gchar *k) {
    return (FListStatus) _flist_lookup(str_to_status, (gpointer) k, (gpointer) FLIST_STATUS_UNKNOWN);
}
const gchar *flist_format_status(FListStatus k) {
    return (const gchar *) _flist_lookup(status_to_proper_str, (gpointer) k, (gpointer) "Unknown");
}
const gchar *flist_internal_status(FListStatus k) {
    return (const gchar *) _flist_lookup(status_to_str, (gpointer) k, (gpointer) "unknown");
}
const gchar *flist_format_friend_status(FListFriendStatus k) {
    return (const gchar *) _flist_lookup(friend_status_to_proper_str, (gpointer) k, (gpointer) "Unknown");
}

static PurpleGroup *flist_get_group(const gchar *name) {
    PurpleGroup *g;
    if(!(g = purple_find_group(name))) {
        g = purple_group_new(name);
    }
    return g;
}
PurpleGroup *flist_get_filter_group(FListAccount *fla) {
    gchar *name = g_strdup_printf("%s - Search", fla->character);
    PurpleGroup *g = flist_get_group(name);
    g_free(name);
    return g;
}
PurpleGroup *flist_get_bookmarks_group(FListAccount *fla) {
    gchar *name = g_strdup_printf("%s - Bookmarks", fla->character);
    PurpleGroup *g = flist_get_group(name);
    g_free(name);
    return g;
}
PurpleGroup *flist_get_friends_group(FListAccount *fla) {
    gchar *name = g_strdup_printf("%s - Friends", fla->character);
    PurpleGroup *g = flist_get_group(name);
    g_free(name);
    return g;
}
PurpleGroup *flist_get_chat_group(FListAccount *fla) {
    gchar *name = g_strdup_printf("%s - Temporary Chats", fla->character);
    PurpleGroup *g = flist_get_group(name);
    g_free(name);
    return g;
}
PurpleGroup *flist_get_im_group(FListAccount *fla) {
    gchar *name = g_strdup_printf("%s - Temporary IMs", fla->character);
    PurpleGroup *g = flist_get_group(name);
    g_free(name);
    return g;
}

const gchar *flist_serialize_account(PurpleAccount *pa) {
    const gchar *name = purple_account_get_username(pa);
    const gchar *ret;
    guint32 rand;
    ret = g_hash_table_lookup(account_to_string, name);

    if(ret) return ret;

    rand = g_random_int();
    ret = g_strdup_printf("%x", rand);
    g_hash_table_insert(account_to_string, (gpointer) name, (gpointer) ret);
    g_hash_table_insert(string_to_account, (gpointer) ret, (gpointer) name);
    return ret;
}

PurpleAccount *flist_deserialize_account(const gchar *lookup) {
    const gchar *name;

    name = g_hash_table_lookup(string_to_account, lookup);
    if(!name) return NULL;

    return purple_accounts_find(name, FLIST_PLUGIN_ID);
}

PurpleTypingState flist_typing_state(const gchar *state) {
    if(!strcmp(state, "clear")) return PURPLE_NOT_TYPING;
    if(!strcmp(state, "paused")) return PURPLE_TYPED;
    if(!strcmp(state, "typing")) return PURPLE_TYPING;
    return PURPLE_NOT_TYPING;
}
const gchar *flist_typing_state_string(PurpleTypingState state) {
    switch(state) {
    case PURPLE_TYPING: return "typing";
    case PURPLE_NOT_TYPING: return "clear";
    case PURPLE_TYPED: return "paused";
    default: return "clear";
    }
}
guint flist_str_hash(const char *nick) {
    char *lc = g_utf8_strdown(nick, -1);
    guint bucket = g_str_hash(lc);
    g_free(lc);
    return bucket;
}
gboolean flist_str_equal(const char *nick1, const char *nick2) {
    return (purple_utf8_strcasecmp(nick1, nick2) == 0);
}

// Those two functions are here because the version of GLIB2 provided by Pidgin
// used to be a bit too old. Now it's still around because the GTK bundle we
// use for the windows build provides an older version of GLIB than the one in
// Pidgin.
void flist_g_slist_free_full(GSList *to_free, GDestroyNotify f) {
    GSList *cur = to_free;
    while(cur) {
        f(cur->data);
        cur = g_slist_next(cur);
    }
    g_slist_free(to_free);
}
void flist_g_list_free_full(GList *to_free, GDestroyNotify f) {
    GList *cur = to_free;
    while(cur) {
        f(cur->data);
        cur = g_list_next(cur);
    }
    g_list_free(to_free);
}

gint flist_pointer_cmp(gconstpointer a, gconstpointer b) {
    if(a > b) return 1;
    if(b > a) return -1;
    return 0;
}
/** Runs in O(n log n) (if g_slist_sort is reasonable) */
GSList *flist_g_slist_intersect_and_free(GSList *list1, GSList *list2) {
    GSList *ret = NULL;
    GSList *cur1, *cur2;
    list1 = g_slist_sort(list1, (GCompareFunc) flist_pointer_cmp);
    list2 = g_slist_sort(list2, (GCompareFunc) flist_pointer_cmp);
    cur1 = list1;
    cur2 = list2;
    while(cur1 && cur2) {
        if(cur1->data == cur2->data) { /* we have a match! */
            ret = g_slist_prepend(ret, cur1->data);
            cur1 = g_slist_next(cur1);
            cur2 = g_slist_next(cur2);
        } else {
            if(cur1->data > cur2->data) {
                cur2 = g_slist_next(cur2);
            } else {
                cur1 = g_slist_next(cur1);
            }
        }
    }
    g_slist_free(list1);
    g_slist_free(list2);
    return ret;
}

void flist_enable_signals(FListAccount *fla)
{
    void *conv_handle = purple_conversations_get_handle();
    purple_signal_connect(conv_handle, "conversation-created", fla,
            PURPLE_CALLBACK(flist_conversation_created_cb), fla);
    // We need to connect this signal at the lowest priority so it gets called
    // before any other plugin (such as OTR) has a chance to alter it
    purple_signal_connect_priority(conv_handle, "sending-im-msg", fla,
            PURPLE_CALLBACK(flist_process_sending_im), fla,
            PURPLE_SIGNAL_PRIORITY_LOWEST );
    purple_signal_connect_priority(conv_handle, "receiving-im-msg", fla,
            PURPLE_CALLBACK(flist_process_receiving_im), fla,
            PURPLE_SIGNAL_PRIORITY_HIGHEST );
}

void flist_disable_signals(FListAccount *fla)
{
    void *conv_handle = purple_conversations_get_handle();
    purple_signal_disconnect(conv_handle, "conversation-created", fla,
            PURPLE_CALLBACK(flist_conversation_created_cb));
    purple_signal_disconnect(conv_handle, "sending-im-msg", fla,
            PURPLE_CALLBACK(flist_process_sending_im));
    purple_signal_disconnect(conv_handle, "receiving-im-msg", fla,
            PURPLE_CALLBACK(flist_process_receiving_im));
}

const char *flist_normalize(const PurpleAccount *account, const char *str) {
    return purple_normalize_nocase(account, str);
}

gint flist_parse_int(const gchar *to_parse, gboolean *success) {
    gchar *endptr;
    long ret = strtol(to_parse, &endptr, 10);
    if(endptr != to_parse + strlen(to_parse)) {
        if(success) *success = FALSE;
        return 0;
    }
    if(success) *success = TRUE;
    return ret;
}

gint json_node_get_parse_int_member(JsonNode *node, gboolean *success) {
    switch(json_node_get_value_type(node)) {
    case G_TYPE_INT64:
        if(success)
          *success = TRUE;
        return json_node_get_int(node);
    case G_TYPE_STRING:
        return flist_parse_int(json_node_get_string(node), success);
    }
    if(success) *success = FALSE;
    return 0;
}

PurpleChat* flist_get_chat(FListAccount *fla, const gchar *name) {
    PurpleChat *b = purple_blist_find_chat(fla->pa, name);
    if(!b) {
        GHashTable *components = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_free);
        g_hash_table_insert(components, g_strdup("channel"), g_strdup(name));
        b = purple_chat_new(fla->pa, name, components);
        purple_blist_add_chat(b, flist_get_chat_group(fla), NULL);
    }
    return b;
}

void flist_remove_chat(FListAccount *fla, const gchar *name) {
    PurpleGroup *g = flist_get_chat_group(fla);
    PurpleChat *b = purple_blist_find_chat(fla->pa, name);
    if(b && purple_chat_get_group(b) == g) {
        purple_blist_remove_chat(b);
    }
}

gint json_object_get_parse_int_member(JsonObject *json, const gchar *name, gboolean *success) {
    JsonNode *node = json_object_get_member(json, name);
    if(!node) {
        if(success) *success = FALSE;
        return 0;
    }
    return json_node_get_parse_int_member(node, success);
}

static GList *flist_actions(PurplePlugin *plugin, gpointer context) {
    PurpleConnection *pc = context;
    FListAccount *fla = purple_connection_get_protocol_data(pc);
    GList *list = NULL;
    PurplePluginAction *act = NULL;

    act = purple_plugin_action_new(_("Create Private Channel"), flist_create_private_channel_action);
    list = g_list_append(list, act);

    act = purple_plugin_action_new(_("Set Status"), flist_set_status_action);
    list = g_list_append(list, act);

    act = purple_plugin_action_new(_("Character Search"), flist_filter_action);
    list = g_list_append(list, act);

    act = purple_plugin_action_new(_("Ignore List"), flist_ignore_list_action);
    list = g_list_append(list, act);

    if(fla && fla->character) {
        if(fla->global_ops && g_hash_table_lookup(fla->global_ops, fla->character)) {
            list = g_list_append(list, NULL); /* this adds a divider */

            act = purple_plugin_action_new(_("Broadcast"), flist_broadcast_action);
            list = g_list_append(list, act);

            act = purple_plugin_action_new(_("Create Public Channel"), flist_create_public_channel_action);
            list = g_list_append(list, act);

            act = purple_plugin_action_new(_("Delete Public Channel"), flist_delete_public_channel_action);
            list = g_list_append(list, act);

            act = purple_plugin_action_new(_("Add Global Operator"), flist_add_global_operator_action);
            list = g_list_append(list, act);

            act = purple_plugin_action_new(_("Remove Global Operator"), flist_remove_global_operator_action);
            list = g_list_append(list, act);
        }
    }

    return list;
}

static gboolean plugin_load(PurplePlugin *plugin) {
#ifndef DISABLE_NSSFIX
    flist_nssfix_enable();
#endif
    return TRUE;
}
static gboolean plugin_unload(PurplePlugin *plugin) {
#ifndef FLIST_PURPLE_ONLY
    flist_pidgin_terminate();
#endif

#ifndef DISABLE_NSSFIX
    flist_nssfix_disable();
#endif

    return TRUE;
}
static const char *flist_list_icon(PurpleAccount *account, PurpleBuddy *buddy) {
    return "flist";
}

GList* flist_chat_info(PurpleConnection *pc) {
    GList *ret = NULL;
    struct proto_chat_entry *pce;

    pce = g_new0(struct proto_chat_entry, 1);
    pce->label = _("Channel");
    pce->identifier = "channel";
    pce->required = TRUE;
    ret = g_list_append(ret, pce);

    return ret;
}

GHashTable* flist_chat_info_defaults(PurpleConnection *pc, const char *chat_name) {
    GHashTable *table = g_hash_table_new_full(g_str_hash, g_str_equal, NULL, g_free);
    if(chat_name) g_hash_table_insert(table, "channel", g_strdup(chat_name));
    return table;
}

FListCharacter *flist_get_character(FListAccount *fla, const gchar *identity) {
    return g_hash_table_lookup(fla->all_characters, identity);
}
GSList *flist_get_all_characters(FListAccount *fla) {
    GSList *ret = NULL;
    GHashTableIter iter;
    gpointer key, value;

    g_hash_table_iter_init (&iter, fla->all_characters);
    while(g_hash_table_iter_next(&iter, &key, &value)) {
        ret = g_slist_prepend(ret, value);
    }
    return ret;
}

void flist_close(PurpleConnection *pc) {
    FListAccount *fla = purple_connection_get_protocol_data(pc);
    if(!fla) return;

    if(fla->connection_status == FLIST_CONNECT) purple_proxy_connect_cancel_with_handle((void*) pc);
    if(fla->input_handle > 0) purple_input_remove(fla->input_handle);
    if(fla->fd > 0) close(fla->fd);
    if(fla->ssl_con) purple_ssl_close(fla->ssl_con);
    if(fla->url_request) purple_util_fetch_url_cancel(fla->url_request);

    if(fla->username) g_free(fla->username);
    if(fla->character) g_free(fla->character);
    if(fla->password) g_free(fla->password);

    if(fla->ticket_request) flist_web_request_cancel(fla->ticket_request);
    if(fla->ticket_timer) purple_timeout_remove(fla->ticket_timer);

    if(fla->fls_cookie) g_free(fla->fls_cookie);
    g_free(fla->rx_buf);
    if(fla->frame_buffer) g_byte_array_free(fla->frame_buffer, TRUE);

    if(fla->ping_timeout_handle) purple_timeout_remove(fla->ping_timeout_handle);

    g_hash_table_destroy(fla->all_characters);
    if(fla->global_ops) g_hash_table_destroy(fla->global_ops);

    if(fla->saved_name) g_free(fla->saved_name);

    /* login options */
    if(fla->server_address) g_free(fla->server_address);
    if(fla->server_path) g_free(fla->server_path);

    if (fla->ignore_list) flist_g_list_free_full(fla->ignore_list, g_free);

    if(fla->input_request) purple_request_close_with_handle((void*) pc);

    if(fla->icon_blacklist) flist_g_slist_free_full(fla->icon_blacklist, g_free);

#ifndef FLIST_PURPLE_ONLY
    flist_pidgin_disable_signals(fla);
#endif
    flist_disable_signals(fla);

    flist_friends_unload(fla);
    flist_fetch_icon_cancel_all(fla);
    flist_global_kinks_unload(fla);
    flist_profile_unload(fla);
    flist_channel_subsystem_unload(fla);

    g_free(fla);

    purple_connection_set_protocol_data(pc, NULL);
}

static void flist_character_free(FListCharacter *character) {
    if(character->name) g_free(character->name);
    if(character->status_message) g_free(character->status_message);
    g_free(character);
}

void flist_welcome_notify_closed_cb(gpointer user_data) {
  gchar *text = (gchar *) user_data;
  g_free(text);
}

void flist_update_notify_closed_cb(gpointer user_data) {
  gchar *text = (gchar *) user_data;
  g_free(text);
}

// Copy pasted from prpl-irc
static PurpleAccount *find_acct(const char *prpl, const char *acct_id)
{
	PurpleAccount *acct = NULL;

	/* If we have a specific acct, use it */
	if (acct_id) {
		acct = purple_accounts_find(acct_id, prpl);
		if (acct && !purple_account_is_connected(acct))
			acct = NULL;
	} else { /* Otherwise find an active account for the protocol */
		GList *l = purple_accounts_get_all();
		while (l) {
			if (purple_strequal(prpl, purple_account_get_protocol_id(l->data))
					&& purple_account_is_connected(l->data)) {
				acct = l->data;
				break;
			}
			l = l->next;
		}
	}

	return acct;
}

static gboolean flist_uri_handler(const char *proto, const char *argument, GHashTable *params)
{
	char *acct_id = params ? g_hash_table_lookup(params, "account") : NULL;
	PurpleAccount *acct;

	// For now, only channel URLs are supported
	// eg https://www.f-list.net/fchat/?joinChannel=ADH-2b5c804cabc154006a57
	// The protocol part is not checked to allow forwarding from browser by rewriting the scheme

	if (g_ascii_strcasecmp(argument, "//www.f-list.net/fchat/"))
		return FALSE;

	acct = find_acct(FLIST_PLUGIN_ID, acct_id);

	if (!acct) {
        purple_debug_info(FLIST_DEBUG, "No connected account on %s\n", FLIST_PLUGIN_ID);
		return FALSE;
    }

    // Get channel name
	if (!params || g_hash_table_lookup_extended(params, "joinchannel", NULL, NULL)) {
        char *channel = g_hash_table_lookup(params, "joinchannel");

        g_hash_table_insert(params, g_strdup(CHANNEL_COMPONENTS_NAME), g_strdup(channel));
        serv_join_chat(purple_account_get_connection(acct), params);
		return TRUE;
    }
	
    return FALSE;
}

void flist_check_update_version(PurpleUtilFetchUrlData *url_data, gpointer user_data, const gchar *url_text, gsize len, const gchar *error_message)
{
  // In case we failed the request, fail gracefully - we don't want to disturb users
  if (url_text == NULL)
  {
    purple_debug_error(FLIST_DEBUG, "Error fetching current version information: %s", error_message);
    return;
  }

  purple_debug_info(FLIST_DEBUG, "Update Check JSON Received: %s\n", url_text);

  JsonParser *parser = json_parser_new();
  GError *err = NULL;
  json_parser_load_from_data(parser, url_text, len, &err);

  if(err) { /* not valid json */
    purple_debug_warning(FLIST_DEBUG, "Expected JSON Object, but did not parse with error message: %s\n", err->message);
    g_error_free(err);
    g_object_unref(parser);
    return;
  }

  JsonNode *root = json_parser_get_root(parser);
  if(json_node_get_node_type(root) != JSON_NODE_OBJECT) {
    purple_debug_warning(FLIST_DEBUG, "Expected JSON Object, but received a different node.\n");
    g_object_unref(parser);
    return;
  }

  JsonObject *object = json_node_get_object(root);
  if (object == NULL) {
    purple_debug_warning(FLIST_DEBUG, "Malformed update JSON, no member 'version' found!\n");
    g_object_unref(parser);
    return;
  }

  JsonObject *version = json_object_get_object_member(object, "version");

  guint major = 0, minor = 0, bugfix = 0;
  sscanf(FLIST_PLUGIN_VERSION, "%u.%u.%u", &major, &minor, &bugfix);

  guint latest_major = json_object_get_int_member(version, "major");
  guint latest_minor = json_object_get_int_member(version, "minor");
  guint latest_bugfix = json_object_get_int_member(version, "bugfix");

  gboolean newer_version = (latest_major * 10000 + latest_minor * 100 + latest_bugfix) > (major * 10000 + minor * 100 + bugfix);

  if (newer_version) {
    FListAccount *fla = user_data;

    const gchar *timestamp = json_object_get_string_member(object, "timestamp");
    time_t tmp = purple_str_to_time(timestamp, TRUE, NULL, NULL, NULL);
    struct tm *tm = localtime(&tmp);

    GString *update_txt = g_string_new(NULL);
    g_string_append_printf(update_txt,
        "<b>Version %s (%s)</b><br>" "<b><a href=\"%s\">Download here</a></b>!<br>",
        json_object_get_string_member(version, "full"),
        purple_date_format_long(tm),
        FLIST_PIDGIN_RELEASE_URL);

    const gchar *summary_raw = json_object_get_string_member(object, "summary");
    if (summary_raw != NULL && strlen(summary_raw) > 0)
    {
      gchar *summary = purple_markup_escape_text(summary_raw, -1);
      g_string_append(update_txt, "<hr><b>Changes</b><br>");

      gchar **summary_lines = g_strsplit(summary, "\n", 0);
      gchar **iter = summary_lines;

      while (*iter)
      {
        g_string_append_printf(update_txt, "%s<br>", *iter);
        iter++;
      }

      g_strfreev(summary_lines);
    }

    g_string_append_printf(update_txt,
        "<hr><b>Problems? Questions? Suggestions?</b><br>Visit us @ <a href=\"flistc://%s/" FLIST_PIDGIN_CHANNEL "\">Pidgin Development Channel</a>!",
        purple_url_encode(flist_serialize_account(fla->pa)));

    gchar *update_str = g_string_free(update_txt, FALSE);
    purple_notify_formatted(
        fla->pc, "F-List Pidgin Update Check",
        "F-List Pidgin Plugin",
        "There's a new update available!",
        update_str,
        flist_update_notify_closed_cb,
        update_str);
  }

  g_object_unref(parser);
}

void flist_login(PurpleAccount *pa) {
    PurpleConnection *pc = purple_account_get_connection(pa);
    FListAccount *fla;
    gchar **ac_split;

    fla = g_new0(FListAccount, 1);
    fla->pa = pa;
    fla->pc = pc;

    fla->all_characters = g_hash_table_new_full((GHashFunc)flist_str_hash, (GEqualFunc)flist_str_equal, g_free, (GDestroyNotify)flist_character_free);

    fla->rx_buf = g_malloc0(256); fla->rx_len = 0;
    purple_connection_set_protocol_data(pc, fla);

    ac_split = g_strsplit(purple_account_get_username(pa), ":", 2);
    fla->username = g_strdup(ac_split[0]);
    fla->character = g_strdup(ac_split[1]);
    fla->password = g_strdup(purple_account_get_password(pa));

    //we don't want to display the whole username:character thing
    if(purple_account_get_alias(pa) == NULL || flist_str_equal(purple_account_get_username(pa), purple_account_get_alias(pa))) {
        purple_account_set_alias(pa, fla->character);
    }

    /* Chat server options */
    const gchar *option_url = purple_account_get_string(pa, "server_url", FLIST_SERVER_URL);
    gchar *protocolless_url, *https_url;

    // purple_url_parse expects https:// URLS instead of wss://
    protocolless_url = g_strrstr(option_url, "://");
    if (protocolless_url) {
        https_url = g_strdup_printf("https%s", protocolless_url);
    } else {
        https_url = g_strdup_printf("https://%s", option_url);
    }

    if (!purple_url_parse(
                https_url,
                &fla->server_address,
                &fla->server_port,
                &fla->server_path,
                NULL,
                NULL)
        ) {
        purple_connection_error_reason (pc,
                PURPLE_CONNECTION_ERROR_INVALID_SETTINGS,
                _("Invalid Server URL"));
        return;

        g_free(https_url);
    }
    g_free(https_url);

    /* login options */
    fla->sync_bookmarks = purple_account_get_bool(pa, "sync_bookmarks", FALSE);
    fla->sync_friends = purple_account_get_bool(pa, "sync_friends", TRUE);
    fla->sync_status = purple_account_get_bool(pa, "sync_status", TRUE);


    fla->receive_notifications = purple_account_get_bool(pa, "receive_notifications", TRUE);
    fla->debug_mode = purple_account_get_bool(pa, "debug_mode", FALSE);
    fla->show_own_character = purple_account_get_bool(pa, "show_own_character", TRUE);
    fla->ignore_list = NULL;

    flist_channel_subsystem_load(fla);
    flist_clear_temp_groups(fla);
    flist_global_kinks_load(fla);
    flist_profile_load(fla);
    flist_friends_load(fla);

    // Enable signals once we are connected. This guarantees that the conversations submodule has been loaded.
    flist_enable_signals(fla);
#ifndef FLIST_PURPLE_ONLY
    flist_pidgin_enable_signals(fla);
#endif

    flist_ticket_timer(fla, 0);

    if (purple_account_get_bool(pa, "display_info", TRUE)) {
      GString *welcome_txt = g_string_new(NULL);
      g_string_append(welcome_txt, "<b>Need help with anything or just wanna hang out?</b><br>Join the ");
      g_string_append_printf(welcome_txt, "<a href=\"flistc://%s/" FLIST_PIDGIN_CHANNEL "\">Pidgin Development Channel</a>!", purple_url_encode(flist_serialize_account(pa)));
      g_string_append(welcome_txt, "<hr><b>Want to report a bug or suggest a new feature?</b><br>Visit our <a href=\"https://github.com/fchat-pidgin/fchat-pidgin/issues\">Bugtracker</a>.<hr>This dialog will only appear when you log in the first time.<br><br><b>Have fun!</b>");

      gchar *welcome_str = g_string_free(welcome_txt, FALSE);
      purple_notify_formatted(
          fla->pc, "F-List Pidgin - Welcome!",
          "F-List Pidgin Plugin",
          "Read on for more information about the plugin.",
          welcome_str,
          flist_welcome_notify_closed_cb,
          welcome_str);

      // Let's not show up next time the user logs in
      purple_account_set_bool(pa, "display_info", FALSE);
    }

    // Only run this check once for all connections
    static gboolean checked_for_updates = FALSE;
    if (!checked_for_updates && purple_account_get_bool(pa, "check_for_updates", TRUE))
    {
      purple_util_fetch_url(FLIST_PIDGIN_UPDATE_URL, TRUE, USER_AGENT, TRUE, flist_check_update_version, fla);
      checked_for_updates = TRUE;
    }

    purple_connection_update_progress(fla->pc, "Requesting login ticket", 1 ,5);
    g_strfreev(ac_split);
}

GList *flist_status_types(PurpleAccount *account) {
    GList *types = NULL;
    PurpleStatusType *status;

    status = purple_status_type_new_with_attrs(PURPLE_STATUS_AVAILABLE, "online", _("Available"),
        TRUE, TRUE, FALSE, FLIST_STATUS_MESSAGE_KEY, _("Message"), purple_value_new(PURPLE_TYPE_STRING), NULL);
    types = g_list_append(types, status);

    status = purple_status_type_new_with_attrs(PURPLE_STATUS_AVAILABLE, "looking", _("Looking"),
        TRUE, TRUE, FALSE, FLIST_STATUS_MESSAGE_KEY, _("Message"), purple_value_new(PURPLE_TYPE_STRING), NULL);
    types = g_list_append(types, status);

    status = purple_status_type_new_with_attrs(PURPLE_STATUS_AVAILABLE, "crown", _("Crown"),
        TRUE, FALSE, FALSE, FLIST_STATUS_MESSAGE_KEY, _("Message"), purple_value_new(PURPLE_TYPE_STRING), NULL);
    types = g_list_append(types, status);

    status = purple_status_type_new_with_attrs(PURPLE_STATUS_AWAY, "idle", _("Idle"),
        TRUE, FALSE, FALSE, FLIST_STATUS_MESSAGE_KEY, _("Message"), purple_value_new(PURPLE_TYPE_STRING), NULL);
    types = g_list_append(types, status);

    status = purple_status_type_new_with_attrs(PURPLE_STATUS_AWAY, "away", _("Away"),
        TRUE, TRUE, FALSE, FLIST_STATUS_MESSAGE_KEY, _("Message"), purple_value_new(PURPLE_TYPE_STRING), NULL);
    types = g_list_append(types, status);

    status = purple_status_type_new_with_attrs(PURPLE_STATUS_UNAVAILABLE, "busy", _("Busy"),
        TRUE, TRUE, FALSE, FLIST_STATUS_MESSAGE_KEY, _("Message"), purple_value_new(PURPLE_TYPE_STRING), NULL);
    types = g_list_append(types, status);

    status = purple_status_type_new_with_attrs(PURPLE_STATUS_UNAVAILABLE, "dnd", _("Do Not Disturb"),
        TRUE, TRUE, FALSE, FLIST_STATUS_MESSAGE_KEY, _("Message"), purple_value_new(PURPLE_TYPE_STRING), NULL);
    types = g_list_append(types, status);

    status = purple_status_type_new(PURPLE_STATUS_OFFLINE, NULL, NULL, TRUE);
    types = g_list_append(types, status);

    return types;
}

static PurplePluginProtocolInfo prpl_info = {
    OPT_PROTO_CHAT_TOPIC | OPT_PROTO_SLASH_COMMANDS_NATIVE,
    NULL,                                               /* user_splits */
    NULL,                                               /* protocol_options */
    {"png", 0, 0, 100, 100, 0, PURPLE_ICON_SCALE_SEND}, /* icon_spec */
    flist_list_icon,                                    /* list_icon */
    NULL,                                               /* list_emblems */
    flist_get_status_text,                              /* status_text */
    flist_get_tooltip,                                  /* tooltip_text */
    flist_status_types,                                 /* status_types */
    flist_blist_node_menu,                              /* blist_node_menu */
    flist_chat_info,                                    /* chat_info */
    flist_chat_info_defaults,                           /* chat_info_defaults */
    flist_login,                                        /* login */
    flist_close,                                        /* close */
    flist_send_message,                                 /* send_im */
    NULL,                                               /* set_info */
    flist_send_typing,                                  /* send_typing */
    flist_get_profile,                                  /* get_info */
    flist_purple_set_status,                            /* set_status */
    NULL,                                               /* set_idle */
    NULL,                                               /* change_passwd */
    flist_buddy_add,                                    /* add_buddy */
    NULL,                                               /* add_buddies */
    flist_buddy_remove,                                 /* remove_buddy */
    NULL,                                               /* remove_buddies */
    NULL,                                               /* add_permit */
    NULL,                                               /* add_deny */
    NULL,                                               /* rem_permit */
    NULL,                                               /* rem_deny */
    NULL,                                               /* set_permit_deny */
    flist_join_channel,                                 /* join_chat */
    NULL,                                               /* reject chat invite */
    flist_get_channel_name,                             /* get_chat_name */
    NULL,                                               /* chat_invite */
    flist_leave_channel,                                /* chat_leave */
    NULL,                                               /* chat_whisper */
    flist_send_channel_message,                         /* chat_send */
    NULL,                                               /* keepalive */
    NULL,                                               /* register_user */
    NULL,                                               /* get_cb_info */
    NULL,                                               /* get_cb_away */
    NULL,                                               /* alias_buddy */
    NULL,                                               /* group_buddy */
    NULL,                                               /* rename_group */
    NULL,                                               /* buddy_free */
    flist_convo_closed,                                 /* convo_closed */
    flist_normalize,                                    /* normalize */
    NULL,                                               /* set_buddy_icon */
    NULL,                                               /* remove_group */
    NULL,                                               /* get_cb_real_name */
    NULL,                                               /* set_chat_topic */
    NULL,                                               /* find_blist_chat */
    flist_get_roomlist,                                 /* roomlist_get_list */
    flist_cancel_roomlist,                              /* roomlist_cancel */
    NULL,                                               /* roomlist_expand_category */
    NULL,                                               /* can_receive_file */
    NULL,                                               /* send_file */
    NULL,                                               /* new_xfer */
    NULL,                                               /* offline_message */
    NULL,                                               /* whiteboard_prpl_ops */
    NULL,                                               /* send_raw */
    NULL,                                               /* roomlist_room_serialize */
    NULL,                                               /* unregister_user */
    NULL,                                               /* send_attention */
    NULL,                                               /* attention_types */
    sizeof(PurplePluginProtocolInfo),                   /* struct_size */
    NULL,                                               /* get_account_text_table */
};

static PurplePluginInfo info = {
    PURPLE_PLUGIN_MAGIC,
    PURPLE_MAJOR_VERSION,
    PURPLE_MINOR_VERSION,
    PURPLE_PLUGIN_PROTOCOL,         /* type */
    NULL,                           /* ui_requirement */
    0,                              /* flags */
    NULL,                           /* dependencies */
    PURPLE_PRIORITY_DEFAULT,        /* priority */
    FLIST_PLUGIN_ID,                /* id */
    "FList",                        /* name */
    FLIST_PLUGIN_VERSION,           /* version */
    "F-List Protocol Plugin",       /* summary */
    "F-List Protocol Plugin",       /* description */
    "TestPanther, Nelwill, Sabhak", /* author */
    FLIST_URL,                      /* homepage */
    plugin_load,                    /* load */
    plugin_unload,                  /* unload */
    NULL,                           /* destroy */
    NULL,                           /* ui_info */
    &prpl_info,                     /* extra_info */
    NULL,                           /* prefs_info */
    flist_actions,                  /* actions */
    /* padding */
    NULL,
    NULL,
    NULL,
    NULL
};

static void plugin_init(PurplePlugin *plugin) {
    PurpleAccountUserSplit *split;
    PurpleAccountOption *option;

    split = purple_account_user_split_new("Character", "", ':');
    prpl_info.user_splits = g_list_append(prpl_info.user_splits, split);

    option = purple_account_option_string_new("Chat server URL", "server_url", FLIST_SERVER_URL);
    prpl_info.protocol_options = g_list_append(prpl_info.protocol_options, option);

    option = purple_account_option_bool_new("Automatically check for updates", "check_for_updates", TRUE);
    prpl_info.protocol_options = g_list_append(prpl_info.protocol_options, option);

    option = purple_account_option_bool_new("Download Friends List", "sync_friends", TRUE);
    prpl_info.protocol_options = g_list_append(prpl_info.protocol_options, option);

    option = purple_account_option_bool_new("Download Bookmarks", "sync_bookmarks", FALSE);
    prpl_info.protocol_options = g_list_append(prpl_info.protocol_options, option);

    option = purple_account_option_bool_new("Synchronize status", "sync_status", TRUE);
    prpl_info.protocol_options = g_list_append(prpl_info.protocol_options, option);

    option = purple_account_option_bool_new("Receive notifications", "receive_notifications", TRUE);
    prpl_info.protocol_options = g_list_append(prpl_info.protocol_options, option);

    option = purple_account_option_bool_new("Display own character in conversations", "show_own_character", TRUE);
    prpl_info.protocol_options = g_list_append(prpl_info.protocol_options, option);

    option = purple_account_option_bool_new("Display information about the plugin on login", "display_info", TRUE);
    prpl_info.protocol_options = g_list_append(prpl_info.protocol_options, option);

    option = purple_account_option_bool_new("Debug Mode", "debug_mode", FALSE);
    prpl_info.protocol_options = g_list_append(prpl_info.protocol_options, option);

    option = purple_account_option_bool_new("Color user names based on their gender", "use_gender_colors", TRUE);
    prpl_info.protocol_options = g_list_append(prpl_info.protocol_options, option);

    option = purple_account_option_bool_new("Color own name based on your gender", "use_gender_colors_self", TRUE);
    prpl_info.protocol_options = g_list_append(prpl_info.protocol_options, option);

    option = purple_account_option_string_new("Roleplay Ads Background", "ads_background", FLIST_RPAD_DEFAULT_BACKGROUND);
    prpl_info.protocol_options = g_list_append(prpl_info.protocol_options, option);

    str_to_gender = g_hash_table_new(g_str_hash, g_str_equal);
    gender_to_struct = g_hash_table_new(g_direct_hash, NULL);
    gender_list = NULL;
    int i;
    for(i = 0; i < FLIST_GENDER_COUNT; i++) {
        gender_list = g_slist_prepend(gender_list, (gpointer) genders[i].name);
        g_hash_table_insert(str_to_gender, (gpointer) genders[i].name, &genders[i]);
        g_hash_table_insert(gender_to_struct, (gpointer) genders[i].gender, &genders[i]);
    }
    gender_list = g_slist_reverse(gender_list);
    g_hash_table_insert(str_to_gender, (gpointer) gender_unknown.name, &gender_unknown);
    g_hash_table_insert(gender_to_struct, (gpointer) gender_unknown.gender, &gender_unknown);

    str_to_channel_mode = g_hash_table_new(g_str_hash, g_str_equal);
    g_hash_table_insert(str_to_channel_mode, "both", GINT_TO_POINTER(CHANNEL_MODE_BOTH));
    g_hash_table_insert(str_to_channel_mode, "ads", GINT_TO_POINTER(CHANNEL_MODE_ADS_ONLY));
    g_hash_table_insert(str_to_channel_mode, "chat", GINT_TO_POINTER(CHANNEL_MODE_CHAT_ONLY));

    status_list = NULL;
    status_list = g_slist_prepend(status_list, "online");
    status_list = g_slist_prepend(status_list, "looking");
    status_list = g_slist_prepend(status_list, "away");
    status_list = g_slist_prepend(status_list, "busy");
    status_list = g_slist_prepend(status_list, "dnd");
    status_list = g_slist_reverse(status_list);

    str_to_status = g_hash_table_new(g_str_hash, g_str_equal);
    g_hash_table_insert(str_to_status, "online", GINT_TO_POINTER(FLIST_STATUS_AVAILABLE));
    g_hash_table_insert(str_to_status, "looking", GINT_TO_POINTER(FLIST_STATUS_LOOKING));
    g_hash_table_insert(str_to_status, "busy", GINT_TO_POINTER(FLIST_STATUS_BUSY));
    g_hash_table_insert(str_to_status, "dnd", GINT_TO_POINTER(FLIST_STATUS_DND));
    g_hash_table_insert(str_to_status, "crown", GINT_TO_POINTER(FLIST_STATUS_CROWN));
    g_hash_table_insert(str_to_status, "idle", GINT_TO_POINTER(FLIST_STATUS_IDLE));
    g_hash_table_insert(str_to_status, "away", GINT_TO_POINTER(FLIST_STATUS_AWAY));
    g_hash_table_insert(str_to_status, "unknown", GINT_TO_POINTER(FLIST_STATUS_UNKNOWN));

    status_to_str = g_hash_table_new(g_direct_hash, NULL);
    g_hash_table_insert(status_to_str, GINT_TO_POINTER(FLIST_STATUS_AVAILABLE), "online");
    g_hash_table_insert(status_to_str, GINT_TO_POINTER(FLIST_STATUS_LOOKING), "looking");
    g_hash_table_insert(status_to_str, GINT_TO_POINTER(FLIST_STATUS_BUSY), "busy");
    g_hash_table_insert(status_to_str, GINT_TO_POINTER(FLIST_STATUS_DND), "dnd");
    g_hash_table_insert(status_to_str, GINT_TO_POINTER(FLIST_STATUS_CROWN), "crown");
    g_hash_table_insert(status_to_str, GINT_TO_POINTER(FLIST_STATUS_AWAY), "away");
    g_hash_table_insert(status_to_str, GINT_TO_POINTER(FLIST_STATUS_IDLE), "idle");
    g_hash_table_insert(status_to_str, GINT_TO_POINTER(FLIST_STATUS_OFFLINE), "offline");
    g_hash_table_insert(status_to_str, GINT_TO_POINTER(FLIST_STATUS_UNKNOWN), "unknown");

    status_to_proper_str = g_hash_table_new(g_direct_hash, NULL);
    g_hash_table_insert(status_to_proper_str, GINT_TO_POINTER(FLIST_STATUS_AVAILABLE), "Available");
    g_hash_table_insert(status_to_proper_str, GINT_TO_POINTER(FLIST_STATUS_LOOKING), "Looking");
    g_hash_table_insert(status_to_proper_str, GINT_TO_POINTER(FLIST_STATUS_BUSY), "Busy");
    g_hash_table_insert(status_to_proper_str, GINT_TO_POINTER(FLIST_STATUS_DND), "Do Not Disturb");
    g_hash_table_insert(status_to_proper_str, GINT_TO_POINTER(FLIST_STATUS_CROWN), "Crown");
    g_hash_table_insert(status_to_proper_str, GINT_TO_POINTER(FLIST_STATUS_AWAY), "Away");
    g_hash_table_insert(status_to_proper_str, GINT_TO_POINTER(FLIST_STATUS_IDLE), "Idle");
    g_hash_table_insert(status_to_proper_str, GINT_TO_POINTER(FLIST_STATUS_OFFLINE), "Offline");
    g_hash_table_insert(status_to_proper_str, GINT_TO_POINTER(FLIST_STATUS_UNKNOWN), "Unknown");

    friend_status_to_proper_str = g_hash_table_new(g_direct_hash, NULL);
    g_hash_table_insert(friend_status_to_proper_str, GINT_TO_POINTER(FLIST_NOT_FRIEND), "No");
    g_hash_table_insert(friend_status_to_proper_str, GINT_TO_POINTER(FLIST_MUTUAL_FRIEND), "Mutual");
    g_hash_table_insert(friend_status_to_proper_str, GINT_TO_POINTER(FLIST_PENDING_IN_FRIEND), "Pending (You)");
    g_hash_table_insert(friend_status_to_proper_str, GINT_TO_POINTER(FLIST_PENDING_OUT_FRIEND), "Pending (Buddy)");

    string_to_account = g_hash_table_new((GHashFunc) flist_str_hash, (GEqualFunc) flist_str_equal);
    account_to_string = g_hash_table_new((GHashFunc) flist_str_hash, (GEqualFunc) flist_str_equal);

    flist_callback_init();
    flist_init_commands();
    flist_bbcode_init();
#ifndef FLIST_PURPLE_ONLY
    flist_pidgin_init();
#endif
    flist_web_requests_init();
    flist_ticket_init();

    purple_signal_connect(purple_get_core(), "uri-handler", plugin,
            PURPLE_CALLBACK(flist_uri_handler), NULL);
}

PURPLE_INIT_PLUGIN(flist, plugin_init, info);
